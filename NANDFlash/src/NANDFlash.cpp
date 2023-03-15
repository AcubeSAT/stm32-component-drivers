#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {
    if (PIO_PinRead(nandReadyBusyPin)) {
        sendCommand(RESET);
        sendCommand(READ_STATUS);
        while (PIO_PinRead(nandReadyBusyPin) == 0) {}
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
    address.row1 = page | (block << 7);
    address.row2 = block >> 1;
    address.row3 = (block >> 9) | (LUN << 3);

    return address;
}


void MT29F::readNANDID(uint8_t *id) {
    sendCommand(READID);
    sendCommand(READ_MODE);
    for (int i = 0; i < 8; i++) {
        id[i] = readData();
    }
}


void MT29F::writeNAND(uint8_t LUN, uint32_t position, uint8_t data) {
    PIO_PinWrite(nandWriteProtect, 1);
    Address writeAddress = setAddress(LUN, position);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    sendData(data);
    sendCommand(PAGE_PROGRAM_CONFIRM);
    if (detectErrorArray()) {
        resetNAND();
        writeNAND(LUN, position, data);
    }
}

void MT29F::writeNAND(uint8_t LUN, uint32_t position, uint8_t *data) {
    uint8_t numberOfAddresses = sizeof(data) / sizeof(uint8_t);
    PIO_PinWrite(nandWriteProtect, 1);
    Address writeAddress = setAddress(LUN, position);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    for (int i = 0; i < numberOfAddresses; i++) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {}
        sendData(data[i]);
    }
    sendCommand(PAGE_PROGRAM_CONFIRM);
    if (detectErrorArray()) {
        resetNAND();
        writeNAND(LUN, position, data);
    }
}

uint8_t MT29F::readNAND(uint8_t LUN, uint32_t position) {
    Address readAddress = setAddress(LUN, position);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    while (PIO_PinRead(nandReadyBusyPin) == 0) {}
    return readData();
}

uint8_t *MT29F::readNAND(uint8_t *data, uint8_t LUN, uint32_t start_position, uint32_t end_position) {
    uint8_t numberOfAddresses = end_position - start_position + 1;
    Address readAddress = setAddress(LUN, start_position);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    for (int i = 0; i < numberOfAddresses; i++) {
        while (PIO_PinRead(nandReadyBusyPin) == 0) {}
        data[i] = readData();
    }
    return data;
}

void MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    PIO_PinWrite(nandWriteProtect, 1);
    uint8_t row1 = block << 7;
    uint8_t row2 = block >> 1;
    uint8_t row3 = (block >> 9) | (LUN << 3);
    sendCommand(ERASE_BLOCK);
    sendAddress(row1);
    sendAddress(row2);
    sendAddress(row3);
    sendCommand(ERASE_BLOCK_CONFIRM);
    if (detectErrorArray()) {
        resetNAND();
        eraseBlock(LUN, block);
    }
}

bool MT29F::detectErrorArray() {
    sendCommand(READ_STATUS);
    while (PIO_PinRead(nandReadyBusyPin) == 0) {}
    uint8_t status = readData();
    while ((status & ArrayReadyMask) == 0) {
        status = readData();
    }
    if (status & 0x1)
        return true;
    else return false;
}

bool MT29F::isNANDAlive() {
    uint8_t *id = {};
    uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
    readNANDID(id);
    if (std::equal(std::begin(valid_id), std::end(valid_id), id))
        return true;
    else return false;
}
