#pragma once

#include "etl/span.h"
#include "SMC.hpp"
#include "definitions.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Logger.hpp"
#include "etl/span.h"

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
            NAND_TIMEOUT,
            NAND_FAIL_OP,
            NAND_NOT_ALIVE
    };

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

    uint8_t resetNAND(); // TODO: use etl::expected

    void readNANDID(etl::array<uint8_t, 8> id);

    Address setAddress(Structure *pos, AddressConfig mode);

    constexpr static bool isValidStructure(Structure *pos, AddressConfig op);

    uint8_t eraseBlock(uint8_t LUN, uint16_t block); // TODO: use etl::expected

    uint8_t detectArrayError(); // TODO: use etl::expected

    bool isNANDAlive();

    uint8_t waitDelay(); // TODO: use etl::expected

    uint8_t errorHandler(); // TODO: use etl::expected

    etl::array<uint8_t, NumECCBytes> generateECCBytes(etl::array<uint8_t, WriteChunkSize> data);

    etl::array<uint8_t, WriteChunkSize>
    detectCorrectECCError(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)> dataECC,
                          etl::array<uint8_t, NumECCBytes> newECC);     // TODO: use etl::expected

    template<Structure *pos, AddressConfig op>
    uint8_t writeNAND(uint8_t data) {               // TODO: use etl::expected
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

    template<unsigned int size, Structure *pos, AddressConfig op>
    uint8_t writeNAND(etl::array<uint8_t, size> data) {     // TODO: use etl::expected
        constexpr uint8_t quotient = size / WriteChunkSize;
        constexpr uint8_t dataChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
        static_assert((dataChunks * (WriteChunkSize + NumECCBytes)) > MaxDataBytesPerPage,
                      "Size of data surpasses the Page limit");
        static_assert(!isValidStructure(pos, op), "The Structure is not valid");
        if (op == POS || op == POS_PAGE) {
            constexpr uint32_t page = pos->position / PageSizeBytes;
            constexpr uint16_t column = pos->position - page * PageSizeBytes;
            static_assert((column + (dataChunks * (WriteChunkSize + NumECCBytes))) > PageSizeBytes,
                          "There is not enough space in this page for the data");
        }

        etl::array<uint8_t, (dataChunks * (WriteChunkSize + NumECCBytes))> dataECC(255);
        for (size_t chunk = 0; chunk < dataChunks; chunk++) {
            etl::move((data.begin() + (chunk * WriteChunkSize)), (data.begin() + WriteChunkSize),
                      (dataECC.begin() + (chunk * (WriteChunkSize + NumECCBytes))));
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                    dataChunker(dataECC, (chunk * (WriteChunkSize + NumECCBytes))));
            etl::move(genECC.begin(), genECC.end(), (dataECC.begin() + (chunk * WriteChunkSize)));
        }

        const Address writeAddress = setAddress(pos, op);
        sendCommand(PAGE_PROGRAM);
        sendAddress(writeAddress.col1);
        sendAddress(writeAddress.col2);
        sendAddress(writeAddress.row1);
        sendAddress(writeAddress.row2);
        sendAddress(writeAddress.row3);
        for (uint16_t i = 0; i < dataECC.size(); i++) {
            if (waitDelay() == NAND_TIMEOUT) {
                return NAND_TIMEOUT;
            }
            sendData(dataECC[i]);
        }
        sendCommand(PAGE_PROGRAM_CONFIRM);
        return detectArrayError();
    }

    template<unsigned int size>
    etl::array<uint8_t, WriteChunkSize> dataChunker(etl::span<uint8_t, size> data, uint32_t startPos) {
        return etl::array<uint8_t, WriteChunkSize>(
                data.subspan(startPos, WriteChunkSize));
    }


    /* This function is the more abstract version of the write function where the user does not need to worry about the
     * size of the data and where they'll be fitted. The position or page the data are written to is configurable
     * by the user.*/
    template<unsigned int size, Structure *pos, AddressConfig op>
    uint8_t abstactWriteNAND(etl::array<uint8_t, size> data) {     // TODO: use etl::expected
        static_assert(!isValidStructure(pos, op), "The Structure provided by the user is not valid");
        constexpr uint32_t quotient = size / WriteChunkSize;
        constexpr uint32_t writeChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
        constexpr uint32_t pagesToWrite = writeChunks / MaxChunksPerPage;

//        etl::array<uint8_t, size> readData = {};
//        readNAND<size, pos, op>(readData);
//
//        if(etl::find()){
//
//        }

        if (op == PAGE || op == PAGE_BLOCK) {
            for (size_t page = 0; page < pagesToWrite; page++) {
                for (size_t chunk = 0; chunk < MaxChunksPerPage; chunk++) {
                    etl::array<uint8_t, WriteChunkSize> dataChunk = dataChunker(data,
                                                                                (WriteChunkSize *
                                                                                 (chunk + (page *
                                                                                           MaxChunksPerPage))));
                    etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
                    etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                    etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                    etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                    uint8_t result = writeNAND<(WriteChunkSize + NumECCBytes), pos, op>(
                            dataToWrite);    //TODO: use etl::expected
                    if (result != 0) return result;
                }
            }
            constexpr uint8_t chunksLeft = writeChunks - (MaxChunksPerPage * pagesToWrite);
            if ((pos->page + 1) != NumPagesBlock) {
                ++pos->page;
                // TODO: add read to see if that page is written already up to numberOfAddresses + NumECCBytes
            } else if ((pos->block + 1) != MaxNumBlock) {
                ++pos->block;
            } else pos->LUN = pos->LUN == 0 ? 1 : 0;

            for (size_t i = 0; i < chunksLeft; i++) {
                etl::array<uint8_t, WriteChunkSize> dataChunk = dataChunker(data, (WriteChunkSize *
                                                                                   (i +
                                                                                    (pagesToWrite *
                                                                                     MaxChunksPerPage))));
                etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                uint8_t result = writeNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToWrite);
                if (result != 0) return result;
            }
        } else {
            pos->page = (op == POS) ? (pos->position / PageSizeBytes) : pos->page;
            pos->block = (op == POS) ? (pos->page / NumPagesBlock) : pos->block;
            constexpr uint16_t column = pos->position - (pos->page * PageSizeBytes);
            constexpr uint8_t chunksFitThisPage = (PageSizeBytes - column) / (WriteChunkSize + NumECCBytes);
            for (size_t i = 0; i < chunksFitThisPage; i++) {
                etl::array<uint8_t, WriteChunkSize> dataChunk = dataChunker(data,
                                                                            (WriteChunkSize * i));
                etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                uint8_t result = writeNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToWrite);
                if (result != 0) return result;
                if (i != (chunksFitThisPage - 1)) {
                    pos->position += (WriteChunkSize + NumECCBytes);
                } else break;
            }
            pos->position = 0;
            if (chunksFitThisPage < writeChunks) {
                constexpr uint32_t pagesToWriteLeft =
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
                        etl::array<uint8_t, WriteChunkSize> dataChunk = dataChunker(data,
                                                                                    (WriteChunkSize *
                                                                                     (chunk +
                                                                                      chunksFitTillThisPage)));
                        etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(dataChunk);
                        etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToWrite = {};
                        etl::move(dataChunk.begin(), dataChunk.end(), dataToWrite.begin());
                        etl::move(genECC.begin(), genECC.end(), (dataToWrite.begin() + WriteChunkSize));
                        uint8_t result = writeNAND<size, pos, PAGE_BLOCK>(dataToWrite);
                        if (result != 0) return result;
                    }
                }

            }

        }
        return 0;

    }

    template<Structure *pos, AddressConfig op>
    uint8_t readNAND(uint8_t data) {           // TODO: use etl::expected
        static_assert(!isValidStructure(pos, op), "The Structure to read a single byte from is not valid");

        const Address readAddress = setAddress(pos, op);
        sendCommand(READ_MODE);
        sendAddress(readAddress.col1);
        sendAddress(readAddress.col2);
        sendAddress(readAddress.row1);
        sendAddress(readAddress.row2);
        sendAddress(readAddress.row3);
        sendCommand(READ_CONFIRM);
        if (waitDelay() == NAND_TIMEOUT) {
            return NAND_TIMEOUT;
        }
        data = readData();
        return 0;

    }

    template<unsigned int size, Structure *pos, AddressConfig op>
    uint8_t readNAND(etl::array<uint8_t, size> data) {         // TODO: use etl::expected
        constexpr uint8_t quotient = size / WriteChunkSize;
        constexpr uint8_t dataChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
        static_assert((dataChunks * (WriteChunkSize + NumECCBytes)) > MaxDataBytesPerPage,
                      "Size of data surpasses the Page limit");
        static_assert(!isValidStructure(pos, op), "The Structure is not valid");
        if (op == POS || op == POS_PAGE) {
            constexpr uint32_t page = pos->position / PageSizeBytes;
            constexpr uint16_t column = pos->position - page * PageSizeBytes;
            static_assert((column + (dataChunks * (WriteChunkSize + NumECCBytes))) > PageSizeBytes,
                          "There is not enough space in this page for the data");
        }
        etl::array<uint8_t, (dataChunks * (WriteChunkSize + NumECCBytes))> dataECC = {};

        const Address readAddress = setAddress(pos, op);
        sendCommand(READ_MODE);
        sendAddress(readAddress.col1);
        sendAddress(readAddress.col2);
        sendAddress(readAddress.row1);
        sendAddress(readAddress.row2);
        sendAddress(readAddress.row3);
        sendCommand(READ_CONFIRM);
        for (uint16_t i = 0; i < dataECC.size(); i++) {
            if (waitDelay() == NAND_TIMEOUT) {
                return NAND_TIMEOUT;
            }
            dataECC[i] = readData();
        }

        for (size_t chunk = 0; chunk < dataChunks; chunk++) {
            etl::span<uint8_t, (WriteChunkSize + NumECCBytes)> thisChunkWithECC = {};
            etl::move((dataECC.begin() + (chunk * (WriteChunkSize + NumECCBytes))),
                      (dataECC.begin() + ((chunk + 1) * (WriteChunkSize + NumECCBytes))), thisChunkWithECC.begin());
            etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                    dataChunker(thisChunkWithECC, 0));
            etl::array<uint8_t, WriteChunkSize> returnedData = detectCorrectECCError(thisChunkWithECC,
                                                                                     genECC);       // TODO: use etl::expected
            if (returnedData == etl::array<uint8_t, WriteChunkSize>{}) {
                return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
            } else etl::move(returnedData.begin(), returnedData.end(), (data.begin() + (chunk * WriteChunkSize)));
        }
        return 0;
    }

    /* This function is the more abstract version of the write function where the user does not need to worry about the
     * size of the data and where they'll be fitted. The position or page the data are written to is configurable
     * by the user.*/
    template<unsigned int size, Structure *pos, AddressConfig op>
    uint8_t abstactReadNAND(etl::array<uint8_t, size> data) {          // TODO: use etl::expected
        static_assert(!isValidStructure(pos, op), "The Structure provided by the user is not valid");
        constexpr uint32_t quotient = size / WriteChunkSize;
        constexpr uint32_t readChunks = (size - (quotient * WriteChunkSize)) > 0 ? (quotient + 1) : quotient;
        constexpr uint32_t pagesToRead = readChunks / MaxChunksPerPage;

        if (op == PAGE || op == PAGE_BLOCK) {
            for (size_t page = 0; page < pagesToRead; page++) {
                for (size_t chunk = 0; chunk < MaxChunksPerPage; chunk++) {
                    etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                    uint8_t result = readNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToRead);
                    if (result != 0) return result;
                    etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                            dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                    etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                    if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                        return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                    }
                    etl::move(finalData.begin(), finalData.end(),
                              data.begin() + (WriteChunkSize * (chunk + (page * MaxChunksPerPage))));
                }
                if ((pos->page + 1) != NumPagesBlock) {
                    ++pos->page;
                } else if ((pos->block + 1) != MaxNumBlock) {
                    ++pos->block;
                } else pos->LUN = pos->LUN == 0 ? 1 : 0;
            }
            constexpr uint8_t chunksLeft = readChunks - (MaxChunksPerPage * pagesToRead);
            for (size_t i = 0; i < chunksLeft; i++) {
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                uint8_t result = readNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToRead);
                if (result != 0) return result;
                etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                        dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                    return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                }
                etl::move(finalData.begin(), finalData.end(),
                          data.begin() + (WriteChunkSize * (i + (pagesToRead * MaxChunksPerPage))));
            }
        } else {
            pos->page = (op == POS) ? (pos->position / PageSizeBytes) : pos->page;
            pos->block = (op == POS) ? (pos->page / NumPagesBlock) : pos->block;
            constexpr uint16_t column = pos->position - (pos->page * PageSizeBytes);
            constexpr uint8_t chunksFitThisPage = (PageSizeBytes - column) / (WriteChunkSize + NumECCBytes);
            for (size_t i = 0; i < chunksFitThisPage; i++) {
                etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                uint8_t result = readNAND<(WriteChunkSize + NumECCBytes), pos, op>(dataToRead);
                if (result != 0) return result;
                etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                        dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                    return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                }
                etl::move(finalData.begin(), finalData.end(),
                          data.begin() + (WriteChunkSize * i));
                if (i != (chunksFitThisPage - 1)) {
                    pos->position += (WriteChunkSize + NumECCBytes);
                } else break;
            }
            pos->position = 0;
            if (chunksFitThisPage < readChunks) {
                constexpr uint32_t pagesToWriteLeft =
                        pagesToRead - ((readChunks - chunksFitThisPage) / MaxChunksPerPage);
                for (size_t page = 0; page < pagesToWriteLeft; page++) {
                    if ((pos->page + 1) != NumPagesBlock) {
                        ++pos->page;
                    } else if ((pos->block + 1) != MaxNumBlock) {
                        ++pos->block;
                    } else pos->LUN = pos->LUN == 0 ? 1 : 0;
                    uint32_t chunksFitTillThisPage = chunksFitThisPage + (page * MaxChunksPerPage);
                    for (size_t chunk = 0;
                         (chunk < (readChunks - chunksFitTillThisPage) ||
                          (chunk < MaxChunksPerPage)); chunk++) {
                        etl::array<uint8_t, (WriteChunkSize + NumECCBytes)> dataToRead = {};
                        uint8_t result = readNAND<size, pos, PAGE_BLOCK>(dataToRead);
                        if (result != 0) return result;
                        etl::array<uint8_t, NumECCBytes> genECC = generateECCBytes(
                                dataChunker(etl::span<uint8_t, (WriteChunkSize + NumECCBytes)>(dataToRead), 0));
                        etl::array<uint8_t, WriteChunkSize> finalData = detectCorrectECCError(dataToRead, genECC);
                        if (finalData == etl::array<uint8_t, WriteChunkSize>{}) {
                            return ECC_ERROR; // TODO: return the equivalent error from detectCorrectECCError
                        }
                        etl::move(finalData.begin(), finalData.end(),
                                  data.begin() + (WriteChunkSize *
                                                  (chunk + chunksFitTillThisPage)));
                    }
                }

            }

        }
        return 0;
    }

    uint8_t writeNAND(Structure *pos, AddressConfig op, etl::span<uint8_t> data, uint64_t size);

    uint8_t abstractWriteNAND(Structure *pos, AddressConfig op, etl::span<const uint8_t> data, uint64_t size);

    uint8_t readNAND(Structure *pos, AddressConfig op, etl::span<uint8_t> data, uint64_t size);

    uint8_t abstractReadNAND(Structure *pos, AddressConfig op, etl::span<uint8_t> data, uint64_t size);

};
