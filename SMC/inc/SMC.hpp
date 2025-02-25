#pragma once

#include <cstdint>
#include "samv71q21b.h"
#include "mutex_Handler.h"

/**
 * Implementation of the basic functions handling the Static Memory Controller (SMC) of ATSAMV71.
 * This class is intended to be used as a base class for all implementations utilizing the Static Memory Controller,
 * as the drivers for the NAND Flash and MRAM memory modules of the On-board Computer.
 * The class contains the necessary functions to read and write a single byte to and from a specific
 * address of the External Bus Interface (EBI) inside a memory area dictated by the \ref Chip Select
 * used for enabling the external module such as the NAND Flash.
 */
class SMC {
protected:
    /**
     * Base address of the memory area mapped to the memory addresses of the external module.
     */
    const uint32_t moduleBaseAddress = 0;

    /**
     * End address of the memory area mapped to the memory addresses of the external module.
     */
    const uint32_t moduleEndAddress = 0;

public:
    /**
     * Available Chip Select pins on the ATSAMV71 MCU.
     */
    enum ChipSelect : uint8_t {
        NCS0 = 0,
        NCS1 = 1,
        NCS2 = 2,
        NCS3 = 3,
    };

protected:
    /**
     * Initialize the \ref moduleBaseAddress constant & \ref moduleEndAddress constant.
     * @param chipSelect Number of the Chip Select used for enabling the external module.
     */
    constexpr SMC(ChipSelect chipSelect) : moduleBaseAddress(smcGetBaseAddress(chipSelect)),
                                           moduleEndAddress(smcGetEndAddress(chipSelect)) {}

    /**
     * Basic 8-bit write to an EBI address.
     * @param dataAddress EBI address to write to.
     * @param data 8-bit data to write to the address.
     */
    void smcWriteByte(uint32_t dataAddress, uint8_t data) {
        while (!takeSemaphoreGroup(smcMutex)) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        *(reinterpret_cast<volatile uint8_t *>(dataAddress)) = data;

        releaseSemaphoreGroup(smcMutex);
    }

    /**
     * Basic 8-bit read from an EBI address.
     * @param dataAddress EBI address to read from.
     * @return 8-bit data saved in that address.
     */
    uint8_t smcReadByte(uint32_t dataAddress) {
        uint8_t result;

        while (!takeSemaphoreGroup(smcMutex)) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        result = *(reinterpret_cast<volatile uint8_t *>(dataAddress));
        releaseSemaphoreGroup(smcMutex);

        return result;
    }

    /**
     * @param chipSelect Number of the Chip Select used for enabling the external module.
     * @return Base address on the EBI peripheral that the Chip Select corresponds to.
     */
    static inline constexpr uint32_t smcGetBaseAddress(ChipSelect chipSelect) {
        switch (chipSelect) {
            case NCS0:
                return EBI_CS0_ADDR;

            case NCS1:
                return EBI_CS1_ADDR;

            case NCS2:
                return EBI_CS2_ADDR;

            case NCS3:
                return EBI_CS3_ADDR;

            default:
                return 0;
        }
    }

    /**
     * @param chipSelect Number of the Chip Select used for enabling the external module.
     * @return End address on the EBI peripheral that the Chip Select corresponds to.
     */

    static inline constexpr uint32_t smcGetEndAddress(ChipSelect ChipSelect){
        switch (ChipSelect) {
            case NCS0:
                return EBI_CS0_ADDR | 0x00FFFFFF;

            case NCS1:
                return EBI_CS1_ADDR | 0x00FFFFFF;

            case NCS2:
                return EBI_CS2_ADDR | 0x00FFFFFF;

            case NCS3:
                return EBI_CS3_ADDR | 0x0FFFFFFF;

            default:
                return 0;
        }
    }
};