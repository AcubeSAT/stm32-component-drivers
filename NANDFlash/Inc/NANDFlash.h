#include <cstdint>
#include "definitions.h"

#pragma once
/**
 * This is a driver for MT29F NAND Flash.
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 *  and https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 */
class MT29F {
private:
    uint32_t dataRegister = 0x60000000;
    uint32_t addressRegister = 0x60200000;
    uint32_t commandRegister = 0x60400000;

    PIO_PIN NANDOE = PIO_PIN_PC9;
    PIO_PIN NANDWE = PIO_PIN_PC10;
    PIO_PIN NANDCLE  = PIO_PIN_PC17;
    PIO_PIN NANDALE = PIO_PIN_PC16;
    PIO_PIN NCS = PIO_PIN_PD19;

public:

    void writeData(uint8_t data);

    void sendAddress(uint8_t address);

    void sendCommand(uint8_t command){
        PIO_PinWrite(NCS, false);
        PIO_PinWrite(NANDALE, false);
        PIO_PinWrite(NANDOE, true);
        PIO_PinWrite(NANDCLE, false);
        *((volatile uint8_t *) commandRegister) = (uint8_t) command;
        PIO_PinWrite(NCS, true);
    }

    uint8_t readDataFromNAND();

    uint8_t initialize(){    MATRIX_REGS->CCFG_SMCNFCS = 8;
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
                PIO_PinWrite(NANDOE, false);
                for(int i = 0; i < 10; i++){
                    data = *((volatile uint8_t *) dataRegister);
                }
                break;
            }
        }
        return data;
    }
};
