#include "NANDFlash.h"
#include <cstring>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ================= Driver Initialization and Basic Info ================= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    LOG_DEBUG << "NAND: Starting reset";
    auto resetResult = reset();
    if (!resetResult) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }
    LOG_DEBUG << "NAND: Reset completed successfully";
    
    LOG_DEBUG << "NAND: Reading device ID";
    etl::array<uint8_t, 5> deviceId;
    auto idResult = readDeviceID(deviceId);
    if (!idResult) {
        LOG_ERROR << "NAND: Failed to read device ID";
        return idResult;
    }
    LOG_DEBUG << "NAND: Device ID read successfully";
    
    if (!std::equal(EXPECTED_DEVICE_ID.begin(), EXPECTED_DEVICE_ID.end(), deviceId.begin())) {
        LOG_ERROR << "NAND: Unexpected device ID: " << static_cast<uint32_t>(deviceId[0]) << " "
                  << static_cast<uint32_t>(deviceId[1]) << " " << static_cast<uint32_t>(deviceId[2])
                  << " " << static_cast<uint32_t>(deviceId[3]) << " " << static_cast<uint32_t>(deviceId[4]);
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    LOG_DEBUG << "NAND: Reading ONFI signature";
    etl::array<uint8_t, 4> onfiSignature;
    auto onfiResult = readONFISignature(onfiSignature);
    if (!onfiResult) {
        LOG_ERROR << "NAND: Failed to read ONFI signature";
        return onfiResult;
    }
    LOG_DEBUG << "NAND: ONFI signature read successfully";
    
    const char* expectedOnfi = "ONFI";
    if (std::memcmp(onfiSignature.data(), expectedOnfi, 4) != 0) {
        LOG_WARNING << "NAND: Device is not ONFI compliant";
        deviceInfo.isONFICompliant = false;
    } else {
        deviceInfo.isONFICompliant = true;
    }
    
    auto paramResult = readParameterPage();
    if (!paramResult) {
        return etl::unexpected(paramResult.error());
    }
    deviceInfo = paramResult.value();
    
    auto scanResult = scanFactoryBadBlocks();
    if (!scanResult) {
        LOG_WARNING << "NAND: Failed to scan factory bad blocks: " << toString(scanResult.error());
        /* This is not critical in order to failt the initialization */
    }
    
    disableWrites();
    
    isInitialized = true;

    LOG_DEBUG << "NAND initialized successfully. Bad blocks: " << badBlockCount;
    
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

etl::expected<NANDDeviceInfo, NANDErrorCode> MT29F::readParameterPage() {
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
            LOG_INFO << "NAND: Valid parameter page found (copy " << copy << ")";
            return parseParameterPage(etl::span<const uint8_t, 256>(paramPageData));
        } else {
            LOG_WARNING << "NAND: Parameter page copy " << copy << " has invalid CRC";
        }
    }
    
    LOG_ERROR << "NAND: All parameter page copies have invalid CRC";

    return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
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
    
    auto badBlockResult = checkBlockBad(addr.block, addr.lun);
    if (!badBlockResult) {
        return etl::unexpected(badBlockResult.error());
    }
    if (badBlockResult.value()) {
        return etl::unexpected(NANDErrorCode::BAD_BLOCK);
    }
    
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
    
    // CRITICAL: After waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR
    
    for (uint32_t i = 0; i < length; ++i) {
        data[i] = readData();
    }
    
    // For read operations, only check read-specific error flags
    // Don't check write protection as it's irrelevant for reads
    auto statusResult = readStatusRegister();
    if (!statusResult) {
        return etl::unexpected(statusResult.error());
    }
    
    uint8_t status = statusResult.value();
    
    if ((status & STATUS_FAILC) != 0) {
        LOG_ERROR << "NAND: Read operation failed (STATUS_FAILC set)";
        return etl::unexpected(NANDErrorCode::READ_FAILED);
    }
    
    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readSpare(const NANDAddress& addr, etl::span<uint8_t>& data, uint32_t length) {
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
    
    // CRITICAL: After waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t readValue = readData();
        data[i] = readValue;
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
    
    auto badBlockResult = checkBlockBad(addr.block, addr.lun);
    if (!badBlockResult) {
        return etl::unexpected(badBlockResult.error());
    }
    if (badBlockResult.value()) {
        return etl::unexpected(NANDErrorCode::BAD_BLOCK);
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
    
    auto statusResult = checkOperationStatus();
    
    disableWrites();
    
    if (!statusResult) {
        /* If writing to flash failed then the block is bad */
        if (statusResult.error() == NANDErrorCode::PROGRAM_FAILED) {
            markBlockBad(addr.block, addr.lun, false);
        }
        return statusResult;
    }
    
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
    
    auto statusResult = checkOperationStatus();
    
    disableWrites();
    
    if (!statusResult) {
        return statusResult;
    }
    
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
    
    auto badBlockResult = checkBlockBad(block, lun);
    if (!badBlockResult) {
        return etl::unexpected(badBlockResult.error());
    }
    if (badBlockResult.value()) {
        return etl::unexpected(NANDErrorCode::BAD_BLOCK);
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
    
    auto statusResult = checkOperationStatus();
    
    disableWrites();
    
    if (!statusResult) {
        if (statusResult.error() == NANDErrorCode::ERASE_FAILED) {
            markBlockBad(block, lun, false);
        }
        return statusResult;
    }
    
    return {};
}

/* ============================ Bad Block Management ============================ */

etl::expected<bool, NANDErrorCode> MT29F::checkBlockBad(uint16_t block, uint8_t lun) {
    for (size_t i = 0; i < badBlockCount; ++i) {
        if (badBlockTable[i].blockNumber == block && badBlockTable[i].lun == lun) {
            return true;
        }
    }

    auto markerResult = readBadBlockMarker(block, lun);
    if (!markerResult) {
        return etl::unexpected(markerResult.error());
    }
    
    return markerResult.value() != GOOD_BLOCK_MARKER;
}

etl::expected<void, NANDErrorCode> MT29F::markBlockBad(uint16_t block, uint8_t lun, bool isFactoryBad) {
    if (badBlockCount >= MAX_BAD_BLOCKS) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
    
    for (size_t i = 0; i < badBlockCount; ++i) {
        if (badBlockTable[i].blockNumber == block && badBlockTable[i].lun == lun) {
            return {};
        }
    }
    
    badBlockTable[badBlockCount] = {block, lun, isFactoryBad, 1};
    ++badBlockCount;
    
    NANDAddress addr(lun, block, 0, DATA_BYTES_PER_PAGE);
    etl::array<uint8_t, 1> badMarker = {BAD_BLOCK_MARKER};
    
    auto programResult = programSpare(addr, etl::span<const uint8_t>(badMarker));
    if (!programResult) {
        LOG_WARNING << "NAND: Failed to write bad block marker for block " << block;
        /* No need to return error as block is already marked in the table */
    }
    
    LOG_WARNING << "NAND: Marked block " << block << " (LUN " << lun << ") as bad";

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
    
    // uint32_t statusChecks = 0;
    while (true) {
        auto statusResult = readStatusRegister();
        if (!statusResult) {
            LOG_ERROR << "NAND: Failed to read status register";
            return etl::unexpected(statusResult.error());
        }
        
        uint8_t status = statusResult.value();
        // statusChecks++;
        
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

etl::expected<uint8_t, NANDErrorCode> MT29F::readStatusRegister() {
    sendCommand(Commands::READ_STATUS);
    
    vTaskDelay(1); // Wait tWHR
    
    return readData();
}

etl::expected<void, NANDErrorCode> MT29F::checkOperationStatus() {
    auto statusResult = readStatusRegister();
    if (!statusResult) {
        return etl::unexpected(statusResult.error());
    }
    
    uint8_t status = statusResult.value();
    
    LOG_DEBUG << "NAND: Status register = 0x" << std::hex << static_cast<uint32_t>(status) 
              << " (FAIL=" << ((status & STATUS_FAIL) ? "1" : "0")
              << ", FAILC=" << ((status & STATUS_FAILC) ? "1" : "0") 
              << ", WP=" << ((status & STATUS_WP) ? "1" : "0") << ")";
    
    if ((status & STATUS_FAIL) != 0) {
        LOG_ERROR << "NAND: STATUS_FAIL bit set";
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }
    
    if ((status & STATUS_FAILC) != 0) {
        LOG_ERROR << "NAND: STATUS_FAILC bit set"; 
        return etl::unexpected(NANDErrorCode::READ_FAILED);
    }
    
    if ((status & STATUS_WP) == 0) {
        LOG_ERROR << "NAND: STATUS_WP bit clear (write protected)";
        return etl::unexpected(NANDErrorCode::WRITE_PROTECTED);
    }
    
    return {};
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

etl::expected<void, NANDErrorCode> MT29F::scanFactoryBadBlocks() {
    badBlockCount = 0;
    
    for (uint16_t block = 0; block < BLOCKS_PER_LUN; ++block) {
        if (block % 16 == 0 && block > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        for (uint8_t lun = 0; lun < LUNS_PER_CE; ++lun) {
            auto markerResult = readBadBlockMarker(block, lun);
            if (!markerResult) {
                continue;
            }
            
            if (markerResult.value() != GOOD_BLOCK_MARKER) {
                if (badBlockCount < MAX_BAD_BLOCKS) {
                    badBlockTable[badBlockCount] = {block, lun, true, 0};
                    ++badBlockCount;
                    LOG_INFO << "NAND: Found factory bad block " << block << " (LUN " << lun << ")";
                }
            }
        }
    }
    
    LOG_INFO << "NAND: Found " << badBlockCount << " factory bad blocks";
    return {};
}

etl::expected<uint8_t, NANDErrorCode> MT29F::readBadBlockMarker(uint16_t block, uint8_t lun) {
    NANDAddress addr(lun, block, 0, DATA_BYTES_PER_PAGE);
    etl::array<uint8_t, 1> marker;
    marker[0] = 0xAA; // Debug: Initialize with known value
    
    etl::span<uint8_t> markerSpan(marker);
    auto readResult = readSpare(addr, markerSpan, 1);
    if (!readResult) {
        return etl::unexpected(readResult.error());
    }
    
    // Debug: Check value immediately after readSpare
    uint8_t debugValue = marker[0];
    return debugValue;
}

etl::expected<NANDDeviceInfo, NANDErrorCode> MT29F::parseParameterPage(const etl::span<const uint8_t, 256>& paramPage) {
    NANDDeviceInfo info{};
    
    if (std::memcmp(paramPage.data(), "ONFI", 4) != 0) {
        return etl::unexpected(NANDErrorCode::BAD_PARAMETER_PAGE);
    }
    
    /* The parameter page is in little-endian and we need to store it correctly */
    auto asUint16 = [](uint8_t low, uint8_t high) -> uint16_t {
        return low | (static_cast<uint16_t>(high) << 8);
    };
    
    auto asUint32 = [](uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) -> uint32_t {
        return b0 | (static_cast<uint32_t>(b1) << 8) | 
               (static_cast<uint32_t>(b2) << 16) | (static_cast<uint32_t>(b3) << 24);
    };
    
    info.revisionNumber = asUint16(paramPage[4], paramPage[5]);
    std::memcpy(info.manufacturer, &paramPage[32], 12);
    info.manufacturer[12] = '\0';
    std::memcpy(info.model, &paramPage[44], 20);
    info.model[20] = '\0';
    info.jedecId = paramPage[64];
    info.dataBytesPerPage = asUint32(paramPage[80], paramPage[81], paramPage[82], paramPage[83]);
    info.spareBytesPerPage = asUint16(paramPage[84], paramPage[85]);
    info.pagesPerBlock = asUint32(paramPage[92], paramPage[93], paramPage[94], paramPage[95]);
    info.blocksPerLUN = asUint32(paramPage[96], paramPage[97], paramPage[98], paramPage[99]);
    info.lunsPerCE = paramPage[100];
    info.maxBadBlocksPerLUN = asUint16(paramPage[103], paramPage[104]);
    info.blockEndurance = asUint16(paramPage[105], paramPage[106]);
    info.eccBits = paramPage[112];
    info.isONFICompliant = true;
    
    return info;
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
    return markBlockBad(block, lun, false);
}

etl::expected<void, NANDErrorCode> MT29F::initializeBlocks(uint16_t startBlock, uint16_t endBlock) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }
    
    if (startBlock > endBlock || endBlock >= BLOCKS_PER_LUN) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }
    
    LOG_INFO << "NAND: Initializing blocks " << startBlock << " to " << endBlock;
    
    enableWrites();
    
    uint16_t successCount = 0;
    uint16_t failCount = 0;
    
    for (uint16_t block = startBlock; block <= endBlock; ++block) {
        // Build address for block erase (LUN 0, block, page 0, column 0)
        NANDAddress addr(0, block, 0, 0);
        AddressCycles cycles;
        buildAddressCycles(addr, cycles);
        
        // Send erase command sequence
        sendCommand(Commands::ERASE_BLOCK);
        sendAddress(cycles.cycle[2]);  // Row 1
        sendAddress(cycles.cycle[3]);  // Row 2  
        sendAddress(cycles.cycle[4]);  // Row 3
        sendCommand(Commands::ERASE_BLOCK_CONFIRM);
        
        // Wait for operation to complete
        auto waitResult = waitForReady(TIMEOUT_ERASE_MS);
        if (!waitResult) {
            disableWrites();
            LOG_ERROR << "NAND: Timeout during block " << block << " initialization";
            return waitResult;
        }
        
        // Check if erase operation succeeded
        auto statusResult = checkOperationStatus();
        if (!statusResult) {
            // Erase failed - this is a genuinely bad block
            LOG_WARNING << "NAND: Block " << block << " failed erase during initialization - marking as bad";
            auto markResult = markBlockBad(block, 0, false);  // Not factory bad, but runtime detected
            if (!markResult) {
                LOG_ERROR << "NAND: Failed to mark block " << block << " as bad";
            }
            failCount++;
        } else {
            LOG_DEBUG << "NAND: Successfully initialized block " << block;
            successCount++;
        }
        
        // Yield every 8 blocks to prevent watchdog timeout
        if (block % 8 == 0 && block > startBlock) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    disableWrites();
    
    LOG_INFO << "NAND: Block initialization complete - " << successCount << " successful, " 
             << failCount << " failed (marked as bad)";
    
    return {};
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
    
    auto statusResult = readStatusRegister();
    if (statusResult) {
        /* STATUS_WP bit is 1 when NOT write protected */
        statusWriteProtected = ((statusResult.value() & STATUS_WP) == 0);
    }
    
    return hwWriteProtected || statusWriteProtected;
}
