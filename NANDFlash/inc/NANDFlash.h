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
    const static inline uint16_t MaxNumPage = 524288;
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

    enum AddressConfig : const uint8_t{
            POS = 0x01,
            PAGE,
            POS_PAGE,
            PAGE_BLOCK
    };

    struct Structure {
        uint64_t position = MaxNumPage * PageSizeBytes; // overall position inside the LUN or inside a specific page
        uint16_t page = MaxNumPage, block = MaxNumBlock; // overall page number or number of page inside a single block
        uint8_t LUN = 2;
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

    void readNANDID(etl::array<uint8_t, 8> id);

    Address setAddress(Structure *pos, AddressConfig mode);

    constexpr static bool isValidStructure(const Structure *pos, const AddressConfig op);

    template<Structure *pos, AddressConfig op>
    bool writeNAND(uint8_t data) {
        static_assert(!isValidStructure(pos, op), "The Structure to write a single byte to is not valid");

        const Address writeAddress = setAddress(pos, PAGE);
        sendCommand(PAGE_PROGRAM);
        sendAddress(writeAddress.col1);
        sendAddress(writeAddress.col2);
        sendAddress(writeAddress.row1);
        sendAddress(writeAddress.row2);
        sendAddress(writeAddress.row3);
        sendData(data);
        sendCommand(PAGE_PROGRAM_CONFIRM);
        return !detectArrayError();
    }

    template<unsigned int size, Structure *pos, AddressConfig op>
    bool writeNAND(etl::array<uint8_t, size> data) {
        static_assert(size > MaxDataBytesPerPage, "Size of data surpasses the Page limit");
        static_assert(!isValidStructure(pos, op), "The Structure is not valid");
        if (op == POS || op == POS_PAGE) {
            constexpr uint8_t page = pos->position / PageSizeBytes;
            constexpr uint16_t column = pos->position - page * PageSizeBytes;
            static_assert((column + size + NumECCBytes) > PageSizeBytes,
                          "There is not enough space in this page for the data");
        }

        const Address writeAddress = setAddress(pos, op);
        sendCommand(PAGE_PROGRAM);
        sendAddress(writeAddress.col1);
        sendAddress(writeAddress.col2);
        sendAddress(writeAddress.row1);
        sendAddress(writeAddress.row2);
        sendAddress(writeAddress.row3);
        for (uint16_t i = 0; i < size; i++) {
            if (waitDelay()) {
                return !NANDisReady;
            }
            sendData(data[i]);
        }
        sendCommand(PAGE_PROGRAM_CONFIRM);
        return !detectArrayError();
    }

    /* This function is the more abstract version of the write function where the user does not need to worry about the
     * size of the data and where they'll be fitted. The position or page the data are written to is configurable
     * by the user */
    template<unsigned int size, Structure *pos, AddressConfig op>
    bool abstactWriteNAND(etl::array<uint8_t, size> data) {
        static_assert(!isValidStructure(pos, op), "The Structure provided by the user is not valid");
        constexpr uint32_t quotient = size / WriteChunkSize;
        constexpr uint32_t writeChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
        constexpr uint16_t pagesToWrite = writeChunks / MaxChunksPerPage;

//        etl::array<uint8_t, size> readData = {};
//        readNAND<size, pos, op>(readData);
//
//        if(etl::find()){
//
//        }

        if (op == PAGE || op == PAGE_BLOCK) {
            for (size_t i = 0; i < pagesToWrite; i++) {
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = (data
                        << (WriteChunkSize * i)); // TODO: add 512-byte mask
                if (!writeNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToWrite)) return false;
            }
        } else {
            pos->page = (op == POS) ? (pos->position / PageSizeBytes) : pos->page;
            pos->block = (op == POS) ? (pos->page / NumPagesBlock) : pos->block;
            constexpr uint16_t column = pos->position - (pos->page * PageSizeBytes);
            constexpr uint8_t chunksFitThisPage = (PageSizeBytes - column) / (WriteChunkSize + NumECCBytes);
            for (size_t i = 0; i < chunksFitThisPage; i++) {
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = (data
                        << (WriteChunkSize * i)); // TODO: add 512-byte mask
                if (!writeNAND<(WriteChunkSize + NumECCBytes), pos, op>(pos, data)) return false;
                if (i != (chunksFitThisPage - 1)) {
                    pos->position += (WriteChunkSize + NumECCBytes);
                } else break;
            }
            pos->position = 0;
            if (chunksFitThisPage < writeChunks) {
                constexpr uint16_t pagesToWriteLeft =
                        pagesToWrite - ((writeChunks - chunksFitThisPage) / MaxChunksPerPage);
                for (size_t page = 0; page < pagesToWriteLeft; page++) {
                    if ((pos->page + 1) != NumPagesBlock) {
                        ++pos->page;
                        // TODO: add read to see if that page is written already up to numberOfAddresses + NumECCBytes
                    } else if ((pos->block + 1) != MaxNumBlock) {
                        ++pos->block;
                    } else pos->LUN = pos->LUN == 0 ? 1 : 0;
                    uint32_t chunksFitTillThisPage = chunksFitThisPage + (page * MaxChunksPerPage);
                    for (size_t chunk = 0;
                         (chunk < (writeChunks - chunksFitTillThisPage) ||
                          (chunk < MaxChunksPerPage)); chunk++) {
                        etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = (data
                                << (WriteChunkSize *
                                    (chunk + chunksFitTillThisPage))); // TODO: add 512-byte mask
                        if (!writeNAND<size, pos, PAGE_BLOCK>(pos, data)) return false;
                    }
                }

            }

        }
        return true;

    }

    bool readNAND(uint8_t data, uint8_t LUN, uint32_t position);

//    template <unsigned int size>
//    bool readNAND(etl::array<uint8_t, size> data, uint8_t LUN, uint32_t start_position, uint32_t numberOfAddresses) {
//        static_assert(size < PageSizeBytes, "Data to be read from NAND surpass the Page limit");
//        if (numberOfAddresses > size){
//            return !NANDisReady;
//        }
//        const Address readAddress = setAddress(LUN, start_position);
//        sendCommand(READ_MODE);
//        sendAddress(readAddress.col1);
//        sendAddress(readAddress.col2);
//        sendAddress(readAddress.row1);
//        sendAddress(readAddress.row2);
//        sendAddress(readAddress.row3);
//        sendCommand(READ_CONFIRM);
//        for (uint16_t i = 0; i < numberOfAddresses; i++) {
//            if (waitDelay()) {
//                return !NANDisReady;
//            }
//            data[i] = readData();
//        }
//        return NANDisReady;
//    }

    bool eraseBlock(uint8_t LUN, uint16_t block);

    bool detectArrayError();

    bool isNANDAlive();

    bool waitDelay();

    bool errorHandler();
};
