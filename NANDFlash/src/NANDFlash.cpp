#include "NANDFlash.h"
#include "BCH_ECC.h"
#include <cstring>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ================= Driver Initialization and Basic Info ================= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {

    auto resetResult = reset();
    if (!resetResult) {
        LOG_ERROR << "NAND: Reset failed";
        return resetResult;
    }
    
    etl::array<uint8_t, 5> deviceId;
    readDeviceID(deviceId);

    if (!std::equal(EXPECTED_DEVICE_ID.begin(), EXPECTED_DEVICE_ID.end(), deviceId.begin())) {
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    etl::array<uint8_t, 4> onfiSignature;
    readONFISignature(onfiSignature);

    const char* expectedOnfi = "ONFI";
    if (std::memcmp(onfiSignature.data(), expectedOnfi, 4) != 0) {
        LOG_WARNING << "NAND: Device is not ONFI compliant";
    }
    
    auto paramResult = validateDeviceParameters();
    if (!paramResult) {
        return paramResult;
    }
    
    disableWrites();

    isInitialized = true;

    auto scanResult = scanFactoryBadBlocks(0); // Scan LUN 0
    if (!scanResult) {
        return scanResult;
    }

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

void MT29F::readDeviceID(etl::span<uint8_t, 5> id) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::MANUFACTURER_ID));
    
    vTaskDelay(1); // Wait tWHR
    
    for (size_t i = 0; i < 5; ++i) {
        id[i] = readData();
    }
}

void MT29F::readONFISignature(etl::span<uint8_t, 4> signature) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::ONFI_SIGNATURE));
    
    vTaskDelay(1); // Wait tWHR
    
    for (size_t i = 0; i < 4; ++i) {
        signature[i] = readData();
    }
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
    
    // ONFI requires 3 redundant copies of parameter page. Try each copy until we find a valid one
    for (uint8_t copy = 0; copy < 3; copy++) {
        etl::array<uint8_t, 256> paramPageData;
    
        for (size_t i = 0; i < 256; ++i) {
            paramPageData[i] = readData();
        }
    
        if (validateParameterPageCRC(paramPageData)) {
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


etl::expected<void, NANDErrorCode> MT29F::readPage(const NANDAddress& addr, etl::span<uint8_t> data, bool performECC) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (performECC) {
        if (addr.column != 0 || data.size() != DATA_BYTES_PER_PAGE) {
            return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
        }
    } else {
        if (addr.column < DATA_BYTES_PER_PAGE) {
            if ((addr.column + data.size()) > DATA_BYTES_PER_PAGE) {
                return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
            }
        } else {
            uint32_t spareOffset = addr.column - DATA_BYTES_PER_PAGE;
            if ((spareOffset + data.size()) > SPARE_BYTES_PER_PAGE) {
                return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
            }
        }
    }
    
    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(60); // tWHR
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
        busyWaitNanoseconds(60); // tWHR
    }
    busyWaitNanoseconds(70); // tADL
    sendCommand(Commands::READ_CONFIRM);

    vTaskDelay(1); // Wait tWB
    
    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return waitResult;
    }

    // CRITICAL: waitForReady() calls readStatusRegister() which puts device in status mode
    // Send READ_MODE (0x00) with NO address cycles to return to data output mode
    // This resumes data output from the previously set column position
    sendCommand(Commands::READ_MODE);
    vTaskDelay(1); // Wait tWHR

    for (uint32_t i = 0; i < data.size(); ++i) {
        data[i] = readData();

        busyWaitNanoseconds(200); // tADL
    }

    vTaskDelay(1); // Wait tRHW

    if (performECC) {
        // Change read column to skip good block marker
        sendCommand(Commands::CHANGE_READ_COLUMN);
        sendAddress((DATA_BYTES_PER_PAGE + 1) & 0xFF);       
        sendAddress(((DATA_BYTES_PER_PAGE + 1) >> 8) & 0x3F); 
        sendCommand(Commands::CHANGE_READ_COLUMN_CONFIRM);

        vTaskDelay(1); // Wait tCCS

        etl::array<uint8_t, ECC_BYTES> eccCodes;
        for (size_t i = 0; i < ECC_BYTES; ++i) {
            eccCodes[i] = readData();
        }

        auto eccResult = validateAndCorrectECC(data, eccCodes);
        if (!eccResult) {
            return etl::unexpected(eccResult.error());
        }
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programPage(const NANDAddress& addr, etl::span<const uint8_t> data, bool addECC) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((addr.column + data.size() > DATA_BYTES_PER_PAGE) || (data.size() < DATA_BYTES_PER_PAGE) || (addr.column > 0)) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    enableWrites();

    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::PAGE_PROGRAM);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }

    vTaskDelay(1); // Wait tADL

    for (size_t i = 0; i < data.size(); ++i) {
        sendData(data[i]);
    }

    sendCommand(Commands::CHANGE_WRITE_COLUMN);
    sendAddress(DATA_BYTES_PER_PAGE & 0xFF);
    sendAddress((DATA_BYTES_PER_PAGE >> 8) & 0x3F);

    vTaskDelay(1); // Wait tCCS

    sendData(GOOD_BLOCK_MARKER);

    vTaskDelay(1); // Wait tCCS

    if (addECC) {
        etl::array<uint8_t, ECC_BYTES> eccData;
        auto eccResult = calculateECC(data, eccData);
        if (!eccResult) {
            return eccResult;
        }

        for (size_t i = 0; i < ECC_BYTES; ++i) {
            sendData(eccData[i]);
        }
    }

    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);

    vTaskDelay(1); // Wait tWB

    auto waitResult = waitForReady(TIMEOUT_PROGRAM_MS);
    if (!waitResult) {
        return waitResult;
    }

    status = readStatusRegister();

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
    
    if (block >= BLOCKS_PER_LUN || lun >= LUNS_PER_CE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    
    enableWrites();

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }
    
    NANDAddress addr(lun, block, 0, 0);
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);
    
    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles.cycle[2]);  // Row 1
    sendAddress(cycles.cycle[3]);  // Row 2  
    sendAddress(cycles.cycle[4]);  // Row 3
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);

    vTaskDelay(1); // Wait tWB
    
    auto waitResult = waitForReady(TIMEOUT_ERASE_MS);
    if (!waitResult) {
        return waitResult;
    }
    
    status = readStatusRegister();
    
    if ((status & STATUS_FAIL) != 0) {
        disableWrites();
        markBadBlock(block, lun);
        return etl::unexpected(NANDErrorCode::ERASE_FAILED);
    }
    
    disableWrites();
    
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
    
    if (nandReadyBusyPin != PIO_PIN_NONE) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {
            if ((xTaskGetTickCount() - startTime) > timeoutTicks) {
                return etl::unexpected(NANDErrorCode::TIMEOUT);
            }
            vTaskDelay(1);
        }
    }
    
    while (true) {
        auto status = readStatusRegister();
 
        if ((status & STATUS_RDY) != 0 && (status & STATUS_ARDY) != 0) {
            return {};
        }
        
        if ((xTaskGetTickCount() - startTime) > timeoutTicks) {
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }
        
        vTaskDelay(1);
    }
    // return {};
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


