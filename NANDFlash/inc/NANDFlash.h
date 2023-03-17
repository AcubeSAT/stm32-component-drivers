#pragma once

#include "SMC.hpp"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Logger.hpp"

/**
 * This is a driver for MT29F NAND Flash.
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 *  and https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 */
class MT29F : public SMC {
private:
    const uint32_t triggerNANDALEAddress = moduleBaseAddress | 0x200000;
    const uint32_t triggerNANDCLEAddress = moduleBaseAddress | 0x400000;

    const PIO_PIN nandReadyBusyPin = PIO_PIN_NONE; //TODO: if PIO_PIN == NONE => throw log message
    const PIO_PIN nandWriteProtect = PIO_PIN_NONE;

    inline static constexpr uint8_t enableNandConfiguration = 1;

    enum Commands : uint8_t {
        RESET = 0xFF,
        READID = 0x90,
        READ_PARAM_PAGE = 0xEC,
        READ_UNIQ_ID = 0xED,
        SET_FEATURE = 0xEF,
        GET_FEATURE = 0xEE,
        READ_STATUS = 0x70,
        READ_STATUS_ENHANCED = 0x78,
        ERASE_BLOCK = 0x60,
        ERASE_BLOCK_CONFIRM = 0xD0,
        READ_MODE = 0x00,
        READ_CONFIRM = 0x30,
        PAGE_PROGRAM = 0x80,
        PAGE_PROGRAM_CONFIRM = 0x10,
        READ_INTERNAL_DATA_MOVE = 0x35,
        PROGRAM_INTERNAL_DATA_MOVE = 0x85,
        LOCK = 0x2A,
        BLOCK_UNLOCK_LOW = 0x23,
        BLOCK_UNLOCK_HIGH = 0x24,
        BLOCK_LOCK_READ_STATUS = 0x7A
    };

    /* The number of bytes a page contains*/
    const static inline uint16_t PageSizeBytes = 8640;
    /* The number of bytes a block contains*/
    const static inline uint32_t BlockSizeBytes = 1105920;
    /* The ARDY bit is bit 5 of the Status Register */
    const static inline uint8_t ArrayReadyMask = 0x20;

    struct Address {
        uint8_t col1, col2, row1, row2, row3;
    };

    const static inline uint8_t TimeoutCycles = 20;

    const static inline bool NANDTimeout = true;

    const static inline bool NANDisReady = true;

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
                MATRIX_REGS->CCFG_SMCNFCS &= 0xF;
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

    constexpr MT29F(ChipSelect chipSelect, PIO_PIN nandReadyBusyPin, PIO_PIN nandWriteProtect) : SMC(chipSelect),
                                                                                                 nandReadyBusyPin(
                                                                                                         nandReadyBusyPin),
                                                                                                 nandWriteProtect(
                                                                                                         nandWriteProtect) {
        selectNandConfiguration(chipSelect);
    }

    inline void sendData(uint8_t data) {
        smcWriteByte(moduleBaseAddress, data);
    }

    inline void sendAddress(uint8_t address) {
        smcWriteByte(triggerNANDALEAddress, address);
    }

    inline void sendCommand(uint8_t command) {
        smcWriteByte(triggerNANDCLEAddress, command);
    }

    inline uint8_t readData() {
        return smcReadByte(moduleBaseAddress);
    }

    uint8_t resetNAND();

    void readNANDID(uint8_t *id);

    Address setAddress(uint8_t LUN, uint32_t position);

    bool writeNAND(uint8_t LUN, uint32_t position, uint8_t data);

    bool writeNAND(uint8_t LUN, uint32_t position, uint32_t numberOfAddresses, uint8_t *data);

    bool readNAND(uint8_t data, uint8_t LUN, uint32_t position);

    bool readNAND(uint8_t *data, uint8_t LUN, uint32_t start_position, uint32_t end_position);

    bool eraseBlock(uint8_t LUN, uint16_t block);

    bool detectArrayError();

    bool isNANDAlive();

    bool waitDelay();

    bool errorHandler();
};
