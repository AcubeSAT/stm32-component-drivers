#include "NANDFlash.h"
#include <cstring>
#include <Logger.hpp>
#include "FreeRTOS.h"
#include "task.h"

/* ================= Driver Initialization and Basic Info ================= */

etl::expected<void, NANDErrorCode> MT29F::initialize() {
    // Create mutex (before any operations)
    mutex = xSemaphoreCreateMutexStatic(&mutexBuffer);
    if (mutex == nullptr) {
        LOG_ERROR << "NAND: Failed to create mutex";
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }

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
        return etl::unexpected(NANDErrorCode::HARDWARE_FAILURE);
    }
    
    auto paramResult = validateDeviceParameters();
    if (!paramResult) {
        return paramResult;
    }

    disableWrites();

    auto scanResult = scanFactoryBadBlocks(0);
    if (!scanResult) {
        return scanResult;
    }

    isInitialized = true;
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

    busyWaitNanoseconds(TWHR_NS);

    for (size_t i = 0; i < 5; ++i) {
        id[i] = readData();
    }

    busyWaitNanoseconds(TRHW_NS);
}

void MT29F::readONFISignature(etl::span<uint8_t, 4> signature) {
    sendCommand(Commands::READID);
    sendAddress(static_cast<uint8_t>(ReadIDAddress::ONFI_SIGNATURE));

    busyWaitNanoseconds(TWHR_NS);

    for (size_t i = 0; i < 4; ++i) {
        signature[i] = readData();
    }

    busyWaitNanoseconds(TRHW_NS);
}

etl::expected<void, NANDErrorCode> MT29F::validateDeviceParameters() {
    sendCommand(Commands::READ_PARAM_PAGE);
    sendAddress(0x00);

    busyWaitNanoseconds(TWHR_NS);

    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return etl::unexpected(waitResult.error());
    }

    busyWaitNanoseconds(TRR_NS);

    // after waitForReady(), device is in status mode
    // Send READ_MODE command to return to data output mode
    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);
    
    // ONFI requires 3 redundant copies of parameter page. Try each copy until we find a valid one
    for (uint8_t copy = 0; copy < 3; copy++) {
        etl::array<uint8_t, 256> paramPageData;

        for (size_t i = 0; i < 256; ++i) {
            paramPageData[i] = readData();
        }

        if (validateParameterPageCRC(paramPageData)) {
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

etl::expected<void, NANDErrorCode> MT29F::executeReadCommandSequence(const NANDAddress& addr) {
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);

    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }

    sendCommand(Commands::READ_CONFIRM);
    busyWaitNanoseconds(TADL_NS);

    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return waitResult;
    }

    busyWaitNanoseconds(TRR_NS);
    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::readPage(const NANDAddress& addr, etl::span<uint8_t> data) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if ((addr.column + data.size()) > TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }

    MutexGuard lock(mutex);

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);

    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }
    sendCommand(Commands::READ_CONFIRM);

    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_READ_MS);
    if (!waitResult) {
        return waitResult;
    }

    busyWaitNanoseconds(TRR_NS);

    // waitForReady() calls readStatusRegister() which puts device in status mode
    // Send READ_MODE (0x00) with NO address cycles to return to data output mode
    sendCommand(Commands::READ_MODE);
    busyWaitNanoseconds(TWHR_NS);

    for (uint32_t i = 0; i < data.size(); ++i) {
        data[i] = readData();
    }

    busyWaitNanoseconds(TRHW_NS);

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::programPage(const NANDAddress& addr, etl::span<const uint8_t> data) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (addr.column + data.size() > TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::INVALID_PARAMETER);
    }

    auto validateResult = validateAddress(addr);
    if (!validateResult) {
        return validateResult;
    }

    MutexGuard lock(mutex);

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this); 

    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::PAGE_PROGRAM);
    for (int i = 0; i < 5; ++i) {
        sendAddress(cycles.cycle[i]);
    }

    busyWaitNanoseconds(TADL_NS);

    for (size_t i = 0; i < data.size(); ++i) {
        sendData(data[i]);
    }

    sendCommand(Commands::CHANGE_WRITE_COLUMN);
    sendAddress(DATA_BYTES_PER_PAGE & 0xFF);
    sendAddress((DATA_BYTES_PER_PAGE >> 8) & 0x3F);

    busyWaitNanoseconds(TCCS_NS);

    sendData(GOOD_BLOCK_MARKER);

    sendCommand(Commands::PAGE_PROGRAM_CONFIRM);

    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_PROGRAM_MS);
    if (!waitResult) {
        return waitResult;
    }

    status = readStatusRegister();

    if ((status & STATUS_FAIL) != 0) {
        return etl::unexpected(NANDErrorCode::PROGRAM_FAILED);
    }

    return {};
}

