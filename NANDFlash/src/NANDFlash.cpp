#include "NANDFlash.h"

//uint8_t MT29F::initialize(){
////
////    uint8_t data;
////    while(1) {
////        if (PIO_PinRead(nandReadyBusyPin)) {
////            sendCommand(0xFF);
////            while (1) {
////                if (PIO_PinRead(nandReadyBusyPin)) {
////                    sendCommand(0x70);
////                    break;
////                }
////            }
////            PIO_PinWrite(NCS, false);
////            for(int i = 0; i < 10; i++){
////                data = readData();
////            }
////            break;
////        }
////    }
////    return data;
////}

void MT29F::resetNAND() {
    sendCommand(RESET);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void MT29F::initialize() {
    resetNAND();
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
