#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {

    uint8_t status = 0;

    if (PIO_PinRead(nandReadyBusyPin)) {
        sendCommand(RESET);
        vTaskDelay(pdMS_TO_TICKS(1));
        sendCommand(READ_STATUS);
        for (int i = 0; i < 2; i++)
            if (i == 1)
                return readData();
    }

}


void MT29F::readNANDID() {
    uint8_t buffer[5];
    sendCommand(0x90);
    sendAddress(0x00);
    vTaskDelay(pdMS_TO_TICKS(300));

    for (int i = 0; i < 5; i++) {
        buffer[i] = readData();
        LOG_DEBUG << buffer[i] << " ";
    }
}


void MT29F::writeNAND(MT29F::Address writeAddress, uint8_t data) {
    vTaskDelay(pdMS_TO_TICKS(1));
    sendAddress(writeAddress.col1);
    sendAddress(writeAddress.col2);
    sendAddress(writeAddress.row1);
    sendAddress(writeAddress.row2);
    sendAddress(writeAddress.row3);
    vTaskDelay(pdMS_TO_TICKS(1));
    sendData(data);
}

uint8_t MT29F::readNAND(MT29F::Address readAddress) {
    sendCommand(READ_MODE);
    vTaskDelay(pdMS_TO_TICKS(1));
    sendAddress(readAddress.col1);
    sendAddress(readAddress.col2);
    sendAddress(readAddress.row1);
    sendAddress(readAddress.row2);
    sendAddress(readAddress.row3);
    vTaskDelay(pdMS_TO_TICKS(1));
    for (int i = 0; i < 2; i++)
        if (i == 1)
            return readData();
}