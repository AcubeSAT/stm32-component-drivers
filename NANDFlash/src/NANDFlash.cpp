#include "NANDFlash.hpp"
#include <etl/algorithm.h>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ============= Bad Block Management ============= */

etl::expected<uint8_t, NANDErrorCode> MT29F::readBlockMarker(uint16_t block, uint8_t lun) {
    const NANDAddress address { lun, block, 0, BlockMarkerOffset };

    if (auto commandResult = executeReadCommandSequence(address); not commandResult) {
        return etl::unexpected(commandResult.error());
    }

    const uint8_t marker = readData();
    busyWaitNanoseconds(TrhwNs);

    return marker;
}

etl::expected<void, NANDErrorCode> MT29F::scanFactoryBadBlocks(uint8_t lun) {

    if (lun >= LunsPerCe) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    for (uint16_t block = 0; block < BlocksPerLun; block++) {

        if ((block % 5) == 0) {
            vTaskDelay(1);
        }

        auto markerResult = readBlockMarker(block, lun);
        bool isBlockBad = false;

        if ((not markerResult) or (markerResult.value() == BadBlockMarker)) {
            // If we can't read the marker, assume block is bad for safety
            isBlockBad = true;
        }

        if (isBlockBad) {
            if (badBlockCount >= MaxBadBlocks) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            markBadBlock(block, lun);
        }
    }

    return {};
}

void MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    if (badBlockCount >= MaxBadBlocks) {
        // For now it's ok just returning. It will be reworked with the integration of littlefs.
        return;
    }

    badBlockTable[badBlockCount] = {block, lun};
    badBlockCount++;
}


/* ============= Write Protection ============= */

void MT29F::enableWrites() const {
    if (NandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(NandWriteProtect, true);
        busyWaitNanoseconds(GpioSettleTimeNs);  // GPIO settling time
    }
}

void MT29F::disableWrites() const {
    if (NandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(NandWriteProtect, false);
        busyWaitNanoseconds(GpioSettleTimeNs);  // Ensure WP# propagates
    }
}


/* ============= Hardware Configuration ============= */

void MT29F::selectNandConfiguration(ChipSelect chipSelect) {
    switch (chipSelect) {
        case NCS0:
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS0(1);
            break;
        case NCS1:
            MATRIX_REGS->CCFG_SMCNFCS &= 0xFU;
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS1(1);
            break;
        case NCS2:
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS2(1);
            break;
        case NCS3:
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS3(1);
            break;
        default:
            break;
    }
}


/* ============= Command Sequences ============= */

etl::expected<void, NANDErrorCode> MT29F::executeReadCommandSequence(const NANDAddress& address) {
    AddressCycles cycles;
    buildAddressCycles(address, cycles);

    sendCommand(Commands::READ_MODE);

    for (const auto& cycle : cycles) {
        sendAddress(cycle);
    }

    sendCommand(Commands::READ_CONFIRM);
    busyWaitNanoseconds(TwbNs);

    auto waitResult = waitForReady(TimeoutReadMs);
    if (not waitResult) {
        return waitResult;
    }
    
    busyWaitNanoseconds(TrrNs);

    sendCommand(Commands::READ_MODE);

    busyWaitNanoseconds(TwhrNs);

    return {};
}


/* ============= Device Identification and Validation ============= */

void MT29F::readDeviceID(etl::span<uint8_t, 5> id) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::MANUFACTURER_ID));
    busyWaitNanoseconds(TwhrNs);

    for (auto& byte : id) {
        byte = readData();
    }

    busyWaitNanoseconds(TrhwNs);
}

void MT29F::readONFISignature(etl::span<uint8_t, 4> signature) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::ONFI_SIGNATURE));
    busyWaitNanoseconds(TwhrNs);

    for (auto& byte : signature) {
        byte = readData();
    }

    busyWaitNanoseconds(TrhwNs);
}

