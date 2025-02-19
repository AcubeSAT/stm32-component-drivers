#pragma once

#include "SMC.hpp"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Logger.hpp"

enum class MT29F_Errno : uint8_t {
    NONE = 0,
    TIMEOUT = 1,
    ADDRESS_OUT_OF_BOUNDS = 2,
    BUSY_IO = 3,
    BUSY_ARRAY = 4,
    FAIL_PREVIOUS = 5,
    FAIL_RECENT = 6,
    NOT_READY = 7
};


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
        READ_ONFI_CONFIRM = 0x20,

        SET_FEATURE = 0xEF,
        GET_FEATURE = 0xEE,

        READ_STATUS = 0x70,
        READ_STATUS_ENHANCED = 0x78,

        ERASE_BLOCK = 0x60,
        ERASE_BLOCK_CONFIRM = 0xD0,

        READ_MODE = 0x00,
        READ_CONFIRM = 0x30,
        READ_NEXT_SEQ_PAGE = 0x31,

        PAGE_PROGRAM = 0x80,
        PAGE_PROGRAM_CONFIRM = 0x10,

        READ_INTERNAL_DATA_MOVE = 0x35,
        PROGRAM_INTERNAL_DATA_MOVE = 0x85,

        LOCK = 0x2A,
        BLOCK_UNLOCK_LOW = 0x23,
        BLOCK_UNLOCK_HIGH = 0x24,
        BLOCK_LOCK_READ_STATUS = 0x7A
    };

    const static inline uint64_t MaxAllowedAddress = 4529848319;    // Max address available by the NAND module
    const static inline uint16_t PageSizeBytes = 8640;              // Bytes per page
    const static inline uint8_t PagesPerBlock = 128;                // Pages per block
    const static inline uint32_t BlockSizeBytes = 1105920;          // Byte per Block = 128 Pages * 8640 bytes
    const static inline uint32_t BlocksPerLUN = 4095;               // Blocks per LUN
    /* Status Register Masks*/
    const static inline uint8_t MASK_STATUS_FAIL = 0x01;
    const static inline uint8_t MASK_STATUS_FAILC = 0x02;
    const static inline uint8_t MASK_STATUS_ARDY = 0x20;
    const static inline uint8_t MASK_STATUS_RDY = 0x40;

    struct Address {
        uint8_t col1, col2, row1, row2, row3;
    };

    const static inline uint8_t TimeoutCycles = 15;     // Tick Rate 1000Hz => 1 Cycle = 1ms
    // Slowest operation: Erase block ~ max time = 7ms

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

    bool setAddress(Address *address, uint8_t LUN, uint64_t position);

    // Status register functions

    bool getARDYstatus();

    bool getFAILCstatus();

    bool getFAILstatus();

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

    uint32_t getBlockSizeBytes() { return BlockSizeBytes; }
    uint32_t getBlocksPerLUN() { return BlocksPerLUN; }
    uint8_t getPagesPerBlock() { return PagesPerBlock; }
    uint16_t getPageSizeBytes() { return PageSizeBytes; }
    uint64_t getMaxAllowedAddress() { return MaxAllowedAddress; }
    // Health checks & actions

    MT29F_Errno resetNAND();

    MT29F_Errno readNANDID(etl::span<uint8_t, 8> id);

    MT29F_Errno readONFIID(etl::span<uint8_t, 8> id);

    bool isNANDAlive();

    bool waitTimeout();

    void errorHandler(MT29F_Errno error);

    // R/W Operations

    MT29F_Errno writeNAND(uint8_t LUN, uint64_t position, uint8_t data);

    MT29F_Errno writeNAND(uint8_t LUN, uint64_t position, etl::span<const uint8_t> data);

    MT29F_Errno readNAND(uint8_t LUN, uint64_t position, uint8_t &data);

    MT29F_Errno readNAND(uint8_t LUN, uint64_t start_position, etl::span<uint8_t> data);

    MT29F_Errno eraseBlock(uint8_t LUN, uint16_t block);

    // Exposed so LittleFS can use the Ready status bit

    bool getRDYstatus();
};