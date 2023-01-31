#pragma once

#include "SMC.hpp"

class MRAM : public SMC {
private:
    /**
     *
     */
    static static constexpr uint32_t memoryAddressLimit = 0x1FFFFF;

    /*
     *
     */
    uint32_t nextUnallocatedMemoryAddress = 0;
public:
    /**
     *
     * @param chipSelect
     */
    constexpr MRAM(ChipSelect chipSelect) : SMC(chipSelect) {}

    /**
     *
     * @param dataAddress
     * @param data
     */
    inline void mramDataWrite(uint32_t dataAddress, uint8_t data) {
        smcDataWrite(moduleBaseAddress | dataAddress, data);
    }

    /**
     *
     * @param dataAddress
     * @return
     */
    inline uint8_t mramDataRead(uint32_t dataAddress) {
        return smcDataRead(moduleBaseAddress | dataAddress);
    }
};

//inline std::optional<MRAM> mram;