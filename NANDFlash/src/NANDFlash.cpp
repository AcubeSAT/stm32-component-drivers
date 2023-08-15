#include "etl/array.h"
#include "etl/bit.h"
#include "etl/bitset.h"
#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {
    sendCommand(RESET);
    sendCommand(READ_STATUS);
    return waitDelay() == NAND_TIMEOUT ? NAND_TIMEOUT : readData();
}


MT29F::Address MT29F::setAddress(MT29F::Structure *pos, MT29F::AddressConfig mode) {
    uint16_t column = pos->position;
    switch (mode) {
        case POS:
            column -= pos->page * PageSizeBytes;
            pos->page = pos->position / PageSizeBytes;
        case POS_PAGE:
            pos->block = pos->position / BlockSizeBytes;
            break;
        case PAGE:
            column = 0;
            pos->block = pos->page / NumPagesBlock;
            pos->page = pos->page - pos->block * NumPagesBlock;
    }


    Address address;
    address.col1 = column & 0xff;
    address.col2 = (column & 0x3f00) >> 8;
    address.row1 = (pos->page & 0x7f) | ((pos->block & 0x01) << 7);
    address.row2 = (pos->block >> 1) & 0xff;
    address.row3 = ((pos->block >> 9) & 0x07) | ((pos->LUN & 0x01) << 3);

    return address;
}

constexpr bool MT29F::isValidStructure(MT29F::Structure *pos, MT29F::AddressConfig op) {
    if (pos->LUN > 1) return false;
    switch (op) {
        case POS:
            if (pos->position >= ((MaxNumPage * PageSizeBytes) - (MaxChunksPerPage * NumECCBytes))) return false;
            break;
        case POS_PAGE:
            if (pos->position >= MaxDataBytesPerPage) return false;
            break;
        case PAGE:
            if (pos->page >= MaxNumPage) return false;
            break;
        case PAGE_BLOCK:
            if (pos->page >= NumPagesBlock || pos->block >= MaxNumBlock) return false;
    }
    return true;
}

void MT29F::readNANDID(etl::array<uint8_t, 8> id) {
    static_assert(id.size() == 8, "NAND's ID array is not 8 digits");
    sendCommand(READID);
    sendAddress(READ_MODE);
    for (uint8_t i = 0; i < 8; i++) {
        id[i] = readData();
    }
}


uint8_t MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    const uint8_t row1 = (block & 0x01) << 7;
    const uint8_t row2 = (block >> 1) & 0xff;
    const uint8_t row3 = ((block >> 9) & 0x07) | ((LUN & 0x01) << 3);
    sendCommand(ERASE_BLOCK);
    sendAddress(row1);
    sendAddress(row2);
    sendAddress(row3);
    sendCommand(ERASE_BLOCK_CONFIRM);
    return detectArrayError();
}

uint8_t MT29F::detectArrayError() {
    sendCommand(READ_STATUS);
    uint8_t status = readData();
    const uint32_t start = xTaskGetTickCount();
    while ((status & ArrayReadyMask) == 0) {
        status = readData();
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return NAND_TIMEOUT;
        }
    }
    return (status & 0x1) ? NAND_FAIL_OP : 0;
}

bool MT29F::isNANDAlive() {
    etl::array<uint8_t, 8> id = {};
    const uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
    readNANDID(id);
    return etl::equal(id.begin(), id.end(), valid_id);
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
            return NAND_NOT_ALIVE;
        }
    }
    return 0;
}

template<>
etl::array<uint8_t, MT29F::WriteChunkSize>
MT29F::dataChunker<MT29F::WriteChunkSize>(etl::span<uint8_t, WriteChunkSize> data, uint32_t startPos);


