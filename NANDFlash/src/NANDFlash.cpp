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

constexpr bool MT29F::isValidStructure(const MT29F::Structure *pos, const MT29F::AddressConfig op) {
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