bool MT29F::validateParameterPageCRC(const etl::array<uint8_t, 256>& paramPage) {
    constexpr uint16_t CrcPolynomial = 0x8005U;
    uint16_t crc = 0x4F4EU;

    for (size_t byte = 0; byte < 254; byte++) {
        crc ^= static_cast<uint16_t>(paramPage[byte]) << 8;

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ CrcPolynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    const uint16_t storedCrc = static_cast<uint16_t>(paramPage[254]) | (static_cast<uint16_t>(paramPage[255]) << 8);

    return (crc == storedCrc);
}

etl::expected<void, NANDErrorCode> MT29F::validateDeviceParameters() {
    sendCommand(Commands::READ_PARAM_PAGE);
    sendAddress(0x00U);
    busyWaitNanoseconds(TwhrNs);

    if (auto waitResult = waitForReady(TimeoutReadMs); not waitResult) {
        return etl::unexpected(waitResult.error());
    }

    busyWaitNanoseconds(TrrNs);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TwhrNs);

    // ONFI requires 3 redundant copies of parameter page. Try each copy until we find a valid one
    for (uint8_t copy = 0; copy < 3; copy++) {
        etl::array<uint8_t, 256> paramPageData;

        for (auto& byte : paramPageData) {
            byte = readData();
        }

        if (validateParameterPageCRC(paramPageData)) {
            // Extract geometry values from parameter page (little-endian)
            auto asUint16 = [](uint8_t low, uint8_t high) -> uint16_t {
                return low | (static_cast<uint16_t>(high) << 8);
            };

            auto asUint32 = [](uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) -> uint32_t {
                return b0 | (static_cast<uint32_t>(b1) << 8) |
                       (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 24);
            };

            const uint32_t readDataBytesPerPage = asUint32(paramPageData[80], paramPageData[81], paramPageData[82], paramPageData[83]);
            const uint16_t readSpareBytesPerPage = asUint16(paramPageData[84], paramPageData[85]);
            const uint32_t readPagesPerBlock = asUint32(paramPageData[92], paramPageData[93], paramPageData[94], paramPageData[95]);
            const uint32_t readBlocksPerLUN = asUint32(paramPageData[96], paramPageData[97], paramPageData[98], paramPageData[99]);

            if (readDataBytesPerPage != DataBytesPerPage) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readSpareBytesPerPage != SpareBytesPerPage) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readPagesPerBlock != PagesPerBlock) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readBlocksPerLUN != BlocksPerLun) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            return {};
        }
    }

    return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
}


/* ============= Address and Status Utilities ============= */

void MT29F::buildAddressCycles(const NANDAddress& address, AddressCycles& cycles) {
    cycles[AddressCycle::CA1] = address.column & 0xFFU;
    cycles[AddressCycle::CA2] = (address.column >> 8) & 0x3FU;
    cycles[AddressCycle::RA1] = (address.page & 0x7FU) | ((address.block & 0x01U) << 7);
    cycles[AddressCycle::RA2] = (address.block >> 1) & 0xFFU;
    cycles[AddressCycle::RA3] = ((address.block >> 9) & 0x07U) | ((address.lun & 0x01U) << 3);
}

etl::expected<void, NANDErrorCode> MT29F::validateAddress(const NANDAddress& address) {
    if ((address.lun >= LunsPerCe)
     or (address.block >= BlocksPerLun)
     or (address.page >= PagesPerBlock)
     or (address.column >= TotalBytesPerPage)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    return {};
}

uint8_t MT29F::readStatusRegister() {
    sendCommand(Commands::READ_STATUS);
    busyWaitNanoseconds(TwhrNs);

    uint8_t status = readData();
    busyWaitNanoseconds(TrhwNs);

    return status;
}

etl::expected<void, NANDErrorCode> MT29F::waitForReady(uint32_t timeoutMs) {
    const TickType_t startTick = xTaskGetTickCount();
    const TickType_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    if (NandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(NandReadyBusyPin) == 0) {
            if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
                return etl::unexpected(NANDErrorCode::TIMEOUT);
            }
            vTaskDelay(1);
        }
    }

    while (true) {
        auto status = readStatusRegister();

        if ((status & StatusRdy) != 0 and (status & StatusArdy) != 0) {
            return {};  // Ready
        }

        if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }

        vTaskDelay(1);
    }
}


/* ============= Timing Utilities ============= */

void MT29F::busyWaitNanoseconds(uint32_t nanoseconds) {
    constexpr uint32_t CPU_MHZ = CPU_CLOCK_FREQUENCY / 1000000UL;

    const uint32_t cycles = (nanoseconds * CPU_MHZ) / 1000UL;

    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm__ volatile ("nop");
    }
}


/* ============= Public Interface - Initialization ============= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    if (isInitialized) {
        LOG_WARNING << "NAND: Already initialized, skipping";
        return {};
    }

    if (NandWriteProtect == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Write protection pin not provided. Hardware write protection disabled";
    }

    if (NandReadyBusyPin == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Ready/busy pin not provided. Using status register polling";
    }

    if (auto resetResult = reset(); not resetResult) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }

    etl::array<uint8_t, 5> deviceId;
    readDeviceID(deviceId);

    if (not etl::equal(ExpectedDeviceId.begin(), ExpectedDeviceId.end(), deviceId.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    etl::array<uint8_t, 4> onfiSignature;
    readONFISignature(onfiSignature);

    constexpr etl::array<uint8_t, 4> expectedOnfi {'O', 'N', 'F', 'I'};
    if (not etl::equal(onfiSignature.begin(), onfiSignature.end(), expectedOnfi.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    if (auto parameterResult = validateDeviceParameters(); not parameterResult) {
        return parameterResult;
    }

    disableWrites();

    if (auto scanResult = scanFactoryBadBlocks(); not scanResult) {
        return scanResult;
    }

    isInitialized = true;
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::reset() {
    sendCommand(Commands::RESET);
    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutResetMs); not waitResult) {
        return waitResult;
    }

    return {};
}


/* ============= Public Interface - Data Operations ============= */