etl::array<uint8_t, MT29F::NumECCBytes>
MT29F::generateECCBytes(etl::array<uint8_t, MT29F::WriteChunkSize> data) {
    ECCBits eccBits{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < WriteChunkSize; i++) {
        if (i & 0x01) {
            eccBits.LP1 = (etl::popcount(data[i]) % 2) ^ eccBits.LP1;
        } else {
            eccBits.LP0 = (etl::popcount(data[i]) % 2) ^ eccBits.LP0;
        }
        if (i & 0x02) {
            eccBits.LP3 = (etl::popcount(data[i]) % 2) ^ eccBits.LP3;
        } else {
            eccBits.LP2 = (etl::popcount(data[i]) % 2) ^ eccBits.LP2;
        }
        if (i & 0x04) {
            eccBits.LP5 = (etl::popcount(data[i]) % 2) ^ eccBits.LP5;
        } else {
            eccBits.LP4 = (etl::popcount(data[i]) % 2) ^ eccBits.LP4;
        }
        if (i & 0x08) {
            eccBits.LP7 = (etl::popcount(data[i]) % 2) ^ eccBits.LP7;
        } else {
            eccBits.LP6 = (etl::popcount(data[i]) % 2) ^ eccBits.LP6;
        }
        if (i & 0x10) {
            eccBits.LP9 = (etl::popcount(data[i]) % 2) ^ eccBits.LP9;
        } else {
            eccBits.LP8 = (etl::popcount(data[i]) % 2) ^ eccBits.LP8;
        }
        if (i & 0x20) {
            eccBits.LP11 = (etl::popcount(data[i]) % 2) ^ eccBits.LP11;
        } else {
            eccBits.LP10 = (etl::popcount(data[i]) % 2) ^ eccBits.LP10;
        }
        if (i & 0x40) {
            eccBits.LP13 = (etl::popcount(data[i]) % 2) ^ eccBits.LP13;
        } else {
            eccBits.LP12 = (etl::popcount(data[i]) % 2) ^ eccBits.LP12;
        }
        if (i & 0x80) {
            eccBits.LP15 = (etl::popcount(data[i]) % 2) ^ eccBits.LP15;
        } else {
            eccBits.LP14 = (etl::popcount(data[i]) % 2) ^ eccBits.LP14;
        }
        if (i & 0x100) {
            eccBits.LP17 = (etl::popcount(data[i]) % 2) ^ eccBits.LP17;
        } else {
            eccBits.LP16 = (etl::popcount(data[i]) % 2) ^ eccBits.LP16;
        }
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column7 ^= (data[i] & 0x80);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column6 ^= (data[i] & 0x40);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column5 ^= (data[i] & 0x20);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column4 ^= (data[i] & 0x10);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column3 ^= (data[i] & 0x08);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column2 ^= (data[i] & 0x04);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column1 ^= (data[i] & 0x02);
    }
    for (size_t i = 0; i < WriteChunkSize; i++) {
        eccBits.column0 ^= (data[i] & 0x01);
    }

    eccBits.CP0 ^= (eccBits.column6 ^ eccBits.column4 ^ eccBits.column2 ^ eccBits.column0);
    eccBits.CP1 ^= (eccBits.column7 ^ eccBits.column5 ^ eccBits.column3 ^ eccBits.column1);
    eccBits.CP2 ^= (eccBits.column5 ^ eccBits.column4 ^ eccBits.column1 ^ eccBits.column0);
    eccBits.CP3 ^= (eccBits.column7 ^ eccBits.column6 ^ eccBits.column3 ^ eccBits.column2);
    eccBits.CP4 ^= (eccBits.column3 ^ eccBits.column2 ^ eccBits.column1 ^ eccBits.column0);
    eccBits.CP5 ^= (eccBits.column7 ^ eccBits.column6 ^ eccBits.column5 ^ eccBits.column4);

    etl::array<uint8_t, NumECCBytes> eccBytes = {};
    eccBytes[0] = (eccBits.LP7 << 7 | eccBits.LP6 << 6 | eccBits.LP5 << 5 | eccBits.LP4 << 4 | eccBits.LP3 << 3 |
                   eccBits.LP2 << 2 | eccBits.LP1 << 1 | eccBits.LP0);
    eccBytes[1] = (eccBits.LP15 << 7 | eccBits.LP14 << 6 | eccBits.LP13 << 5 | eccBits.LP12 << 4 | eccBits.LP11 << 3 |
                   eccBits.LP10 << 2 | eccBits.LP9 << 1 | eccBits.LP8);
    eccBytes[2] = (eccBits.CP5 << 7 | eccBits.CP4 << 6 | eccBits.CP3 << 5 | eccBits.CP2 << 4 | eccBits.CP1 << 3 |
                   eccBits.CP0 << 2 | eccBits.LP17 << 1 | eccBits.LP16);
    return eccBytes;
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

uint8_t MT29F::abstractWriteNAND(MT29F::Structure *pos, MT29F::AddressConfig op, etl::span<const uint8_t> data,
                                 uint64_t size) {
    if (!isValidStructure(pos, op)) return -1;
    const uint32_t quotient = size / WriteChunkSize;
    const uint32_t writeChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
    const uint32_t pagesToWrite = writeChunks / MaxChunksPerPage;

//        etl::array<uint8_t, size> readData = {};
//        readNAND<size, pos, op>(readData);
//
//        if(etl::find()){
//
//        }

    if (op == PAGE || op == PAGE_BLOCK) {
        for (size_t page = 0; page < pagesToWrite; page++) {
            for (size_t chunk = 0; chunk < MaxChunksPerPage; chunk++) {

                etl::array<uint8_t, WriteChunkSize> dataChunk = {};
                uint64_t offset = (WriteChunkSize * (chunk + (page * MaxChunksPerPage)));
                etl::move((data.begin() + offset), (data.begin() + offset + WriteChunkSize), dataChunk.begin());
                etl::array<uint8_t, NumECCBytes>
                        genECC = generateECCBytes(dataChunk);
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                uint8_t result = writeNAND(pos, op, dataToWrite,
                                           (WriteChunkSize + NumECCBytes));    //TODO: use etl::expected
                if (result != 0) return result;
            }
        }
        const uint8_t chunksLeft = writeChunks - (MaxChunksPerPage * pagesToWrite);
        if ((pos->page + 1) != NumPagesBlock) {
            ++pos->page;
            // TODO: add read to see if that page is written already up to numberOfAddresses + NumECCBytes
        } else if ((pos->block + 1) != MaxNumBlock) {
            ++pos->block;
        } else pos->LUN = pos->LUN == 0 ? 1 : 0;

        for (size_t i = 0; i < chunksLeft; i++) {
            etl::array<uint8_t, WriteChunkSize> dataChunk = {};
            uint64_t offset = WriteChunkSize * (i + (pagesToWrite * MaxChunksPerPage));
            etl::move((data.begin() + offset), (data.begin() + offset + WriteChunkSize), dataChunk.begin());
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
            etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
            etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
            etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
            uint8_t result = writeNAND(pos, op, dataToWrite, (WriteChunkSize + NumECCBytes));
            if (result != 0) return result;
        }
    } else {
        pos->page = (op == POS) ? (pos->position / PageSizeBytes) : pos->page;
        pos->block = (op == POS) ? (pos->page / NumPagesBlock) : pos->block;
        const uint16_t column = pos->position - (pos->page * PageSizeBytes);
        const uint8_t chunksFitThisPage = (PageSizeBytes - column) / (WriteChunkSize + NumECCBytes);
        for (size_t i = 0; i < chunksFitThisPage; i++) {
            etl::array<uint8_t, WriteChunkSize> dataChunk = {};
            uint64_t offset = WriteChunkSize * i;
            etl::move((data.begin() + offset), (data.begin() + offset + WriteChunkSize), dataChunk.begin());
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
            etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
            etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
            etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
            uint8_t result = writeNAND(pos, op, dataToWrite, (WriteChunkSize + NumECCBytes));
            if (result != 0) return result;
            if (i != (chunksFitThisPage - 1)) {
                pos->position += (WriteChunkSize + NumECCBytes);
            } else break;
        }
        pos->position = 0;
        if (chunksFitThisPage < writeChunks) {
            const uint32_t pagesToWriteLeft =
                    pagesToWrite - ((writeChunks - chunksFitThisPage) / MaxChunksPerPage);
            for (size_t page = 0; page < pagesToWriteLeft; page++) {
                if ((pos->page + 1) != NumPagesBlock) {
                    ++pos->page;
                    // TODO: add read to see if that page is written already up to numberOfAddresses + NumECCBytes
                } else if ((pos->block + 1) != MaxNumBlock) {
                    ++pos->block;
                } else pos->LUN = pos->LUN == 0 ? 1 : 0;
                uint32_t chunksFitTillThisPage = chunksFitThisPage + (page * MaxChunksPerPage);
                for (size_t chunk = 0;
                     (chunk < (writeChunks - chunksFitTillThisPage) ||
                      (chunk < MaxChunksPerPage)); chunk++) {
                    etl::array<uint8_t, WriteChunkSize> dataChunk = {};
                    uint64_t offset = (WriteChunkSize * (chunk + chunksFitTillThisPage));
                    etl::move((data.begin() + offset), (data.begin() + offset + WriteChunkSize), dataChunk.begin());
                    etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
                    etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                    etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                    etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                    uint8_t result = writeNAND(pos, PAGE_BLOCK, dataToWrite, size);
                    if (result != 0) return result;
                }
            }

        }

    }
    return 0;

}

template<>
etl::array<uint8_t, MT29F::WriteChunkSize>
MT29F::dataChunker<(MT29F::WriteChunkSize + MT29F::NumECCBytes)>(
        etl::span<uint8_t, (MT29F::WriteChunkSize + MT29F::NumECCBytes)>, uint32_t startPos);

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

uint8_t
MT29F::abstractReadNAND(MT29F::Structure *pos, MT29F::AddressConfig op, etl::span<uint8_t> data, uint64_t size) {
    if (!isValidStructure(pos, op)) return -1;
    const uint32_t quotient = size / WriteChunkSize;
    const uint32_t readChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
    const uint32_t pagesToRead = readChunks / MaxChunksPerPage;

    if (op == PAGE || op == PAGE_BLOCK) {
        for (size_t page = 0; page < pagesToRead; page++) {
            for (size_t chunk = 0; chunk < MaxChunksPerPage; chunk++) {
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                uint8_t result = readNAND(pos, op, dataToRead, (WriteChunkSize + NumECCBytes));
                if (result != 0) return result;
                etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                        dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                    return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                }
                etl::move(finalData.begin(), finalData.end(),
                          data.begin() + (WriteChunkSize * (chunk + (page * MaxChunksPerPage))));
            }
            if ((pos->page + 1) != NumPagesBlock) {
                ++pos->page;
            } else if ((pos->block + 1) != MaxNumBlock) {
                ++pos->block;
            } else pos->LUN = pos->LUN == 0 ? 1 : 0;
        }
        const uint8_t chunksLeft = readChunks - (MaxChunksPerPage * pagesToRead);
        for (size_t i = 0; i < chunksLeft; i++) {
            etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
            uint8_t result = readNAND(pos, op, dataToRead, (WriteChunkSize + NumECCBytes));
            if (result != 0) return result;
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                    dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
            etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
            if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
            }
            etl::move(finalData.begin(), finalData.end(),
                      data.begin() + (WriteChunkSize * (i + (pagesToRead * MaxChunksPerPage))));
        }
    } else {
        pos->page = (op == POS) ? (pos->position / PageSizeBytes) : pos->page;
        pos->block = (op == POS) ? (pos->page / NumPagesBlock) : pos->block;
        const uint16_t column = pos->position - (pos->page * PageSizeBytes);
        const uint8_t chunksFitThisPage = (PageSizeBytes - column) / (WriteChunkSize + NumECCBytes);
        for (size_t i = 0; i < chunksFitThisPage; i++) {
            etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
            uint8_t result = readNAND(pos, op, dataToRead, (WriteChunkSize + NumECCBytes));
            if (result != 0) return result;
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                    dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
            etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
            if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
            }
            etl::move(finalData.begin(), finalData.end(),
                      data.begin() + (WriteChunkSize * i));
            if (i != (chunksFitThisPage - 1)) {
                pos->position += (WriteChunkSize + NumECCBytes);
            } else break;
        }
        pos->position = 0;
        if (chunksFitThisPage < readChunks) {
            const uint32_t pagesToWriteLeft =
                    pagesToRead - ((readChunks - chunksFitThisPage) / MaxChunksPerPage);
            for (size_t page = 0; page < pagesToWriteLeft; page++) {
                if ((pos->page + 1) != NumPagesBlock) {
                    ++pos->page;
                } else if ((pos->block + 1) != MaxNumBlock) {
                    ++pos->block;
                } else pos->LUN = pos->LUN == 0 ? 1 : 0;
                uint32_t chunksFitTillThisPage = chunksFitThisPage + (page * MaxChunksPerPage);
                for (size_t chunk = 0;
                     (chunk < (readChunks - chunksFitTillThisPage) ||
                      (chunk < MaxChunksPerPage)); chunk++) {
                    etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                    uint8_t result = readNAND(pos, PAGE_BLOCK, dataToRead, (WriteChunkSize + NumECCBytes));
                    if (result != 0) return result;
                    etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                            dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                    etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                    if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                        return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                    }
                    etl::move(finalData.begin(), finalData.end(),
                              data.begin() + (WriteChunkSize *
                                              (chunk + chunksFitTillThisPage)));
                }
            }

        }

    }
    return 0;
}





