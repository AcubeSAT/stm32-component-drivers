#include "NANDFlash.h"
#include <cstring>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ================= Driver Initialization and Basic Info ================= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    // LOG_DEBUG << "NAND: Starting reset";
    auto resetResult = reset();
    if (!resetResult) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }
    // LOG_DEBUG << "NAND: Reset completed successfully";
    
    // LOG_DEBUG << "NAND: Reading device ID";
    etl::array<uint8_t, 5> deviceId;
    auto idResult = readDeviceID(deviceId);
    if (!idResult) {
        LOG_ERROR << "NAND: Failed to read device ID";
        return idResult;
    }
    // LOG_DEBUG << "NAND: Device ID read successfully";
    
    if (!std::equal(EXPECTED_DEVICE_ID.begin(), EXPECTED_DEVICE_ID.end(), deviceId.begin())) {
        LOG_ERROR << "NAND: Unexpected device ID: " << static_cast<uint32_t>(deviceId[0]) << " "
                  << static_cast<uint32_t>(deviceId[1]) << " " << static_cast<uint32_t>(deviceId[2])
                  << " " << static_cast<uint32_t>(deviceId[3]) << " " << static_cast<uint32_t>(deviceId[4]);
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    // LOG_DEBUG << "NAND: Reading ONFI signature";
    etl::array<uint8_t, 4> onfiSignature;
    auto onfiResult = readONFISignature(onfiSignature);
    if (!onfiResult) {
        LOG_ERROR << "NAND: Failed to read ONFI signature";
        return onfiResult;
    }
    // LOG_DEBUG << "NAND: ONFI signature read successfully";
    
    const char* expectedOnfi = "ONFI";
    if (std::memcmp(onfiSignature.data(), expectedOnfi, 4) != 0) {
        LOG_WARNING << "NAND: Device is not ONFI compliant";
    }
    
    LOG_DEBUG << "NAND: Validating device parameters";
    auto paramResult = validateDeviceParameters();
    if (!paramResult) {
        LOG_ERROR << "NAND: Device parameter validation failed";
        return paramResult;
    }
    // LOG_DEBUG << "NAND: Device parameters validated successfully";
    
    disableWrites();
    
    isInitialized = true;

    LOG_DEBUG << "NAND initialized successfully";
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::reset() {
    sendCommand(Commands::RESET);
    
    auto waitResult = waitForReady(TIMEOUT_RESET_MS);
    if (!waitResult) {
        return waitResult;
    }
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readDeviceID(etl::span<uint8_t, 5> id) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::MANUFACTURER_ID));
    
    vTaskDelay(1); // Wait tWHR
    
    for (size_t i = 0; i < 5; ++i) {
        id[i] = readData();
    }
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readONFISignature(etl::span<uint8_t, 4> signature) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::ONFI_SIGNATURE));
    
    vTaskDelay(1); // Wait tWHR
    
    for (size_t i = 0; i < 4; ++i) {
        signature[i] = readData();
    }
    
    return {};
}


