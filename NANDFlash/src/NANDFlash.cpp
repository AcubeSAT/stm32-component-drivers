#include "NANDFlash.h"


uint8_t MT29F::resetNAND() {

    uint8_t status = 0;

    if (PIO_PinRead(nandReadyBusyPin)) {
        sendCommand(RESET);
        vTaskDelay(pdMS_TO_TICKS(1));
        sendCommand(READ_STATUS);
        for (int i = 0; i < 2; i++)
            if (i == 1)
                status = readData();
    }
    return status;
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
