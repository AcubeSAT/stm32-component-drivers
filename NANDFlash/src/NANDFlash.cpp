#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {
    sendCommand(RESET);
    sendCommand(READ_STATUS);
    if (waitDelay()) {
        return !NANDisReady;
    }
    return readData();
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
    sendAddress(READ_MODE);
    for (uint8_t i = 0; i < 8; i++) {
        id[i] = readData();
    }
}


bool MT29F::writeNAND(uint8_t LUN, uint32_t position, uint8_t data) {
    Address writeAddress = setAddress(LUN, position);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    sendData(data);
    sendCommand(PAGE_PROGRAM_CONFIRM);
    return !detectArrayError();
}

bool MT29F::writeNAND(uint8_t LUN, uint32_t position, uint32_t numberOfAddresses, uint8_t *data) {
    Address writeAddress = setAddress(LUN, position);
    sendCommand(PAGE_PROGRAM);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    for (uint16_t i = 0; i < numberOfAddresses; i++) {
        if (waitDelay()) {
            return !NANDisReady;
        }
        sendData(data[i]);
    }
    sendCommand(PAGE_PROGRAM_CONFIRM);
    return !detectArrayError();
}

bool MT29F::readNAND(uint8_t data, uint8_t LUN, uint32_t position) {
    Address readAddress = setAddress(LUN, position);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    if (waitDelay() == 0) {
        return !NANDisReady;
    }
    data = readData();
    return NANDisReady;
}

bool MT29F::readNAND(uint8_t *data, uint8_t LUN, uint32_t start_position, uint32_t end_position) {
    uint8_t numberOfAddresses = end_position - start_position + 1;
    Address readAddress = setAddress(LUN, start_position);
    sendCommand(READ_MODE);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    sendCommand(READ_CONFIRM);
    for (uint16_t i = 0; i < numberOfAddresses; i++) {
        if (waitDelay()) {
            return !NANDisReady;
        }
        data[i] = readData();
    }
    return NANDisReady;
}

bool MT29F::eraseBlock(uint8_t LUN, uint16_t block) {
    uint8_t row1 = block << 7;
    uint8_t row2 = block >> 1;
    uint8_t row3 = (block >> 9) | (LUN << 3);
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
    uint32_t start = xTaskGetTickCount();
    while ((status & ArrayReadyMask) == 0) {
        status = readData();
        if ((xTaskGetTickCount() - start) > TimeoutCycles) {
            return NANDTimeout;
        }
    }
    if (status & 0x1) {
        return true;
    } else return false;
}

bool MT29F::isNANDAlive() {
    uint8_t id[8] = {};
    uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
    readNANDID(id);
    if (std::equal(std::begin(valid_id), std::end(valid_id), id)) {
        return true;
    } else return false;
}

bool MT29F::waitDelay() {
    uint32_t start = xTaskGetTickCount();
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
