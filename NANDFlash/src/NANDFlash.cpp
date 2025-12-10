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

    badBlockBitset[lun].reset();

    for (uint16_t block = 0; block < BlocksPerLun; block++) {
        if ((block % BlockScanYieldInterval) == 0) {
            yieldMilliseconds(1);
        }

        etl::expected<uint8_t, NANDErrorCode> readResult;
        bool readSucceeded = false;

        for (uint8_t attempt = 0; attempt < BlockMarkerReadRetries; attempt++) {
            readResult = readBlockMarker(block, lun);

            if (readResult.has_value()) {
                readSucceeded = true;
                break;
            }
        }

        if ((not readSucceeded) or (readResult.value() == BadBlockMarker)) {
            if (auto markResult = markBadBlock(block, lun); not markResult.has_value()) {
                return markResult;
            }
        }
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    if ((block >= BlocksPerLun) or (lun >= LunsPerCe)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    badBlockBitset[lun].set(block);

    return {};
}


/* ============= Write Protection ============= */

void MT29F::enableWrites() {
    if (nandWriteProtectPin != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtectPin, true);

        busyWaitNanoseconds(GpioSettleTimeNs);
    }
}

void MT29F::disableWrites() {
    if (nandWriteProtectPin != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtectPin, false);

        busyWaitNanoseconds(GpioSettleTimeNs);
    }
}


/* ============= Hardware Configuration ============= */