etl::expected<void, NANDErrorCode> MT29F::eraseBlock(uint16_t block, uint8_t lun) {
    if (!isInitialized) {
        return etl::unexpected(NANDErrorCode::NOT_INITIALIZED);
    }

    if (block >= BLOCKS_PER_LUN || lun >= LUNS_PER_CE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }

    MutexGuard lock(mutex);

    auto status = readStatusRegister();
    if ((status & STATUS_RDY) == 0 || (status & STATUS_ARDY) == 0) {
        return etl::unexpected(NANDErrorCode::BUSY_ARRAY);
    }

    WriteEnableGuard guard(*this);

    NANDAddress addr(lun, block, 0, 0);
    AddressCycles cycles;
    buildAddressCycles(addr, cycles);

    sendCommand(Commands::ERASE_BLOCK);
    sendAddress(cycles.cycle[2]);
    sendAddress(cycles.cycle[3]);
    sendAddress(cycles.cycle[4]);
    sendCommand(Commands::ERASE_BLOCK_CONFIRM);

    busyWaitNanoseconds(TWB_NS);

    auto waitResult = waitForReady(TIMEOUT_ERASE_MS);
    if (!waitResult) {
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
    const TickType_t busyWaitEndTick = startTick + 1; // 1 tick is 1ms

    while (true) {
        bool rbPinReady = (nandReadyBusyPin == PIO_PIN_NONE) || (PIO_PinRead(nandReadyBusyPin) != 0);

        if (rbPinReady) {
            auto status = readStatusRegister();
            if ((status & STATUS_RDY) != 0 && (status & STATUS_ARDY) != 0) {
                return {};  // Ready
            }
        }

        if ((xTaskGetTickCount() - startTick) >= timeoutTicks) {
            return etl::unexpected(NANDErrorCode::TIMEOUT);
        }

        if (xTaskGetTickCount() >= busyWaitEndTick) {
            vTaskDelay(1);  // After ~1ms delay via the scheduler
        }
        // Before ~1ms continue immediately (busy-wait)
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
    if (addr.lun >= LUNS_PER_CE ||
        addr.block >= BLOCKS_PER_LUN ||
        addr.page >= PAGES_PER_BLOCK ||
        addr.column >= TOTAL_BYTES_PER_PAGE) {
        return etl::unexpected(NANDErrorCode::ADDRESS_OUT_OF_BOUNDS);
    }
    return {};
}


etl::expected<uint8_t, NANDErrorCode> MT29F::readBlockMarker(uint16_t block, uint8_t lun) {
    // Based on the datasheet the bad block marker is stored in the first page only
    // in byte 0 of the spare area
    NANDAddress addr(lun, block, 0, BLOCK_MARKER_OFFSET);

    auto cmdResult = executeReadCommandSequence(addr);
    if (!cmdResult) {
        return etl::unexpected(cmdResult.error());
    }


    return readData();
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
    
    uint16_t storedCrc = static_cast<uint16_t>(paramPage[254]) | (static_cast<uint16_t>(paramPage[255]) << 8);
    
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
    if (badBlockCount >= MAX_BAD_BLOCKS) {
        // For now it's ok just returning. It will be reworked with the integration of littlefs.
        return; 
    }

    badBlockTable[badBlockCount] = {block, lun};
    ++badBlockCount;
}

/* ================== Write Protection Management ======================= */

void MT29F::enableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 1);
        busyWaitNanoseconds(GPIO_SETTLE_TIME_NS);  // GPIO settling time
    }
}

void MT29F::disableWrites() {
    if (nandWriteProtect != PIO_PIN_NONE) {
        PIO_PinWrite(nandWriteProtect, 0);
        busyWaitNanoseconds(GPIO_SETTLE_TIME_NS);  // Ensure WP# propagates
    }
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
