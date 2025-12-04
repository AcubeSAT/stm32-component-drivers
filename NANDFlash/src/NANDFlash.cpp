#include "NANDFlash.hpp"
#include <etl/algorithm.h>
#include <Logger.hpp>

/* ============= Bad Block Management ============= */

etl::expected<uint8_t, NANDErrorCode> MT29F::readBlockMarker(uint16_t block, uint8_t lun) {
    const NANDAddress address { lun, block, 0, BlockMarkerOffset };

    if (auto commandResult = executeReadCommandSequence(address); not commandResult.has_value()) {
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
        if ((block % 64) == 0) {
            yieldMilliseconds(1);
        }

        auto markerResult = readBlockMarker(block, lun);

        if (not markerResult.has_value()) {
            LOG_ERROR << "NAND: Failed to read block marker for block " << block;
            return etl::unexpected(markerResult.error());
        }

        if (markerResult.value() == BadBlockMarker) {
            if (auto markResult = markBadBlock(block, lun); not markResult.has_value()) {
                return markResult;
            }
        }
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    if (badBlockCount >= MaxBadBlocks) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    badBlockTable[badBlockCount] = { block, lun };
    badBlockCount++;

    return {};
}


/* ============= Write Protection ============= */

void MT29F::enableWrites() {
    if (NandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(NandWriteProtect, true);
        busyWaitNanoseconds(GpioSettleTimeNs);
    }
}

void MT29F::disableWrites() {
    if (NandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(NandWriteProtect, false);
        busyWaitNanoseconds(GpioSettleTimeNs);
    }
}


/* ============= Hardware Configuration ============= */

void MT29F::selectNandConfiguration(ChipSelect chipSelect) {
    switch (chipSelect) {
        case NCS0:
            MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS0(1);
            break;
        case NCS1:
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

    if (auto waitResult = waitForReady(TimeoutReadUs); not waitResult.has_value()) {
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

bool MT29F::validateParameterPageCRC(etl::span<const uint8_t, 256> parameterPage) {
    constexpr uint16_t CrcPolynomial = 0x8005U;
    constexpr uint16_t CrcInitialValue = 0x4F4EU;
    constexpr size_t CrcDataLength = 254;
    constexpr size_t StoredCrcLowByteOffset = 254;
    constexpr size_t StoredCrcHighByteOffset = 255;
    constexpr uint8_t BitsPerByte = 8;
    constexpr uint16_t MsbMask = 0x8000U;

    uint16_t crc = CrcInitialValue;

    for (const auto byte : parameterPage.first<CrcDataLength>()) {
        crc ^= static_cast<uint16_t>(byte) << BitsPerByte;

        for (uint8_t bit = 0; bit < BitsPerByte; bit++) {
            if ((crc & MsbMask) != 0) {
                crc = (crc << 1) ^ CrcPolynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    const uint16_t StoredCrc = static_cast<uint16_t>(parameterPage[StoredCrcLowByteOffset]) |
                               (static_cast<uint16_t>(parameterPage[StoredCrcHighByteOffset]) << BitsPerByte);

    return crc == StoredCrc;
}

etl::expected<void, NANDErrorCode> MT29F::validateDeviceParameters() {
    constexpr size_t OnfiDataBytesPerPageOffset = 80;
    constexpr size_t OnfiSpareBytesPerPageOffset = 84;
    constexpr size_t OnfiPagesPerBlockOffset = 92;
    constexpr size_t OnfiBlocksPerLunOffset = 96;
    constexpr uint8_t OnfiParameterPageCopies = 3;

    sendCommand(Commands::READ_PARAM_PAGE);
    sendAddress(0x00U);
    busyWaitNanoseconds(TwhrNs);

    if (auto waitResult = waitForReady(TimeoutReadUs); not waitResult.has_value()) {
        return waitResult;
    }

    busyWaitNanoseconds(TrrNs);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TwhrNs);

    for (uint8_t copy = 0; copy < OnfiParameterPageCopies; copy++) {
        etl::array<uint8_t, 256> parametersPageData;

        for (auto& byte : parametersPageData) {
            byte = readData();
        }

        if (validateParameterPageCRC(parametersPageData)) {
            auto asUint16 = [](uint8_t byte0, uint8_t byte1) -> uint16_t {
                return byte0 | (static_cast<uint16_t>(byte1) << 8);
            };

            auto asUint32 = [](uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) -> uint32_t {
                return byte0 | (static_cast<uint32_t>(byte1) << 8) |
                       (static_cast<uint32_t>(byte2) << 16) | (static_cast<uint32_t>(byte3) << 24);
            };

            const uint32_t ReadDataBytesPerPage = asUint32(parametersPageData[OnfiDataBytesPerPageOffset],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 1],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 2],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 3]);

            const uint16_t ReadSpareBytesPerPage = asUint16(parametersPageData[OnfiSpareBytesPerPageOffset],
                                                            parametersPageData[OnfiSpareBytesPerPageOffset + 1]);

            const uint32_t ReadPagesPerBlock = asUint32(parametersPageData[OnfiPagesPerBlockOffset],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 1],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 2],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 3]);

            const uint32_t ReadBlocksPerLun = asUint32(parametersPageData[OnfiBlocksPerLunOffset],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 1],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 2],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 3]);

            if (ReadDataBytesPerPage != DataBytesPerPage) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (ReadSpareBytesPerPage != SpareBytesPerPage) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (ReadPagesPerBlock != PagesPerBlock) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (ReadBlocksPerLun != BlocksPerLun) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            return {};
        }
    }

    return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
}