etl::expected<void, NANDErrorCode> MT29F::validateDeviceParameters() {
    sendCommand(Commands::READ_PARAM_PAGE);
    sendAddress(0x00);  // Parameter page address
    
    vTaskDelay(1); // Wait tWHR
    
    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return etl::unexpected(waitResult.error());
    }
    
    // CRITICAL: After waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR
    
    /* ONFI requires 3 redundant copies of parameter page. Try each copy until we find a valid one */
    for (uint8_t copy = 0; copy < 3; copy++) {
        etl::array<uint8_t, 256> paramPageData;
    
        for (size_t i = 0; i < 256; ++i) {
            paramPageData[i] = readData();
        }
    
        if (validateParameterPageCRC(paramPageData)) {
            // LOG_DEBUG << "NAND: Valid parameter page found (copy " << copy << "), validating geometry";
    
            // Validate ONFI signature
            if (std::memcmp(paramPageData.data(), "ONFI", 4) != 0) {
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
    
            // Extract geometry values from parameter page (little-endian)
            auto asUint16 = [](uint8_t low, uint8_t high) -> uint16_t {
                return low | (static_cast<uint16_t>(high) << 8);
            };
    
            auto asUint32 = [](uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) -> uint32_t {
                return b0 | (static_cast<uint32_t>(b1) << 8) |
                       (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 24);
            };
    
            uint32_t readDataBytesPerPage = asUint32(paramPageData[80], paramPageData[81], paramPageData[82], paramPageData[83]);
            uint16_t readSpareBytesPerPage = asUint16(paramPageData[84], paramPageData[85]);
            uint32_t readPagesPerBlock = asUint32(paramPageData[92], paramPageData[93], paramPageData[94], paramPageData[95]);
            uint32_t readBlocksPerLUN = asUint32(paramPageData[96], paramPageData[97], paramPageData[98], paramPageData[99]);
    
            // Validate against hardcoded constants
            if (readDataBytesPerPage != DATA_BYTES_PER_PAGE) {
                LOG_ERROR << "NAND: Data bytes per page mismatch: expected " << DATA_BYTES_PER_PAGE << ", got " << readDataBytesPerPage;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readSpareBytesPerPage != SPARE_BYTES_PER_PAGE) {
                LOG_ERROR << "NAND: Spare bytes per page mismatch: expected " << SPARE_BYTES_PER_PAGE << ", got " << readSpareBytesPerPage;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readPagesPerBlock != PAGES_PER_BLOCK) {
                LOG_ERROR << "NAND: Pages per block mismatch: expected " << PAGES_PER_BLOCK << ", got " << readPagesPerBlock;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
            if (readBlocksPerLUN != BLOCKS_PER_LUN) {
                LOG_ERROR << "NAND: Blocks per LUN mismatch: expected " << BLOCKS_PER_LUN << ", got " << readBlocksPerLUN;
                return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
            }
    
            LOG_DEBUG << "NAND: Device geometry validated successfully";
            // Parameter page goes out of scope here - stack space reclaimed!
            return {};
        } else {
            LOG_WARNING << "NAND: Parameter page copy " << copy << " has invalid CRC";
        }
    }
    
    LOG_ERROR << "NAND: All parameter page copies have invalid CRC";
    return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
    // return {};
}


/* ===================== Data Operations ===================== */

etl::expected<void, NANDErrorCode> MT29F::readPage(const NANDAddress& addr, etl::span<uint8_t> data, uint32_t length) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }
    
    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }
    
    if (length > DATA_BYTES_PER_PAGE || length > data.size()) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
    
    // auto badBlockResult = checkBlockBad(addr.block, addr.lun);
    // if (!badBlockResult) {
    //     return etl::unexpected(badBlockResult.error());
    // }
    // if (badBlockResult.value()) {
    //     return etl::unexpected(NANDErrorCode::BAD_BLOCK);
    // }
    
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);
    
    sendCommand(Commands::READ_MODE);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }
    sendCommand(Commands::READ_CONFIRM);
    
    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return waitResult;
    }
    
    auto status = readStatusRegister();

    if ((status & STATUS_FAILC) != 0) {
        return etl::unexpected(NANDErrorCode::READ_FAILED);
    }

    // CRITICAL: After waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR
    
    for (uint32_t i = 0; i < length; ++i) {
        data[i] = readData();
    }
    
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readSpare(const NANDAddress& addr, etl::span<uint8_t> data, uint32_t length) {
    // if (!isInitialized) {
    //     return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    // }
    
    if (length > SPARE_BYTES_PER_PAGE || length > data.size()) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
    
    NANDAddress spareAddr = addr;
    spareAddr.column = DATA_BYTES_PER_PAGE;  // Spare area starts after main data
    
    auto validateResult = validateAddress(spareAddr);
    if (!validateResult) {
        return validateResult;
    }
    
    AddressCycles cycles;
    buildAddressCycles(spareAddr, cycles);
    
    sendCommand(Commands::READ_MODE);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }
    sendCommand(Commands::READ_CONFIRM);
    
    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return waitResult;
    }

    auto status = readStatusRegister();

    if ((status & STATUS_FAILC) != 0) {
        return etl::unexpected(NANDErrorCode::READ_FAILED);
    }
    
    // CRITICAL: After waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR

    for (uint32_t i = 0; i < length; ++i) {
        data[i] = readData();
    }
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programPage(const NANDAddress& addr, etl::span<const uint8_t> data) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }
    
    // if (isWriteProtected()) {
    //     return etl::unexpected(NANDErrorCode::WRITE_PROTECTED);
    // }
    
    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }
    
    if (data.size() > DATA_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
        
    enableWrites();
    
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);
    
    sendCommand(Commands::PAGE_PROGRAM);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }
    
    for (size_t i = 0; i < data.size(); ++i) {
        sendData(data[i]);
    }
    
    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);
    
    auto waitResult = waitForReady(TIMEOUT_PROGRAM_MS);
    if (!waitResult) {
        disableWrites();
        return waitResult;
    }
    
    auto status = readStatusRegister();
    
    if ((status & STATUS_FAIL) != 0) {
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }
    
    disableWrites();
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programSpare(const NANDAddress& addr, etl::span<const uint8_t> data) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }
    
    // if (isWriteProtected()) {
    //     return etl::unexpected(NANDErrorCode::WRITE_PROTECTED);
    // }
    
    if (data.size() > SPARE_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
    
    NANDAddress spareAddr = addr;
    spareAddr.column = DATA_BYTES_PER_PAGE;
    
    auto validateResult = validateAddress(spareAddr);
    if (!validateResult) {
        return validateResult;
    }
    
    enableWrites();
    
    AddressCycles cycles;
    buildAddressCycles(spareAddr, cycles);
    
    sendCommand(Commands::PAGE_PROGRAM);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }
    
    for (size_t i = 0; i < data.size(); ++i) {
        sendData(data[i]);
    }
    
    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);
    
    auto waitResult = waitForReady(TIMEOUT_PROGRAM_MS);
    if (!waitResult) {
        disableWrites();
        return waitResult;
    }
    
    auto status = readStatusRegister();
    
    if ((status & STATUS_FAIL) != 0) {
        disableWrites();
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }
    
    disableWrites();
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::eraseBlock(uint16_t block, uint8_t lun) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }
    
    // if (isWriteProtected()) {
    //     return etl::unexpected(NANDErrorCode::WRITE_PROTECTED);
    // }
    
    if (block >= BLOCKS_PER_LUN || lun >= LUNS_PER_CE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    
    
    enableWrites();
    
    NANDAddress addr(lun, block, 0, 0);
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);
    
    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles.cycle[2]);  // Row 1
    sendAddress(cycles.cycle[3]);  // Row 2  
    sendAddress(cycles.cycle[4]);  // Row 3
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);
    
    auto waitResult = waitForReady(TIMEOUT_ERASE_MS);
    if (!waitResult) {
        disableWrites();
        return waitResult;
    }
    
    auto status = readStatusRegister();
    
    if ((status & STATUS_FAIL) != 0) {
        disableWrites();
        return etl::unexpected(NANDErrorCode::ERASE_FAILED);
    }
    
    disableWrites();
    
    return {};
}

