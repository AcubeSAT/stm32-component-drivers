#include "NANDFlash.h"
#include <etl/algorithm.h>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ================= SMC Configuration ================= */

void MT29F::selectNandConfiguration(ChipSelect chipSelect) {
    switch (chipSelect) {
        case NCS0:
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS0(1);
            break;
        case NCS1:
            MATRIX_REGS->CCFG_SMCNFCS &= 0xF;
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

/* ================= Timing Utilities ================= */

void MT29F::busyWaitNanoseconds(uint32_t nanoseconds) {
    constexpr uint32_t CPU_MHZ = CPU_CLOCK_FREQUENCY / 1000000UL;

    const uint32_t cycles = (nanoseconds * CPU_MHZ) / 1000UL;

    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm__ volatile ("nop");
    }
}

/* ================= Driver Initialization and Basic Info ================= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    if (isInitialized) {
        LOG_WARNING << "NAND: Already initialized, skipping";
        return {};
    }

    if (nandWriteProtect == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Write protection pin not provided. Hardware write protection disabled";
    }

    if (nandReadyBusyPin == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Ready/busy pin not provided. Using status register polling";
    }

    auto resetResult = reset();
    if (not resetResult) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }
    
    etl::array<uint8_t, 5> deviceId;
    readDeviceID(deviceId);

    if (not etl::equal(EXPECTED_DEVICE_ID.begin(), EXPECTED_DEVICE_ID.end(), deviceId.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    etl::array<uint8_t, 4> onfiSignature;
    readONFISignature(onfiSignature);

    constexpr etl::array<uint8_t, 4> expectedOnfi = {'O', 'N', 'F', 'I'};
    if (not etl::equal(onfiSignature.begin(), onfiSignature.end(), expectedOnfi.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    auto parameterResult = validateDeviceParameters();
    if (not parameterResult) {
        return parameterResult;
    }

    disableWrites();

    auto scanResult = scanFactoryBadBlocks();
    if (not scanResult) {
        return scanResult;
    }

    isInitialized = true;
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::reset() {
    sendCommand(Commands::RESET);
    busyWaitNanoseconds(TWB_NS);
    
    auto waitResult = waitForReady(TIMEOUT_RESET_MS);
    if (not waitResult) {
        return waitResult;
    }
    
    return {};
}

void MT29F::readDeviceID(etl::span<uint8_t, 5> id) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::MANUFACTURER_ID));
    busyWaitNanoseconds(TWHR_NS);

    for (auto& byte : id) {
        byte = readData();
    }

    busyWaitNanoseconds(TRHW_NS);
}

void MT29F::readONFISignature(etl::span<uint8_t, 4> signature) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::ONFI_SIGNATURE));
    busyWaitNanoseconds(TWHR_NS);

    for (auto& byte : signature) {
        byte = readData();
    }

    busyWaitNanoseconds(TRHW_NS);
}

etl::expected<void, NANDErrorCode> MT29F::validateDeviceParameters() {
    sendCommand(Commands::READ_PARAM_PAGE);
    sendAddress(0x00);
    busyWaitNanoseconds(TWHR_NS);

    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (not waitResult) {
        return etl::unexpected(waitResult.error());
    }

    busyWaitNanoseconds(TRR_NS);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);
    
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
    
            if (readDataBytesPerPage != DATA_BYTES_PER_PAGE) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readSpareBytesPerPage != SPARE_BYTES_PER_PAGE) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readPagesPerBlock != PAGES_PER_BLOCK) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readBlocksPerLUN != BLOCKS_PER_LUN) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            return {};
        }
    }

    return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
}


/* ===================== Data Operations ===================== */

etl::expected<void, NANDErrorCode> MT29F::executeReadCommandSequence(const NANDAddress& addr) {
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::READ_MODE);

    for (const auto& addressByte : cycles.cycle) {
        sendAddress(addressByte);
    }

    sendCommand(Commands::READ_CONFIRM);
    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (not waitResult) {
        return waitResult;
    }

    busyWaitNanoseconds(TRR_NS);
    
    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readPage(const NANDAddress& addr, etl::span<uint8_t> data) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((addr.column + data.size()) > TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto validateResult = validateAddress(addr);
    if (not validateResult) {
        return validateResult;
    }

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    auto cmdResult = executeReadCommandSequence(addr);
    if (not cmdResult) {
        return etl::unexpected(cmdResult.error());
    }

    for (auto& byte : data) {
        byte = readData();
    }

    busyWaitNanoseconds(TRHW_NS);

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programPage(const NANDAddress& addr, etl::span<const uint8_t> data) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (addr.column + data.size() > TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto validateResult = validateAddress(addr);
    if (not validateResult) {
        return validateResult;
    }

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this); 

    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::PAGE_PROGRAM);

    for (const auto& addressByte : cycles.cycle) {
        sendAddress(addressByte);
    }

    busyWaitNanoseconds(TADL_NS);

    for (const auto& byte : data) {
        sendData(byte);
    }

    sendCommand(Commands::CHANGE_WRITE_COLUMN);
    sendAddress(DATA_BYTES_PER_PAGE & 0xFF);
    sendAddress((DATA_BYTES_PER_PAGE >> 8) & 0x3F);
    busyWaitNanoseconds(TCCS_NS);

    sendData(GOOD_BLOCK_MARKER);

    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);
    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_PROGRAM_MS);
    if (not waitResult) {
        return waitResult;
    }

    status = readStatusRegister();

    if ((status & STATUS_FAIL) != 0) {
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::eraseBlock(uint16_t block, uint8_t lun) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((block >= BLOCKS_PER_LUN) || (lun >= LUNS_PER_CE)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this);

    const NANDAddress addr{lun, block, 0, 0};
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles.cycle[2]);
    sendAddress(cycles.cycle[3]);
    sendAddress(cycles.cycle[4]);
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);
    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_ERASE_MS);
    if (not waitResult) {
        return waitResult;
    }

    status = readStatusRegister();

    if ((status & STATUS_FAIL) != 0) {
        markBadBlock(block, lun);
        return etl::unexpected(NANDErrorCode::ERASE_FAILED);
    }

    return {};
}

