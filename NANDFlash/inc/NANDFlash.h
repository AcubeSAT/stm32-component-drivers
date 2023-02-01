#pragma once

#include "SMC.hpp"
//#include <cstdint>
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * This is a driver for MT29F NAND Flash.
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 *  and https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 */
class MT29F : public SMC {
public:
    // WIP enum
//    enum NandReadyBusyPin : PIO_PIN {
//        MemoryPartition1 = MEM_NAND_BUSY_1_PIN,
//        MemoryPartition2 = MEM_NAND_BUSY_2_PIN
//    };

private:
    const uint32_t triggerNANDALEAddress = moduleBaseAddress | 0x200000;
    const uint32_t triggerNANDCLEAddress = moduleBaseAddress | 0x400000;

    const PIO_PIN nandReadyBusyPin = PIO_PIN_NONE; //TODO: if PIO_PIN == NONE => throw error
//    PIO_PIN NANDOE = PIO_PIN_PC9;
//    PIO_PIN NANDWE = PIO_PIN_PC10;
//    PIO_PIN NANDCLE  = PIO_PIN_PC17;
//    PIO_PIN NANDALE = PIO_PIN_PC16;
//    PIO_PIN NCS = PIO_PIN_PD18;

public:
    constexpr MT29F(ChipSelect chipSelect, PIO_PIN nandReadyBusyPin) : SMC(chipSelect), nandReadyBusyPin(nandReadyBusyPin) {}

    uint8_t initialize();

    inline void nandSendData(uint8_t data) {
        smcDataWrite(moduleBaseAddress, data);
    }

    inline void nandSendAddress(uint8_t address) {
        smcDataWrite(triggerNANDALEAddress, address);
    }

    inline void nandSendCommand(uint8_t command) {
        smcDataWrite(triggerNANDCLEAddress, command);
    }

    inline uint8_t nandReadData() {
        return smcDataRead(moduleBaseAddress);
    }

    void READ_ID();
};