etl::expected<void, NANDErrorCode> MT29F::readPage(const NANDAddress& address, etl::span<uint8_t> data) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (data.size() > (TotalBytesPerPage - address.column)) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    if (auto validateResult = validateAddress(address); not validateResult) {
        return validateResult;
    }

    if (auto status = readStatusRegister();
        (status & StatusRdy) == 0 or (status & StatusArdy) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    if (auto commandResult = executeReadCommandSequence(address); not commandResult) {
        return etl::unexpected(commandResult.error());
    }

    for (auto& byte : data) {
        byte = readData();
    }

    busyWaitNanoseconds(TrhwNs);

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programPage(const NANDAddress& address, etl::span<const uint8_t> data) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (data.size() > (TotalBytesPerPage - address.column)) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    if ((address.column <= BlockMarkerOffset) and ((address.column + data.size()) > BlockMarkerOffset)) {
        const size_t markerIndex = BlockMarkerOffset - address.column;
        const uint8_t markerValue = data[markerIndex];
        if (markerValue != GoodBlockMarker) {
            return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
        }
    }

    if (auto validateResult = validateAddress(address); not validateResult) {
        return validateResult;
    }

    if (auto status = readStatusRegister();
        (status & StatusRdy) == 0 or (status & StatusArdy) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this);

    AddressCycles cycles;
    buildAddressCycles(address, cycles);

    sendCommand(Commands::PAGE_PROGRAM);

    for (const auto& cycle : cycles) {
        sendAddress(cycle);
    }

    busyWaitNanoseconds(TadlNs);

    for (const auto& byte : data) {
        sendData(byte);
    }

    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);
    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutProgramMs); not waitResult) {
        return waitResult;
    }

    if (auto status = readStatusRegister(); (status & StatusFail) != 0) {
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::eraseBlock(uint16_t block, uint8_t lun) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((block >= BlocksPerLun) or (lun >= LunsPerCe)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    if (auto status = readStatusRegister();
        (status & StatusRdy) == 0 or (status & StatusArdy) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this);

    const NANDAddress address { lun, block, 0, 0 };
    AddressCycles cycles;
    buildAddressCycles(address, cycles);

    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles[AddressCycle::RA1]);
    sendAddress(cycles[AddressCycle::RA2]);
    sendAddress(cycles[AddressCycle::RA3]);
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);
    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutEraseMs); not waitResult) {
        return waitResult;
    }

    if (auto status = readStatusRegister(); (status & StatusFail) != 0) {
        markBadBlock(block, lun);
        return etl::unexpected(NANDErrorCode::ERASE_FAILED);
    }

    return {};
}


/* ============= Public Interface - Bad Block Management ============= */

bool MT29F::isBlockBad(uint16_t block, uint8_t lun) const {
    for (size_t tableIndex = 0; tableIndex < badBlockCount; tableIndex++) {
        if ((badBlockTable[tableIndex].blockNumber == block) && (badBlockTable[tableIndex].lun == lun)) {
            return true;
        }
    }
    return false;
}


/* ==================== Debug ==================== */

const char* toString(NANDErrorCode error) {
    switch (error) {
        case NANDErrorCode::TIMEOUT: return "Timeout";
        case NANDErrorCode::ADDRESS_OUT_OF_BOUNDS: return "Address out of bounds";
        case NANDErrorCode::BUSY_IO: return "Device busy - I/O";
        case NANDErrorCode::BUSY_ARRAY: return "Device busy - Array";
        case NANDErrorCode::PROGRAM_FAILED: return "Program operation failed";
        case NANDErrorCode::ERASE_FAILED: return "Erase operation failed";
        case NANDErrorCode::READ_FAILED: return "Read operation failed";
        case NANDErrorCode::NOT_READY: return "Device not ready";
        case NANDErrorCode::WRITE_PROTECTED: return "Device write protected";
        case NANDErrorCode::BAD_BLOCK: return "Bad block detected";
        case NANDErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case NANDErrorCode::NOT_INITIALIZED: return "Driver not initialized";
        case NANDErrorCode::HARDWARE_FAILURE: return "Hardware failure";
        case NANDErrorCode::BAD_PARAMETER_PAGE: return "Bad parameter page";
        default: return "Unknown error";
    }
}
