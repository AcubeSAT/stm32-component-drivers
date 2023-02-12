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

    const PIO_PIN nandReadyBusyPin = PIO_PIN_NONE; //TODO: if PIO_PIN == NONE => throw log message
    const PIO_PIN NANDOE = PIO_PIN_PC9;
    const PIO_PIN NANDWE = PIO_PIN_PC10;
    const PIO_PIN NANDCLE  = PIO_PIN_PC17;
    const PIO_PIN NANDALE = PIO_PIN_PC16;
    const PIO_PIN NCS = PIO_PIN_PD18;
    inline static constexpr uint8_t enableNandConfiguration = 1;

public:
    /**
     * @param chipSelect Number of the Chip Select used for enabling the Nand Flash Die.
     */
    inline constexpr void selectNandConfiguration(ChipSelect chipSelect) {
        switch (chipSelect) {
            case NCS0:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS0(enableNandConfiguration);
                return;

            case NCS1:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS1(enableNandConfiguration);
                return;

            case NCS2:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS2(enableNandConfiguration);
                return;

            case NCS3:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS3(enableNandConfiguration);
                return;

            default:
                return;
        }
    }

    constexpr MT29F(ChipSelect chipSelect, PIO_PIN nandReadyBusyPin) : SMC(chipSelect), nandReadyBusyPin(nandReadyBusyPin) {
        selectNandConfiguration(chipSelect);
        //TODO: add reset function from manufacturer
    }

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

    uint8_t initialize();

    void READ_ID();
};
