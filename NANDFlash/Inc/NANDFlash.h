#include <cstdint>
#include "definitions.h"


/**
 * This is a driver for MT29F NAND Flash.
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 *  and https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 */
class MT29F {
private:
    static uint32_t dataRegister = 0x60000000;
    static uint32_t addressRegister = 0x60200000;
    static uint32_t commandRegister = 0x60400000;

    void writeData(uint8_t data) {
        *((volatile uint8_t *) dataRegister) = (uint8_t) data;
    }

    void sendAddress(uint8_t address) {
        *((volatile uint8_t *) addressRegister) = (uint8_t) address;
    }

    void sendCommand(uint8_t command) {
        *((volatile uint8_t *) commandRegister) = (uint8_t) command;
    }

    PIO_PIN NANDOE = PIO_PIN_PC9;
    PIO_PIN NANDWE = PIO_PIN_PC10;
    PIO_PIN NANDCLE  = PIO_PIN_PC17;
    PIO_PIN NANDALE = PIO_PIN_PC16;
    PIO_PIN NCS = PIO_PIN_PA3;
public:

    void enableCS() {
        PIO_PinWrite(NCS, false);
    }

    void disableCS() {
        PIO_PinWrite(NCS, true);
    }

    void busIdle() {
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDWE, true);
        PIO_PinWrite(NANDOE, true);
    }

    void sendCommandToNAND(uint8_t command) {
        sendCommand(command);
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDALE, false);
        PIO_PinWrite(NANDCLE, true);
        PIO_PinWrite(NANDOE, true);
    }

    void sendAddressToNAND(uint8_t address) {
        sendAddress(address);
        PIO_PinWrite(NCS, true);
        PIO_PinWrite(NANDALE, true);
        PIO_PinWrite(NANDCLE, false);
        PIO_PinWrite(NANDOE, true);
    }

    void writeDataToNAND(uint8_t data) {
        WriteData(data);
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDALE, false);
        PIO_PinWrite(NANDCLE, false);
        PIO_PinWrite(NANDOE, true);
    }

    uint8_t readDataFromNAND() {
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDALE, false);
        PIO_PinWrite(NANDCLE, false);
        PIO_PinWrite(NANDWE, false);
        return *((volatile uint8_t *) dataRegister) = (uint8_t) data;
    }
};
