#pragma once

#include <cstdint>
#include "samv71q21b.h"

/**
 * Implementation of the basic functions handling the Static Memory Controller (SMC) of ATSAMV71.
 * This class is intended to be used as a base class for all implementations utilizing the Static Memory Controller,
 * as the drivers for the NAND Flash and MRAM memory modules of the On-board Computer.
 * The class contains the necessary functions to read and write a single byte to and from a specific
 * address of the External Bus Interface (EBI) inside a memory area dictated by the \ref Chip Select
 * used for enabling the external module such as the NAND Flash.
 */
class SMC {
private:
    /**
     * Base address of the memory area mapped to the memory addresses of the external module.
     */
    const uint32_t moduleBaseAddress = 0;

public:
    /**
     * Initialize the \ref moduleBaseAddress constant.
     * @param chipSelect Number of the Chip Select used for enabling the external module.
     */
    SMC(uint8_t chipSelect) : moduleBaseAddress(smcGetBaseAddress(chipSelect)) {}

    /**
     * Basic 8-bit write to an EBI address.
     * @param dataAddress EBI address to write to.
     * @param data 8-bit data to write to the address.
     */
    inline void smcDataWrite(uint32_t dataAddress, uint8_t data) {
        *(reinterpret_cast<volatile uint8_t *>(dataAddress)) = data;
    }

    /**
     * Basic 8-bit read from an EBI address.
     * @param dataAddress EBI address to read from.
     * @return 8-bit data saved in that address.
     */
    inline uint8_t smcDataRead(uint32_t dataAddress) {
        return *(reinterpret_cast<volatile uint8_t *>(dataAddress));
    }

    /**
     * @param chipSelect Number of the Chip Select used for enabling the external module.
     * @return Base address on the EBI peripheral that the Chip Select corresponds to.
     */
    static inline uint32_t smcGetBaseAddress(uint8_t chipSelect) {
        uint32_t dataAddress = 0; // TODO: add error handling in case chipSelect out of bounds.

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
                break;
        }
        return dataAddress;
    }
};
