#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {
    if (PIO_PinRead(nandReadyBusyPin)) {
        sendCommand(RESET);
        sendCommand(READ_STATUS);
        return readData();
    }
}


MT29F::Address MT29F::setAddress(uint8_t LUN, uint32_t position) {
    uint8_t page = position / PageSizeBytes;
    uint16_t column = position - page * PageSizeBytes;
    uint16_t block = position / BlockSizeBytes;

    Address address;
    address.col1 = column;
    address.col2 = column >> 8;
    address.row1 = page | block << 7;
    address.row2 = block >> 1;
    address.row3 = block >> 9 | LUN << 3;

    return address;
}


uint8_t *MT29F::readNANDID() {
    static uint8_t id[8] = {};
    sendCommand(READID);
    sendAddress(READ_MODE);
    for (int i = 0; i < 8; i++) {
        id[i] = readData();
    }
    return id;
}


void MT29F::writeNAND(uint8_t LUN, uint32_t position, uint8_t data) {
    PIO_PinWrite(nandWriteProtect,1);
    Address writeAddress = setAddress(LUN, position);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    sendData(data);
    sendCommand(PAGE_PROGRAM_CONFIRM);
}

uint8_t MT29F::readNAND(uint8_t LUN, uint32_t position) {
    uint8_t data = 0;
    Address readAddress = setAddress(LUN, position);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    for (volatile int i = 0; i < 10; i++)
        data = readData();
    return data;
}

void MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    PIO_PinWrite(nandWriteProtect,1);
    uint8_t row1 = block << 7;
    uint8_t row2 = block >> 1;
    uint8_t row3 = block >> 9 | LUN << 3;
    sendCommand(ERASE_BLOCK);
    sendAddress(row1);
    sendAddress(row2);
    sendAddress(row3);
    sendCommand(ERASE_BLOCK_CONFIRM);
}