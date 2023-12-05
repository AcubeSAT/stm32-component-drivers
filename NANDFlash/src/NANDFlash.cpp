#include "etl/array.h"
#include "etl/bit.h"
#include "etl/bitset.h"
#include "NANDFlash.hpp"


uint8_t MT29F::resetNAND() {
    NAND_Reset();
}

void MT29F::readNANDID(etl::array<uint8_t, 8>& id) {
    NAND_Read_ID_ONFI(id.data());
}


uint8_t MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    const uint8_t row1 = (block & 0x01) << 7;
    const uint8_t row2 = (block >> 1) & 0xff;
    const uint8_t row3 = ((block >> 9) & 0x07) | ((LUN & 0x01) << 3);

    nand_addr_t addr;
    addr.column = 0;
    addr.page = 0;
    addr.block = block;

    uint8_t result = NAND_Block_Erase(addr);

    return result;
}

bool MT29F::isNANDAlive() {
    flash_width status = NAND_Read_Status();

    if (status == DRIVER_STATUS_NOT_INITIALIZED) {
        return false;
    }

    if (status & !STATUS_FAIL) {
        etl::array<uint8_t, 8> id = {};
        readNANDID(id);

        const uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};

        return etl::equal(id.begin(), id.end(), valid_id);
    }

    return false;
}

uint8_t MT29F::waitDelay() {
    const uint32_t start = xTaskGetTickCount();
    while ((PIO_PinRead(nandReadyBusyPin) == 0)) {
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return NAND_TIMEOUT;
        }
    }
    return 0;
}

uint8_t MT29F::errorHandler() {
    if (resetNAND() != 224) {
        if (!isNANDAlive()) {
            // TODO: Check if LCL is on and execute its task again if need be
            return NAND_UNSUPPORTED;
        }
    }
    return 0;
}

etl::array<uint8_t, MT29F::WriteChunkSize> MT29F::detectCorrectECCError(
        etl::span<uint8_t, (WriteChunkSize + NumECCBytes)> dataECC, etl::array<uint8_t, NumECCBytes> newECC) {
    etl::array<uint8_t, NumECCBytes> storedECC;
    etl::span<uint8_t> eccSpan = dataECC.subspan(WriteChunkSize, NumECCBytes);
    etl::copy(eccSpan.begin(), eccSpan.end(), storedECC.begin());

    if (etl::equal(storedECC.begin(), storedECC.end(), newECC.begin()))
        return MT29F::dataChunker<WriteChunkSize>(dataECC, 0);

    uint8_t xorECC[NumECCBytes];
    for (size_t i = 0; i < NumECCBytes; i++) {
        xorECC[i] = storedECC[i] ^ newECC[i];
    }

    uint16_t xorECCBytes = xorECC[0] ^ xorECC[1] ^ xorECC[2];
    uint16_t setBits = etl::popcount(xorECCBytes);

    if (setBits == 12) {
        uint8_t bitAddress = 0;
        bitAddress |= ((xorECC[2] & 0x08) | (xorECC[2] & 0x020) | (xorECC[2] & 0x80));
        // Byte address = (LP17,LP15,LP13,LP11,LP9,LP7,LP5,LP3,LP1)
        uint16_t byteAddress = 0;
        for (size_t eccByte = 0; eccByte < 2; eccByte++) {
            for (size_t offset = 0x01; offset <= 0x80; offset += 0x02) {
                byteAddress |= (xorECC[eccByte] & offset);
            }
        }
        byteAddress |= (xorECC[2] & 0x01);

        dataECC[byteAddress] ^= (0x01 << bitAddress);
        return MT29F::dataChunker<WriteChunkSize>(dataECC, 0);
    } else if (setBits == 1) return {}; // TODO: replace with ECC_ERROR code
    else return {}; // TODO: replace with UNCORRECTABLE_ERROR
}