/* ============= Address and Status Utilities ============= */

void MT29F::buildAddressCycles(const NANDAddress& address, AddressCycles& cycles) {
    constexpr uint8_t ByteMask = 0xFFU;
    constexpr uint8_t ColumnHighByteMask = 0x3FU;
    constexpr uint8_t PageMask = 0x7FU;
    constexpr uint8_t SingleBitMask = 0x01U;
    constexpr uint8_t BlockHighBitsMask = 0x07U;

    constexpr uint8_t BitsPerByte = 8;
    constexpr uint8_t PageBitsWidth = 7;
    constexpr uint8_t BlockShiftForRowAddress2 = 1;
    constexpr uint8_t BlockShiftForRowAddress3 = 9;
    constexpr uint8_t LunShiftInRowAddress3 = 3;

    cycles[AddressCycle::COLUMN_ADDRESS_1] = address.column & ByteMask;
    cycles[AddressCycle::COLUMN_ADDRESS_2] = (address.column >> BitsPerByte) & ColumnHighByteMask;
    cycles[AddressCycle::ROW_ADDRESS_1] = (address.page & PageMask) | ((address.block & SingleBitMask) << PageBitsWidth);
    cycles[AddressCycle::ROW_ADDRESS_2] = (address.block >> BlockShiftForRowAddress2) & ByteMask;
    cycles[AddressCycle::ROW_ADDRESS_3] = ((address.block >> BlockShiftForRowAddress3) & BlockHighBitsMask) |
                                          ((address.lun & SingleBitMask) << LunShiftInRowAddress3);
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

etl::expected<void, NANDErrorCode> MT29F::ensureDeviceReady() {
    if (not isReady(readStatusRegister())) {
        return etl::unexpected(NANDErrorCode::DEVICE_BUSY);
    }
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::verifyWriteEnabled() {
    if (NandWriteProtect != PIO_PIN_NONE) {
        if (isWriteProtected(readStatusRegister())) {
            return etl::unexpected(NANDErrorCode::WRITE_PROTECTED);
        }
    }
    return {};
}

/* ============= Wait Policy ============= */

etl::expected<void, NANDErrorCode> MT29F::waitForReady(uint32_t timeoutUs) {
    const bool usePureBusyWait = (timeoutUs <= BusyWaitThresholdUs);
    uint32_t elapsedUs = 0;

    if (NandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(NandReadyBusyPin) == 0) {
            if (elapsedUs > timeoutUs) {
                return etl::unexpected(NANDErrorCode::TIMEOUT);
            }

            if (usePureBusyWait || elapsedUs < BusyWaitThresholdUs) {
                busyWaitMicroseconds(BusyWaitPollIntervalUs);
                elapsedUs += BusyWaitPollIntervalUs;
            } else {
                yieldMilliseconds(YieldIntervalMs);
                elapsedUs += YieldIntervalMs * 1000U;
            }
        }
    }

    while (true) {
        if (isReady(readStatusRegister())) {
            return {};
        }

        if (elapsedUs > timeoutUs) {
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }

        if (usePureBusyWait || elapsedUs < BusyWaitThresholdUs) {
            busyWaitMicroseconds(BusyWaitPollIntervalUs);
            elapsedUs += BusyWaitPollIntervalUs;
        } else {
            yieldMilliseconds(YieldIntervalMs);
            elapsedUs += YieldIntervalMs * 1000U;
        }
    }
}


/* ============= Timing Utilities ============= */

void MT29F::busyWaitNanoseconds(uint32_t nanoseconds) {
    constexpr uint32_t HertzPerMegahertz = 1000000U;
    constexpr uint32_t NanosecondsPerMicrosecond = 1000U;
    constexpr uint32_t CpuMhz = CPU_CLOCK_FREQUENCY / HertzPerMegahertz;

    const uint32_t Cycles = (nanoseconds * CpuMhz) / NanosecondsPerMicrosecond;

    for (volatile uint32_t i = 0; i < Cycles; i++) {
        __asm__ volatile ("nop");
    }
}

void MT29F::busyWaitMicroseconds(uint32_t microseconds) {
    busyWaitNanoseconds(microseconds * 1000U);
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

    if (auto resetResult = reset(); not resetResult.has_value()) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }

    etl::array<uint8_t, 5> deviceId;
    
    readDeviceID(deviceId);

    if (not etl::equal(ExpectedDeviceId.begin(), ExpectedDeviceId.end(), deviceId.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    constexpr etl::array<uint8_t, 4> expectedOnfi { 'O', 'N', 'F', 'I' };
    etl::array<uint8_t, 4> onfiSignature;

    readONFISignature(onfiSignature);

    if (not etl::equal(onfiSignature.begin(), onfiSignature.end(), expectedOnfi.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    if (auto parameterResult = validateDeviceParameters(); not parameterResult.has_value()) {
        return parameterResult;
    }

    disableWrites();

    if (auto scanResult = scanFactoryBadBlocks(); not scanResult.has_value()) {
        return scanResult;
    }

    isInitialized = true;

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::reset() {
    sendCommand(Commands::RESET);
    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutResetUs); not waitResult.has_value()) {
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

    if (auto validateResult = validateAddress(address); not validateResult.has_value()) {
        return validateResult;
    }

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    if (auto commandResult = executeReadCommandSequence(address); not commandResult.has_value()) {
        return commandResult;
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

    if (auto validateResult = validateAddress(address); not validateResult.has_value()) {
        return validateResult;
    }

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    WriteEnableGuard guard(*this);

    if (auto wpResult = verifyWriteEnabled(); not wpResult.has_value()) {
        return wpResult;
    }

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

    if (auto waitResult = waitForReady(TimeoutProgramUs); not waitResult.has_value()) {
        return waitResult;
    }

    if (hasOperationFailed(readStatusRegister())) {
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

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    WriteEnableGuard guard(*this);

    if (auto wpResult = verifyWriteEnabled(); not wpResult.has_value()) {
        return wpResult;
    }

    const NANDAddress address { lun, block, 0, 0 };
    AddressCycles cycles;
    buildAddressCycles(address, cycles);

    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles[AddressCycle::ROW_ADDRESS_1]);
    sendAddress(cycles[AddressCycle::ROW_ADDRESS_2]);
    sendAddress(cycles[AddressCycle::ROW_ADDRESS_3]);
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);
    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutEraseUs); not waitResult.has_value()) {
        return waitResult;
    }

    if (hasOperationFailed(readStatusRegister())) {
        return etl::unexpected(NANDErrorCode::ERASE_FAILED);
    }

    return {};
}


/* ============= Public Interface - Bad Block Management ============= */

bool MT29F::isBlockBad(uint16_t block, uint8_t lun) const {
    for (const auto& entry : etl::span(badBlockTable).first(badBlockCount)) {
        if ((entry.blockNumber == block) and (entry.lun == lun)) {
            return true;
        }
    }
    return false;
}


/* ==================== Debug ==================== */

const char* toString(NANDErrorCode error) {
    switch (error) {
        case NANDErrorCode::TIMEOUT:
            return "Timeout";
        case NANDErrorCode::ADDRESS_OUT_OF_BOUNDS:
            return "Address out of bounds";
        case NANDErrorCode::DEVICE_BUSY:
            return "Device busy";
        case NANDErrorCode::PROGRAM_FAILED:
            return "Program operation failed";
        case NANDErrorCode::ERASE_FAILED:
            return "Erase operation failed";
        case NANDErrorCode::WRITE_PROTECTED:
            return "Device write protected";
        case NANDErrorCode::INVALID_PARAMETER:
            return "Invalid parameter";
        case NANDErrorCode::NOT_INITIALIZED:
            return "Driver not initialized";
        case NANDErrorCode::HARDWARE_FAILURE:
            return "Hardware failure";
        case NANDErrorCode::BAD_PARAMETER_PAGE:
            return "Bad parameter page";
        default:
            return "Unknown error";
    }
}
