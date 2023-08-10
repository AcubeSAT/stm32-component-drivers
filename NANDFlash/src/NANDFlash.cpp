#include <etl/array.h>
#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {
    sendCommand(RESET);
    sendCommand(READ_STATUS);
    if (waitDelay()) {
        return !NANDisReady;
    }
    return readData();
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

constexpr bool MT29F::isValidStructure(const MT29F::Structure *pos,const MT29F::AddressConfig op) {
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

//bool MT29F::readNAND(uint8_t data, uint8_t LUN, uint32_t position) {
//    const Address readAddress = setAddress(LUN, position);
//    sendCommand(READ_MODE);
//    sendAddress(readAddress.col1);
//    sendAddress(readAddress.col2);
//    sendAddress(readAddress.row1);
//    sendAddress(readAddress.row2);
//    sendAddress(readAddress.row3);
//    sendCommand(READ_CONFIRM);
//    if (waitDelay() == 0) {
//        return !NANDisReady;
//    }
//    data = readData();
//    return NANDisReady;
//}


bool MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    const uint8_t row1 = (block & 0x01) << 7;
    const uint8_t row2 = (block >> 1) & 0xff;
    const uint8_t row3 = ((block >> 9) & 0x07) | ((LUN & 0x01) << 3);
    sendCommand(ERASE_BLOCK);
    sendAddress(row1);
    sendAddress(row2);
    sendAddress(row3);
    sendCommand(ERASE_BLOCK_CONFIRM);
    return !detectArrayError();
}

bool MT29F::detectArrayError() {
    sendCommand(READ_STATUS);
    uint8_t status = readData();
    const uint32_t start = xTaskGetTickCount();
    while ((status & ArrayReadyMask) == 0) {
        status = readData();
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return NANDTimeout;
        }
    }
    return (status & 0x1);
}

bool MT29F::isNANDAlive() {
    etl::array<uint8_t, 8> id = {};
    const uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
    readNANDID(id);
    if (etl::equal(id.begin(), id.end(), valid_id)) {
        return true;
    } else return false;
}

bool MT29F::waitDelay() {
    const uint32_t start = xTaskGetTickCount();
    while ((PIO_PinRead(nandReadyBusyPin) == 0)) {
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return NANDTimeout;
        }
    }
    return false;
}

bool MT29F::errorHandler() {
    if (resetNAND() != 224) {
        if (!isNANDAlive()) {
            // TODO: Check if LCL is on and execute its task again if need be
            return !NANDisReady;
        }
    }
    return NANDisReady;
}