/* ============================ Bad Block Management ============================ */

bool MT29F::checkBlockBad(uint16_t block, uint8_t lun) {
    // for (size_t i = 0; i < badBlockCount; ++i) {
    //     if (badBlockTable[i].blockNumber == block && badBlockTable[i].lun == lun) {
    //         return true;
    //     }
    // }

    return false;
}

etl::expected<void, NANDErrorCode> MT29F::markBlockBad(uint16_t block, uint8_t lun) {
    // if (badBlockCount >= MAX_BAD_BLOCKS) {
    //     return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    // }
    //
    // badBlockTable[badBlockCount] = {block, lun};
    // ++badBlockCount;
    
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
    const uint32_t startTime = xTaskGetTickCount();
    const uint32_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);
    
   // LOG_DEBUG << "NAND: waitForReady() called with timeout " << timeoutMs << "ms (" << timeoutTicks << " ticks)";
    
    if (nandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {
            if ((xTaskGetTickCount() - startTime) > timeoutTicks) {
                LOG_ERROR << "NAND: Hardware ready pin timeout";
                return etl::unexpected(NANDErrorCode::TIMEOUT);
            }
            vTaskDelay(1);
        }
        // LOG_DEBUG << "NAND: Hardware ready pin signaled ready";
    }
    
    while (true) {
        auto status = readStatusRegister();
 
        if ((status & STATUS_RDY) != 0 && (status & STATUS_ARDY) != 0) {
            // LOG_DEBUG << "NAND: Device ready after " << statusChecks << " status checks, "
            //           << (xTaskGetTickCount() - startTime) << " ticks elapsed";
            return {};
        }
        
        if ((xTaskGetTickCount() - startTime) > timeoutTicks) {
            // LOG_ERROR << "NAND: Status register timeout after " << statusChecks << " checks, status=0x" 
            //           << std::hex << static_cast<uint32_t>(status);
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }
        
        vTaskDelay(1);
    }
}

