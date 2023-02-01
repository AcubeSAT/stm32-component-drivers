#include "NANDFlash.h"

//uint8_t MT29F::initialize(){
//
//    uint8_t data;
//    while(1) {
//        if (PIO_PinRead(PIO_PIN_PA12)) {
//            sendCommand(0xFF);
//            while (1) {
//                if (PIO_PinRead(PIO_PIN_PA12)) {
//                    sendCommand(0x70);
//                    break;
//                }
//            }
//            PIO_PinWrite(NCS, false);
//            for(int i = 0; i < 10; i++){
//                data = *((volatile uint8_t *) dataRegister);
//            }
//            break;
//        }
//    }
//    return data;
//}


void MT29F::READ_ID() {
    uint8_t buffer[5];
    sendCommand(0x90);
    sendAddress(0x00);

    for(int i=0; i<5; i++) {
        buffer[i] = readDataFromNAND();
    }
 return ;
}

void MT29F::writeData(uint8_t data) {
    *((volatile uint8_t *) dataRegister) = (uint8_t) data;
}

void MT29F::sendAddress(uint8_t address) {
    *((volatile uint8_t *) addressRegister) = (uint8_t) address;
}

void MT29F::sendCommand(uint8_t command) {
        *((volatile uint8_t *) commandRegister) = (uint8_t) command;
}

uint8_t MT29F::readDataFromNAND() {
    return *((volatile uint8_t *) dataRegister);
}


