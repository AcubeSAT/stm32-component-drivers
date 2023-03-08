#pragma once

#include "SMC.hpp"

class MRAM : public SMC {
private:
    /**
     * The MRAM has a total of 21 address pins, organised as 2,097,152 (2^21) words of 8 bits.
     * The device can store data from address 0 to 2^21-1.
     * Primarily used for error detection and loop limits.
     */
    static inline constexpr uint32_t totalNumberOfBytes = 0x200000;

    /**
     * A variable to keep track of the total allocated memory and help in inserting new data.
     */
    uint32_t nextUnallocatedMemoryAddress = 0;

public:
    /**
     * Utilizes the \ref SMC constructor to initialize the \ref moduleBaseAddress constant.
     * @param chipSelect Number of the Chip Select used for enabling the MRAM.
     */
    constexpr MRAM(ChipSelect chipSelect) : SMC(chipSelect) {}

    /**
     * Basic 8-bit write that takes advantage of the \ref moduleBaseAddress and creates an abstraction
     * to receive as a parameter directly an MRAM address.
     * @param dataAddress MRAM address to write to.
     * @param data 8-bit data to write to the address.
     */
    inline void mramWriteByte(uint32_t dataAddress, uint8_t data) {
        smcDataWrite(moduleBaseAddress | dataAddress, data);
    }

    /**
     * Basic 8-bit write that takes advantage of the \ref moduleBaseAddress and creates an abstraction
     * to receive as a parameter directly an MRAM address.
     * @param dataAddress MRAM address to write to.
     * @return 8-bit data saved in that address.
     */
    inline uint8_t mramReadByte(uint32_t dataAddress) {
        return smcDataRead(moduleBaseAddress | dataAddress);
    }

    /**
     *
     * @param dataAddress
     * @param data
     * @param sizeOfData
     */
    void mramWriteData(uint32_t dataAddress, uint8_t *data, uint32_t sizeOfData);

    /**
     *
     * @param dataAddress
     * @param data
     * @param sizeOfData
     */
    void mramReadData(uint32_t dataAddress, uint8_t *data, uint32_t sizeOfData);
};
