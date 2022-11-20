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

    void writeData(uint8_t data);

    void sendAddress(uint8_t address);

    void sendCommand(uint8_t command);
};
