#include "NANDFlash.h"


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
    PIO_PinWrite(NANDCLE, true);
    PIO_PinWrite(NANDOE, true);
    *((volatile uint8_t *) commandRegister) = (uint8_t) command;
    PIO_PinWrite(NCS, true);
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


