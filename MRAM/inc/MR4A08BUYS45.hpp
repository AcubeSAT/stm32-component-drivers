#pragma once
#include "Peripheral_Definitions.hpp"

class MRAM {
public:

    static inline void smcDataWrite(uint32_t dataAddress, uint8_t data)
    {
        *(reinterpret_cast<volatile uint8_t *>(MRAM_BASE_ADDRESS + dataAddress)) = data;
    }

    static inline uint8_t smcDataRead(uint32_t dataAddress)
    {
        return *(reinterpret_cast<volatile uint8_t *>(MRAM_BASE_ADDRESS + dataAddress));
    }
};

//inline std::optional<MRAM> mram;