/* ================= Internal Helper Functions ================= */

void MT29F::buildAddressCycles(const NANDAddress& addr, AddressCycles& cycles) {
    cycles.cycle[0] = addr.column & 0xFF;                                       // CA1
    cycles.cycle[1] = (addr.column >> 8) & 0x3F;                                // CA2
    cycles.cycle[2] = (addr.page & 0x7F) | ((addr.block & 0x01) << 7);          // RA1
    cycles.cycle[3] = (addr.block >> 1) & 0xFF;                                 // RA2
    cycles.cycle[4] = ((addr.block >> 9) & 0x07) | ((addr.lun & 0x01) << 3);    // RA3
}

etl::expected<void, NANDErrorCode> MT29F::waitForReady(uint32_t timeoutMs) {
    const TickType_t startTick = xTaskGetTickCount();
    const TickType_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    if (nandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {
            if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
                return etl::unexpected(NANDErrorCode::TIMEOUT);
            }
            vTaskDelay(1);
        }
    }

    while (true) {
        auto status = readStatusRegister();

        if ((status & STATUS_RDY) != 0 && (status & STATUS_ARDY) != 0) {
            return {};  // Ready
        }

        if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }

        vTaskDelay(1);
    }
}

uint8_t MT29F::readStatusRegister() {
    sendCommand(Commands::READ_STATUS);
    busyWaitNanoseconds(TWHR_NS);

    uint8_t status = readData();
    busyWaitNanoseconds(TRHW_NS);

    return status;
}

etl::expected<void, NANDErrorCode> MT29F::validateAddress(const NANDAddress& addr) {
    if ((addr.lun >= LUNS_PER_CE)      ||
        (addr.block >= BLOCKS_PER_LUN) ||
        (addr.page >= PAGES_PER_BLOCK) ||
        (addr.column >= TOTAL_BYTES_PER_PAGE)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    return {};
}

bool MT29F::validateParameterPageCRC(const etl::array<uint8_t, 256>& paramPage) {
    constexpr uint16_t CRC_POLYNOMIAL = 0x8005;
    uint16_t crc = 0x4F4E;
    
    for (size_t byte = 0; byte < 254; byte++) {
        crc ^= static_cast<uint16_t>(paramPage[byte]) << 8;
        
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    
    const uint16_t storedCrc = static_cast<uint16_t>(paramPage[254]) | (static_cast<uint16_t>(paramPage[255]) << 8);
    
    return (crc == storedCrc);
}

/* ================== Bad Block Management ================== */

bool MT29F::isBlockBad(uint16_t block, uint8_t lun) const {
    for (size_t tableIndex = 0; tableIndex < badBlockCount; tableIndex++) {
        if ((badBlockTable[tableIndex].blockNumber == block) && (badBlockTable[tableIndex].lun == lun)) {
            return true;
        }
    }
    return false;
}

void MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    if (badBlockCount >= MAX_BAD_BLOCKS) {
        // For now it's ok just returning. It will be reworked with the integration of littlefs.
        return;
    }

    badBlockTable[badBlockCount] = {block, lun};
    badBlockCount++;
}

etl::expected<void, NANDErrorCode> MT29F::scanFactoryBadBlocks(uint8_t lun) {

    if (lun >= LUNS_PER_CE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    for (uint16_t block = 0; block < BLOCKS_PER_LUN; block++) {

        if ((block % 5) == 0) {
            vTaskDelay(1);
        }

        auto markerResult = readBlockMarker(block, lun);
        bool isBlockBad = false;

        if ((not markerResult) || (markerResult.value() == BAD_BLOCK_MARKER)) {
            // If we can't read the marker, assume block is bad for safety
            isBlockBad = true;
        }

        if (isBlockBad) {
            if (badBlockCount >= MAX_BAD_BLOCKS) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            markBadBlock(block, lun);
        }
    }

    return {};
}

etl::expected<uint8_t, NANDErrorCode> MT29F::readBlockMarker(uint16_t block, uint8_t lun) {
    // Based on the datasheet the bad block marker is stored in the first page only
    // in byte 0 of the spare area
    const NANDAddress addr{lun, block, 0, BLOCK_MARKER_OFFSET};

    auto cmdResult = executeReadCommandSequence(addr);
    if (not cmdResult) {
        return etl::unexpected(cmdResult.error());
    }

    const uint8_t marker = readData();
    busyWaitNanoseconds(TRHW_NS);

    return marker;
}

/* ================== Write Protection Management ======================= */

void MT29F::enableWrites() const {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, true);
        busyWaitNanoseconds(GPIO_SETTLE_TIME_NS);  // GPIO settling time
    }
}

void MT29F::disableWrites() const {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, false);
        busyWaitNanoseconds(GPIO_SETTLE_TIME_NS);  // Ensure WP# propagates
    }
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
