#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {

    if (PIO_PinRead(nandReadyBusyPin)) {
        sendCommand(RESET);
        sendCommand(READ_STATUS);
        vTaskDelay(pdMS_TO_TICKS(1));
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


uint8_t* MT29F::readNANDID() {
    uint8_t id[8] = {};
    sendCommand(READID);
    sendAddress(READ_MODE);
    for (int i = 0; i < 8; i++) {
        id[i] = readData();
    }
    return id;
}


void MT29F::writeNAND(uint8_t LUN, uint32_t position, uint8_t data) {
    Address writeAddress = setAddress(LUN, position);
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    vTaskDelay(pdMS_TO_TICKS(1));
    sendData(data);
}

uint8_t MT29F::readNAND(uint8_t LUN, uint32_t position) {
    sendCommand(READ_MODE);
    Address readAddress = setAddress(LUN, position);
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    for (int i = 0; i < 2; i++)
        if (i == 1)
            return readData();
}