void MT29F::enableNandFlashMode(ChipSelect chipSelect) {
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

    const uint16_t storedCrc = static_cast<uint16_t>(parameterPage[StoredCrcLowByteOffset]) |
                               (static_cast<uint16_t>(parameterPage[StoredCrcHighByteOffset]) << BitsPerByte);

    return crc == storedCrc;
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

            const uint32_t readDataBytesPerPage = asUint32(parametersPageData[OnfiDataBytesPerPageOffset],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 1],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 2],
                                                           parametersPageData[OnfiDataBytesPerPageOffset + 3]);

            const uint16_t readSpareBytesPerPage = asUint16(parametersPageData[OnfiSpareBytesPerPageOffset],
                                                            parametersPageData[OnfiSpareBytesPerPageOffset + 1]);

            const uint32_t readPagesPerBlock = asUint32(parametersPageData[OnfiPagesPerBlockOffset],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 1],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 2],
                                                        parametersPageData[OnfiPagesPerBlockOffset + 3]);

            const uint32_t readBlocksPerLun = asUint32(parametersPageData[OnfiBlocksPerLunOffset],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 1],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 2],
                                                       parametersPageData[OnfiBlocksPerLunOffset + 3]);

            if (readDataBytesPerPage != DataBytesPerPage) {
                LOG_ERROR << "NAND: Geometry mismatch - DataBytesPerPage: expected " << DataBytesPerPage << ", got " << readDataBytesPerPage;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (readSpareBytesPerPage != SpareBytesPerPage) {
                LOG_ERROR << "NAND: Geometry mismatch - SpareBytesPerPage: expected " << SpareBytesPerPage << ", got " << readSpareBytesPerPage;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (readPagesPerBlock != PagesPerBlock) {
                LOG_ERROR << "NAND: Geometry mismatch - PagesPerBlock: expected " << static_cast<uint32_t>(PagesPerBlock) << ", got " << readPagesPerBlock;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }

            if (readBlocksPerLun != BlocksPerLun) {
                LOG_ERROR << "NAND: Geometry mismatch - BlocksPerLun: expected " << BlocksPerLun << ", got " << readBlocksPerLun;
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
    if (nandWriteProtectPin != PIO_PIN_NONE) {
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

    if (nandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {
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

__attribute__((noinline, section(".ramfunc")))
void MT29F::busyWaitCycles(uint32_t cycles) {
    constexpr uint32_t CyclesPerLoop = 2U;

    uint32_t iterations = cycles / CyclesPerLoop;

    if (iterations == 0) {
        return;
    }

    __asm__ volatile (
        ".align 4              \n"
        "1:  subs %0, %0, #1   \n"
        "    bne  1b           \n"
        : "+r" (iterations)
        :
        : "cc"
    );
}


/* ============= Public Interface - Initialization ============= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    if (isInitialized) {
        LOG_WARNING << "NAND: Already initialized, skipping";
        return {};
    }

    if (nandWriteProtectPin == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Write protection pin not provided. Hardware write protection disabled";
    }

    if (nandReadyBusyPin == PIO_PIN_NONE) {
        LOG_INFO << "NAND: Ready/busy pin not provided. Using status register polling";
    }

    if (auto resetResult = reset(); not resetResult.has_value()) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }

    etl::array<uint8_t, 5> deviceId;
    
    readDeviceID(deviceId);

    if (not etl::equal(ExpectedDeviceId.begin(), ExpectedDeviceId.end(), deviceId.begin())) {
        LOG_ERROR << "NAND: Device ID mismatch";
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

    constexpr etl::array<uint8_t, 4> expectedOnfi { 'O', 'N', 'F', 'I' };
    
    etl::array<uint8_t, 4> onfiSignature;

    readONFISignature(onfiSignature);

    if (not etl::equal(onfiSignature.begin(), onfiSignature.end(), expectedOnfi.begin())) {
        LOG_ERROR << "NAND: ONFI signature verification failed";
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

    if (auto writeEnabledResult = verifyWriteEnabled(); not writeEnabledResult.has_value()) {
        return writeEnabledResult;
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

    if (auto writeEnabledResult = verifyWriteEnabled(); not writeEnabledResult.has_value()) {
        return writeEnabledResult;
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

etl::expected<bool, NANDErrorCode> MT29F::isBlockBad(uint16_t block, uint8_t lun) const {
    if ((block >= BlocksPerLun) or (lun >= LunsPerCe)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    return badBlockBitset[lun].test(block);
}


/* ==================== Multi-Plane Operations ==================== */

etl::expected<void, NANDErrorCode> MT29F::eraseBlockMultiPlane(uint16_t block0, uint16_t block1, uint8_t lun) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((block0 >= BlocksPerLun) or (block1 >= BlocksPerLun) or (lun >= LunsPerCe)) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    if (getPlane(block0) == getPlane(block1)) {
        return etl::unexpected(NANDErrorCode::PLANE_MISMATCH);
    }

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    WriteEnableGuard guard(*this);

    if (auto writeEnabledResult = verifyWriteEnabled(); not writeEnabledResult.has_value()) {
        return writeEnabledResult;
    }

    const NANDAddress address0 { lun, block0, 0, 0 };
    const NANDAddress address1 { lun, block1, 0, 0 };

    AddressCycles cycles0;
    AddressCycles cycles1;

    buildAddressCycles(address0, cycles0);
    buildAddressCycles(address1, cycles1);

    sendCommand(Commands::ERASE_BLOCK);

    sendAddress(cycles0[AddressCycle::ROW_ADDRESS_1]);
    sendAddress(cycles0[AddressCycle::ROW_ADDRESS_2]);
    sendAddress(cycles0[AddressCycle::ROW_ADDRESS_3]);

    sendCommand(Commands::ERASE_MULTIPLANE_CONFIRM);

    sendCommand(Commands::ERASE_BLOCK);

    sendAddress(cycles1[AddressCycle::ROW_ADDRESS_1]);
    sendAddress(cycles1[AddressCycle::ROW_ADDRESS_2]);
    sendAddress(cycles1[AddressCycle::ROW_ADDRESS_3]);

    sendCommand(Commands::ERASE_BLOCK_CONFIRM);

    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutEraseUs); not waitResult.has_value()) {
        return waitResult;
    }

    if (hasOperationFailed(readStatusRegister())) {
        return etl::unexpected(NANDErrorCode::MULTIPLANE_FAILED);
    }

    return {};
}


/* ==================== Copyback Operations ==================== */

etl::expected<void, NANDErrorCode> MT29F::copyback(
    const NANDAddress& sourceAddress,
    const NANDAddress& destinationAddress) {

    if (getPlane(sourceAddress.block) != getPlane(destinationAddress.block)) {
        return etl::unexpected(NANDErrorCode::PLANE_MISMATCH);
    }

    if (auto readResult = copybackRead(sourceAddress); not readResult.has_value()) {
        return readResult;
    }

    return copybackProgram(destinationAddress);
}

etl::expected<void, NANDErrorCode> MT29F::copybackRead(const NANDAddress& sourceAddress) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (auto validateResult = validateAddress(sourceAddress); not validateResult.has_value()) {
        return validateResult;
    }

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    AddressCycles cycles;
    buildAddressCycles(sourceAddress, cycles);

    sendCommand(Commands::READ_MODE);

    for (const auto& cycle : cycles) {
        sendAddress(cycle);
    }

    sendCommand(Commands::COPYBACK_READ_CONFIRM);

    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutReadUs); not waitResult.has_value()) {
        return waitResult;
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::copybackProgram(const NANDAddress& destinationAddress) {
    if (not isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (auto validateResult = validateAddress(destinationAddress); not validateResult.has_value()) {
        return validateResult;
    }

    if (auto readyResult = ensureDeviceReady(); not readyResult.has_value()) {
        return readyResult;
    }

    WriteEnableGuard guard(*this);

    if (auto writeEnabledResult = verifyWriteEnabled(); not writeEnabledResult.has_value()) {
        return writeEnabledResult;
    }

    AddressCycles cycles;
    buildAddressCycles(destinationAddress, cycles);

    sendCommand(Commands::COPYBACK_PROGRAM);

    for (const auto& cycle : cycles) {
        sendAddress(cycle);
    }

    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);

    busyWaitNanoseconds(TwbNs);

    if (auto waitResult = waitForReady(TimeoutProgramUs); not waitResult.has_value()) {
        return waitResult;
    }

    if (hasOperationFailed(readStatusRegister())) {
        return etl::unexpected(NANDErrorCode::COPYBACK_FAILED);
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::copybackViaHost(
    const NANDAddress& sourceAddress,
    const NANDAddress& destinationAddress,
    etl::span<uint8_t> buffer) {

    if (buffer.size() < DataBytesPerPage) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    const NANDAddress readAddress { sourceAddress.lun, sourceAddress.block, sourceAddress.page, 0 };

    if (auto readResult = readPage(readAddress, buffer.first(DataBytesPerPage)); not readResult.has_value()) {
        return readResult;
    }

    const NANDAddress writeAddress { destinationAddress.lun, destinationAddress.block, destinationAddress.page, 0 };

    if (auto programResult = programPage(writeAddress, buffer.first(DataBytesPerPage)); not programResult.has_value()) {
        return programResult;
    }

    return {};
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
        case NANDErrorCode::COPYBACK_FAILED:
            return "Copyback operation failed";
        case NANDErrorCode::MULTIPLANE_FAILED:
            return "Multi-plane operation failed";
        case NANDErrorCode::PLANE_MISMATCH:
            return "Blocks not in different planes";
        default:
            return "Unknown error";
    }
}
