#include "NANDFlash.h"


uint8_t MT29F::initialize(){

    uint8_t data;
    while(1) {
        if (PIO_PinRead(PIO_PIN_PA12)) {
            sendCommand(0xFF);
            while (1) {
                if (PIO_PinRead(PIO_PIN_PA12)) {
                    sendCommand(0x70);
                    break;
                }
            }
            PIO_PinWrite(NCS, false);
            for(int i = 0; i < 10; i++){
                data = *((volatile uint8_t *) dataRegister);
            }
            break;
        }
    }
    return data;
}

void MT29F::writeData(uint8_t data) {
    PIO_PinWrite(NCS, false);
    PIO_PinWrite(NANDALE, false);
    PIO_PinWrite(NANDCLE, false);
    PIO_PinWrite(NANDOE, true);
    *((volatile uint8_t *) dataRegister) = (uint8_t) data;
    PIO_PinWrite(NCS, true);
}

void MT29F::sendAddress(uint8_t address) {
    PIO_PinWrite(NCS, false);
    PIO_PinWrite(NANDALE, true);
    PIO_PinWrite(NANDCLE, false);
    PIO_PinWrite(NANDOE, true);
    *((volatile uint8_t *) addressRegister) = (uint8_t) address;
    PIO_PinWrite(NCS, true);
}

void MT29F::sendCommand(uint8_t command) {
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDALE, false);
        PIO_PinWrite(NANDOE, true);
        PIO_PinWrite(NANDCLE, false);
        *((volatile uint8_t *) commandRegister) = (uint8_t) command;
        PIO_PinWrite(NCS, true);
        PIO_PinWrite(NANDOE, false);
}

uint8_t MT29F::readDataFromNAND() {
    PIO_PinWrite(NCS, false);
    PIO_PinWrite(NANDALE, false);
    PIO_PinWrite(NANDCLE, false);
    PIO_PinWrite(NANDWE, true);
    uint8_t data = *((volatile uint8_t *) dataRegister);
    PIO_PinWrite(NCS, true);
    return data;
}


