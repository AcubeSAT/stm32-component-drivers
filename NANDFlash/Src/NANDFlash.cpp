#include "NANDFlash.h"


    void MT29F::writeData(uint8_t data) {
        *((volatile uint8_t *) dataRegister) = (uint8_t) data;
    }

    void MT29F::sendAddress(uint8_t address) {
        *((volatile uint8_t *) addressRegister) = (uint8_t) address;
    }

    void MT29F::sendCommand(uint8_t command) {
        *((volatile uint8_t *) commandRegister) = (uint8_t) command;
    }

};

