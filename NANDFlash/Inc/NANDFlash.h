#pragma once

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
    uint32_t dataRegister = 0x61000000;
    uint32_t addressRegister = 0x61200000;
    uint32_t commandRegister = 0x61400000;

    PIO_PIN NANDOE = PIO_PIN_PC9;
    PIO_PIN NANDWE = PIO_PIN_PC10;
    PIO_PIN NANDCLE  = PIO_PIN_PC17;
    PIO_PIN NANDALE = PIO_PIN_PC16;
    PIO_PIN NCS = PIO_PIN_PD18;

public:

    uint8_t initialize();

    void writeData(uint8_t data);

    void sendAddress(uint8_t address);

    void sendCommand(uint8_t command);

    uint8_t readDataFromNAND();

};