uint8_t MT29F::readStatusRegister() {
    sendCommand(Commands::READ_STATUS);
    
    vTaskDelay(1); // Wait tWHR
    
    return readData();
}

etl::expected<void, NANDErrorCode> MT29F::validateAddress(const NANDAddress& addr) {
    if (addr.lun >= LUNS_PER_CE ||
        addr.block >= BLOCKS_PER_LUN ||
        addr.page >= PAGES_PER_BLOCK ||
        addr.column >= TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    return {};
}


etl::expected<uint8_t, NANDErrorCode> MT29F::readBadBlockMarker(uint16_t block, uint8_t lun) {
    NANDAddress addr(lun, block, 0, DATA_BYTES_PER_PAGE);
    
    // Validate address calculation
    if (addr.lun >= LUNS_PER_CE || addr.block >= BLOCKS_PER_LUN) {
        LOG_ERROR << "NAND: Invalid address for block " << block << " LUN " << lun;
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
        
    etl::array<uint8_t, 4> marker; // Read 4 bytes instead of 1 for comparison
    // marker.fill(0xAA); // Initialize with known pattern
    
    etl::span<uint8_t> markerSpan(marker);
    auto readResult = readSpare(addr, markerSpan, 4);
    if (!readResult) {
        LOG_ERROR << "NAND: readSpare failed for block " << block << " LUN " << lun;
        return etl::unexpected(readResult.error());
    }
    
    return marker[0];
}

bool MT29F::validateParameterPageCRC(const etl::array<uint8_t, 256>& paramPage) {
    constexpr uint16_t CRC_POLYNOMIAL = 0x8005;
    uint16_t crc = 0x4F4E;
    
    for (size_t i = 0; i < 254; ++i) {
        crc ^= static_cast<uint16_t>(paramPage[i]) << 8;
        
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    
    uint16_t storedCrc = static_cast<uint16_t>(paramPage[254]) | 
                        (static_cast<uint16_t>(paramPage[255]) << 8);
    
    bool isValid = (crc == storedCrc);
    if (!isValid) {
        LOG_DEBUG << "NAND: CRC mismatch - calculated: 0x" << std::hex << crc 
                  << ", stored: 0x" << std::hex << storedCrc;
        LOG_DEBUG << "First few bytes: " << std::hex << static_cast<uint32_t>(paramPage[0]) 
                  << " " << static_cast<uint32_t>(paramPage[1]) 
                  << " " << static_cast<uint32_t>(paramPage[2]) 
                  << " " << static_cast<uint32_t>(paramPage[3]);
    }
    
    return isValid;
}

/* ================== Public Bad Block Management API ================== */

etl::expected<bool, NANDErrorCode> MT29F::isBadBlock(uint16_t block, uint8_t lun) {
    return checkBlockBad(block, lun);
}

etl::expected<void, NANDErrorCode> MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    return markBlockBad(block, lun);
}

/* ================== Write Protection Management ======================= */

void MT29F::enableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 1);
    }
}

void MT29F::disableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 0);
    }
}

bool MT29F::isWriteProtected() {
    bool hwWriteProtected = false;
    bool statusWriteProtected = false;
    
    if (nandWriteProtect != PIO_PIN_NONE) {
        hwWriteProtected = (PIO_PinRead(nandWriteProtect) == 0);
    }
    
    auto status = readStatusRegister();

    /* STATUS_WP bit is 1 when NOT write protected */
    statusWriteProtected = ((status & STATUS_WP) == 0);
    
    
    return hwWriteProtected || statusWriteProtected;
}


etl::expected<void, NANDErrorCode> MT29F::markBlockGood(uint16_t block, uint8_t lun) {
    NANDAddress addr(lun, block, 0, DATA_BYTES_PER_PAGE);
    etl::array<uint8_t, 1> goodMarker = {GOOD_BLOCK_MARKER};
    
    auto programResult = programSpare(addr, etl::span<const uint8_t>(goodMarker));
    if (!programResult) {
        LOG_WARNING << "NAND: Failed to write good block marker for block " << block;
        return programResult;
    }
    
    return {};
}