etl::expected<uint8_t, NANDErrorCode> MT29F::readBlockMarker(uint16_t block, uint8_t lun) {
    NANDAddress addr(lun, block, 0, DATA_BYTES_PER_PAGE);
    etl::array<uint8_t, 1> marker;

    auto readResult = readPage(addr, marker, false);
    if (!readResult) {
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
    
    
    return (crc == storedCrc);
}

/* ================== Public Bad Block Management API ================== */

bool MT29F::isBlockBad(uint16_t block, uint8_t lun) {
    bool isBad = false;

    for (size_t i = 0; i < badBlockCount; ++i) {
        if (badBlockTable[i].blockNumber == block && badBlockTable[i].lun == lun) {
            isBad = true;
        }
    }

    return isBad;
}

void MT29F::markBadBlock(uint16_t block, uint8_t lun) {
    badBlockTable[badBlockCount] = {block, lun};
    ++badBlockCount;
}

/* ================== Write Protection Management ======================= */

void MT29F::enableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 1);
    }

    vTaskDelay(1);
}

void MT29F::disableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 0);
    }

    vTaskDelay(1);
}

etl::expected<void, NANDErrorCode> MT29F::scanFactoryBadBlocks(uint8_t lun) {

    if (lun >= LUNS_PER_CE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    badBlockCount = 0;

    for (uint16_t block = 0; block < BLOCKS_PER_LUN; ++block) {

        if ((block % 5) == 0) {
            vTaskDelay(1);
        }

        auto markerResult = readBlockMarker(block, lun);
        bool isBlockBad = false;

        if (!markerResult) {
            // If we can't read the marker, assume block is bad for safety
            isBlockBad = true;
        } else if (markerResult.value() == BAD_BLOCK_MARKER) {
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

etl::expected<void, NANDErrorCode> MT29F::calculateECC(etl::span<const uint8_t> pageData,
                                                       etl::span<uint8_t> eccOutput) {
    if (eccOutput.size() != ECC_BYTES) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    if (pageData.size() != DATA_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto bchInit = BCH_ECC::initialize();
    if (!bchInit) {
        return etl::unexpected(NANDErrorCode::ECC_ALGORITHM_ERROR);
    }

    etl::array<uint8_t, BCH_ECC::DATA_BYTES_PER_CODEWORD> codewordBuffer;

    for (uint8_t codeword = 0; codeword < BCH_ECC::CODEWORDS_PER_PAGE; ++codeword) {
        const size_t dataOffset = codeword * BCH_ECC::DATA_BYTES_PER_CODEWORD;
        const size_t parityOffset = codeword * BCH_ECC::PARITY_BYTES_PER_CODEWORD;

        const size_t bytesAvailable = (dataOffset < pageData.size())
                                      ? ((dataOffset + BCH_ECC::DATA_BYTES_PER_CODEWORD <= pageData.size())
                                         ? BCH_ECC::DATA_BYTES_PER_CODEWORD
                                         : (pageData.size() - dataOffset))
                                      : 0;

        if (bytesAvailable == 0) {
            break;
        }

        std::memcpy(codewordBuffer.data(), pageData.data() + dataOffset, bytesAvailable);

        // Zero-pad if this is a partial codeword
        if (bytesAvailable < BCH_ECC::DATA_BYTES_PER_CODEWORD) {
            std::memset(codewordBuffer.data() + bytesAvailable, 0,
                        BCH_ECC::DATA_BYTES_PER_CODEWORD - bytesAvailable);
        }

        auto codewordParity = eccOutput.subspan(parityOffset, BCH_ECC::PARITY_BYTES_PER_CODEWORD);

        auto encodeResult = BCH_ECC::encode(etl::span<const uint8_t>(codewordBuffer), codewordParity);
        if (!encodeResult) {
            return etl::unexpected(NANDErrorCode::ECC_ALGORITHM_ERROR);
        }

        if ((codeword & 0x0F) == 0x0F) {
            vTaskDelay(1);
        }
    }

    return {};
}

etl::expected<uint8_t, NANDErrorCode> MT29F::validateAndCorrectECC(etl::span<uint8_t> pageData,
                                                                  etl::span<const uint8_t> eccCodes) {
    if (eccCodes.size() != ECC_BYTES) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    if (pageData.size() != DATA_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto bchInit = BCH_ECC::initialize();
    if (!bchInit) {
        return etl::unexpected(NANDErrorCode::ECC_ALGORITHM_ERROR);
    }

    uint8_t totalCorrectedErrors = 0;

    etl::array<uint8_t, BCH_ECC::DATA_BYTES_PER_CODEWORD> codewordBuffer;

    for (uint8_t codeword = 0; codeword < BCH_ECC::CODEWORDS_PER_PAGE; ++codeword) {
        const size_t dataOffset = codeword * BCH_ECC::DATA_BYTES_PER_CODEWORD;
        const size_t parityOffset = codeword * BCH_ECC::PARITY_BYTES_PER_CODEWORD;

        const size_t bytesAvailable = (dataOffset < pageData.size())
                                      ? ((dataOffset + BCH_ECC::DATA_BYTES_PER_CODEWORD <= pageData.size())
                                         ? BCH_ECC::DATA_BYTES_PER_CODEWORD
                                         : (pageData.size() - dataOffset))
                                      : 0;

        if (bytesAvailable == 0) {
            break;
        }

        std::memcpy(codewordBuffer.data(), pageData.data() + dataOffset, bytesAvailable);

        // Zero-pad if this is a partial codeword
        if (bytesAvailable < BCH_ECC::DATA_BYTES_PER_CODEWORD) {
            std::memset(codewordBuffer.data() + bytesAvailable, 0,
                        BCH_ECC::DATA_BYTES_PER_CODEWORD - bytesAvailable);
        }

        auto codewordParity = eccCodes.subspan(parityOffset, BCH_ECC::PARITY_BYTES_PER_CODEWORD);

        auto decodeResult = BCH_ECC::decode(etl::span<uint8_t>(codewordBuffer), codewordParity);
        if (!decodeResult) {
            if (decodeResult.error() == BCH_ECC::BCHError::TOO_MANY_ERRORS ||
                decodeResult.error() == BCH_ECC::BCHError::LOCATOR_ERROR) {
                return etl::unexpected(NANDErrorCode::ECC_UNCORRECTABLE);
            }
            return etl::unexpected(NANDErrorCode::ECC_ALGORITHM_ERROR);
        }

        std::memcpy(pageData.data() + dataOffset, codewordBuffer.data(), bytesAvailable);

        totalCorrectedErrors += decodeResult.value();

        if (totalCorrectedErrors > 255) {
            return etl::unexpected(NANDErrorCode::ECC_UNCORRECTABLE);
        }

        if ((codeword & 0x0F) == 0x0F) {
            vTaskDelay(1);
        }
    }

    return totalCorrectedErrors;
}
