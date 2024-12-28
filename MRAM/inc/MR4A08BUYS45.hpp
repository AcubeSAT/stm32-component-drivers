#pragma once

#include "SMC.hpp"
#include "etl/span.h"
enum class MRAM_Errno : uint8_t{
    MRAM_NONE = 0,
    MRAM_TIMEOUT = 1,
    MRAM_ADDRESS_OUT_OF_BOUNDS = 2,
    MRAM_READY= 3,
    MRAM_NOT_READY = 4
};

class MRAM : public SMC {
private:
    /**
     * The MRAM has a total of 21 address pins, organised as 2,097,152 (2^21) words of 8 bits.
     * The device can store data from address 0 to 2^21-1.
     * Primarily used for error detection and loop limits.
     */

    static constexpr uint8_t customIDSize = 4;
    static constexpr uint8_t customID[customIDSize] = {0xDE, 0xAD, 0xBE, 0xEF};

    static constexpr uint32_t maxAllowedAddress   = 0x1FFFFF;                        // 0x200000-1
    static constexpr uint32_t customMRAMIDAddress = maxAllowedAddress-customIDSize;  // Custom ID start Address
    static constexpr uint32_t maxWriteableAddress = customMRAMIDAddress-1;           // Max allowed address for R/W data

    /**
     * Nominally used only once to write the custom ID address
     * on the predefined addresses
     */
    void writeID(void);
    /**
     * @param idArray: The custom ID address
     * Used to read and check the custom ID from the
     * predefined addresses in the MRAM module
     */
    bool checkID(uint8_t* idArray);

public:
    /**
     * Utilizes the \ref SMC constructor to initialize the \ref moduleBaseAddress constant.
     * @param chipSelect Number of the Chip Select used for enabling the MRAM.
     */
    constexpr MRAM(ChipSelect chipSelect) : SMC(chipSelect) {}

    // R/W operations

    /**
     * Basic 8-bit write that takes advantage of the \ref moduleBaseAddress and creates an abstraction
     * to receive as a parameter directly an MRAM address.
     * @param dataAddress MRAM address to write to.
     * @param data 8-bit data to write to the address.
     */
    MRAM_Errno mramWriteByte(uint32_t  dataAddress, const uint8_t &data);

    /**
     * Basic 8-bit read that takes advantage of the \ref moduleBaseAddress and creates an abstraction
     * to receive as a parameter directly an MRAM address.
     * @param dataAddress MRAM address to write to.
     * @return 8-bit data saved in that address.
     */
    MRAM_Errno mramReadByte(uint32_t dataAddress, uint8_t &data);

    /**
     * @param startAddress Base MRAM address to write to. Data will be written starting from startAddress to
     * startAddress + sizeOfData - 1.
     * @param data Array of bytes to write to the memory module.
     * @param sizeOfData Number of bytes to write.
     */
    MRAM_Errno mramWriteData(uint32_t startAddress, etl::span<const uint8_t> &data);

    /**
     * @param startAddress Base MRAM address to read from. Data will be read starting from startAddress to
     * startAddress + sizeOfData - 1.
     * @param data Array of bytes to store the data read from the memory module.
     * @param sizeOfData Number of bytes to read.
     */
    MRAM_Errno mramReadData(uint32_t startAddress, etl::span<uint8_t> data);

    // HealthChecks & Handlers
    MRAM_Errno isMRAMAlive();

    void errorHandler(MRAM_Errno error);

};