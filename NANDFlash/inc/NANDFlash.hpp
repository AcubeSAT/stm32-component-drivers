#pragma once

#include "SMC.hpp"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Logger.hpp"
#include "nand_MT29F_lld.hpp"
#include "nand_hw_if.hpp"

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

    enum Commands : const uint8_t{
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

    /* Number of bytes contained in a page */
    const static inline uint16_t PageSizeBytes = 8640;
    /* Number of bytes contained in a block */
    const static inline uint32_t BlockSizeBytes = 1105920;
    /* Number of pages contained in a block */
    const static inline uint8_t NumPagesBlock = 128;
    /* Total number of pages in an LUN */
    const static inline uint32_t MaxNumPage = 524288;
    /* Total number of blocks in a LUN */
    const static inline uint16_t MaxNumBlock = 4096;
    /* The ARDY bit is bit 5 of the Status Register */
    const static inline uint8_t ArrayReadyMask = 0x20;
    /* Chunk size of data to be written in addition to ECC parity bits */
    const static inline uint16_t WriteChunkSize = 512;
    /* Max number of chunks contained in a page */
    const static inline uint8_t MaxChunksPerPage = PageSizeBytes / WriteChunkSize;
    /* Number of ECC bytes per write data chunk */
    const static inline uint16_t NumECCBytes = 3;
    /* Max number of data bytes available in a page including ECC bytes */
    const static inline uint16_t MaxDataBytesPerPage = PageSizeBytes - (MaxChunksPerPage * NumECCBytes);


    struct Address {
        uint8_t col1, col2, row1, row2, row3;
    };

    struct ECCBits {
        unsigned LP0: 1, LP1: 1, LP2: 1, LP3: 1, LP4: 1, LP5: 1, LP6: 1, LP7: 1, LP8: 1, LP9: 1, LP10: 1, LP11: 1, LP12: 1,
                LP13: 1, LP14: 1, LP15: 1, LP16: 1, LP17: 1, CP0: 1, CP1: 1, CP2: 1, CP3: 1, CP4: 1, CP5: 1,
                column0: 1, column1: 1, column2: 1, column3: 1, column4: 1, column5: 1, column6: 1, column7: 1;
    };

    const static inline uint8_t TimeoutCycles = 20;

public:
    enum AddressConfig : const uint8_t{
            POS = 0x01,
            PAGE,
            POS_PAGE,
            PAGE_BLOCK,
            ECC_ERROR,
            UNCORRECTABLE_ERROR
    };

    struct Structure {
        uint64_t position = MaxNumPage * PageSizeBytes; // overall position inside the LUN or inside a specific page
        uint32_t page = MaxNumPage, block = MaxNumBlock; // overall page number or number of page inside a single block
        uint8_t LUN = 2;
    };

    enum ErrorCodes : const uint8_t{
            NAND_NOT_READY = 0x01,
            NAND_TIMEENDS,
            NAND_FAIL_OP,
            NAND_NOT_ALIVE
    };

    /**
     * @param chipSelect Number of the Chip Select used for enabling the Nand Flash Die.
     */
    inline constexpr void selectNandConfiguration(SMC::ChipSelect chipSelect) {
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

    uint8_t resetNAND(){
        return NAND_Reset();
    }

    void readNANDID(etl::array<uint8_t, 8> id) {
        NAND_Read_ID(id.data());
    }

    bool isNANDAlive(){
        etl::array<uint8_t, 8> id = {};
        const uint8_t valid_id[8] = {0x2C, 0x68, 0x00, 0x27, 0xA9, 0x00, 0x00, 0x00};
        readNANDID(id);
        return etl::equal(id.begin(), id.end(), valid_id);
    }

    uint8_t eraseBlock(uint8_t LUN, uint16_t block){
        nand_address_t addr = {LUN, block};
        return NAND_Block_Erase(addr);
    }

    Address setAddress(Structure *pos, AddressConfig mode) {
        uint16_t column = pos->position;
        switch (mode) {
            case POS:
                column -= pos->page * PageSizeBytes;
                pos->page = pos->position / PageSizeBytes;
            case POS_PAGE:
                pos->block = pos->position / BlockSizeBytes;
                break;
            case PAGE:
                column = 0;
                pos->block = pos->page / NumPagesBlock;
                pos->page = pos->page - pos->block * NumPagesBlock;
        }


        Address address;
        address.col1 = column & 0xff;
        address.col2 = (column & 0x3f00) >> 8;
        address.row1 = (pos->page & 0x7f) | ((pos->block & 0x01) << 7);
        address.row2 = (pos->block >> 1) & 0xff;
        address.row3 = ((pos->block >> 9) & 0x07) | ((pos->LUN & 0x01) << 3);

        return address;
    }

    constexpr static bool isValidStructure(Structure *pos, AddressConfig op){
        if (pos->LUN > 1) return false;
        switch (op) {
            case POS:
                if (pos->position >= ((MaxNumPage * PageSizeBytes) - (MaxChunksPerPage * NumECCBytes))) return false;
                break;
            case POS_PAGE:
                if (pos->position >= MaxDataBytesPerPage) return false;
                break;
            case PAGE:
                if (pos->page >= MaxNumPage) return false;
                break;
            case PAGE_BLOCK:
                if (pos->page >= NumPagesBlock || pos->block >= MaxNumBlock) return false;
        }
        return true;
    }

    uint8_t detectArrayError(){
        sendCommand(READ_STATUS);
        uint8_t status = readData();
        const uint32_t start = xTaskGetTickCount();
        while ((status & ArrayReadyMask) == 0) {
            status = readData();
            if ((xTaskGetTickCount() - start) > TimeoutCycles) {
                return NAND_TIMEOUT;
            }
        }
        return (status & 0x1) ? NAND_FAIL_OP : 0;
    }

    template<Structure *pos, AddressConfig op>
    uint8_t writeNAND(uint8_t data){
        static_assert(!isValidStructure(pos, op), "The Structure to write a single byte to is not valid");

        const Address writeAddress = setAddress(pos, op);
        sendCommand(PAGE_PROGRAM);
        sendAddress(writeAddress.col1);
        sendAddress(writeAddress.col2);
        sendAddress(writeAddress.row1);
        sendAddress(writeAddress.row2);
        sendAddress(writeAddress.row3);
        sendData(data);
        sendCommand(PAGE_PROGRAM_CONFIRM);
        return detectArrayError();
    }

};