uint8_t MT29F::writeNAND(MT29F::Structure *pos, MT29F::AddressConfig op, etl::span<uint8_t> data, uint64_t size) {
    const uint8_t quotient = size / WriteChunkSize;
    const uint8_t dataChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
    if ((dataChunks * (WriteChunkSize + NumECCBytes)) > MaxDataBytesPerPage) return -1;
    if (!isValidStructure(pos, op)) return -1;
    if (op == POS || op == POS_PAGE) {
        const uint32_t page = pos->position / PageSizeBytes;
        const uint16_t column = pos->position - page * PageSizeBytes;
        if ((column + (dataChunks * (WriteChunkSize + NumECCBytes))) > PageSizeBytes) return -1;
    }

    etl::span<uint8_t> dataECC;
    for (size_t chunk = 0; chunk < dataChunks; chunk++) {
        uint64_t offsetData = (chunk * WriteChunkSize);
        etl::move((data.begin() + offsetData), (data.begin() + offsetData + WriteChunkSize),
                  (dataECC.begin() + (chunk * (WriteChunkSize + NumECCBytes))));
        etl::array<uint8_t, WriteChunkSize> arrayChunkConversion;
        uint64_t offsetDataECC = (chunk * (WriteChunkSize + NumECCBytes));
        etl::move((dataECC.begin() + offsetDataECC),
                  (dataECC.begin() + offsetDataECC + WriteChunkSize),
                  arrayChunkConversion.begin());
        etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(arrayChunkConversion);
        etl::move(genECC.begin(), genECC.end(), (dataECC.begin() + offsetData));
    }

    const Address writeAddress = setAddress(pos, op);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    for (uint16_t i = 0; i < (dataChunks * (WriteChunkSize + NumECCBytes)); i++) {
        if (waitDelay() == NAND_TIMEOUT) {
            return NAND_TIMEOUT;
        }
        sendData(dataECC[i]);
    }
    sendCommand(PAGE_PROGRAM_CONFIRM);
    return detectArrayError();
}

uint8_t MT29F::readNAND(MT29F::Structure *pos, MT29F::AddressConfig op, etl::span<uint8_t> data, uint64_t size) {
    const uint8_t quotient = size / WriteChunkSize;
    const uint8_t dataChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
    if ((dataChunks * (WriteChunkSize + NumECCBytes)) > MaxDataBytesPerPage) return -1;
    if (!isValidStructure(pos, op)) return -1;
    if (op == POS || op == POS_PAGE) {
        uint32_t page = pos->position / PageSizeBytes;
        uint16_t column = pos->position - page * PageSizeBytes;
        if ((column + (dataChunks * (WriteChunkSize + NumECCBytes))) > PageSizeBytes) return -1;
    }
    etl::span<uint8_t> dataECC = {};

    const Address readAddress = setAddress(pos, op);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    for (uint16_t i = 0; i < (dataChunks * (WriteChunkSize + NumECCBytes)); i++) {
        if (waitDelay() == NAND_TIMEOUT) {
            return NAND_TIMEOUT;
        }
        dataECC[i] = readData();
    }

    for (size_t chunk = 0; chunk < dataChunks; chunk++) {
        etl::span<uint8_t, (WriteChunkSize + NumECCBytes)> thisChunkWithECC = {};
        etl::move((dataECC.begin() + (chunk * (WriteChunkSize + NumECCBytes))),
                  (dataECC.begin() + ((chunk + 1) * (WriteChunkSize + NumECCBytes))), thisChunkWithECC.begin());
        etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                dataChunker(thisChunkWithECC, 0));
        etl::array<uint8_t, WriteChunkSize> returnedData = detectCorrectECCError(thisChunkWithECC,
                                                                                 genECC);       // TODO: use etl::expected
        if (returnedData == etl::array<uint8_t, WriteChunkSize>{}) {
            return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
        } else etl::move(returnedData.begin(), returnedData.end(), (data.begin() + (chunk * WriteChunkSize)));
    }
    return 0;
}