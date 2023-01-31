#pragma once

#include <cstdint>
#include "samv71q21b.h"

class SMC {
private:
    const uint32_t moduleBaseAddress = 0;

public:
    SMC(uint8_t chipSelect) : moduleBaseAddress(smcGetBaseAddress(chipSelect)) {}

    inline void smcDataWrite(uint32_t dataAddress, uint8_t data) {
        *(reinterpret_cast<volatile uint8_t *>(dataAddress)) = data;
    }

    inline uint8_t smcDataRead(uint32_t dataAddress) {
        return *(reinterpret_cast<volatile uint8_t *>(dataAddress));
    }

    static inline uint32_t smcGetBaseAddress(uint8_t chipSelect) {
        uint32_t dataAddress = 0;

        switch (chipSelect) {
            case 0:
                dataAddress = EBI_CS0_ADDR;
                break;

            case 1:
                dataAddress = EBI_CS1_ADDR;
                break;

            case 2:
                dataAddress = EBI_CS2_ADDR;
                break;

            case 3:
                dataAddress = EBI_CS3_ADDR;
                break;

            default:
                /*default*/
                break;
        }
        return dataAddress;
    }
};
