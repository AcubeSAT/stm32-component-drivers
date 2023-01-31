#pragma once

#include "SMC.hpp"

class MRAM : public SMC {
public:
    /**
     *
     * @param chipSelect
     */
    MRAM(ChipSelect chipSelect) : SMC(chipSelect) {}

    /*
     *
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