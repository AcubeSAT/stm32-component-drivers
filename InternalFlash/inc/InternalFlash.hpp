#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "plib_efc.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"

/**
 * Class to interact with the Internal FLASH Memory of the MCU.
 *
 * @brief This class provides functions with a generic implementation based on the EFC peripheral library for easy integration into any code.
 */
class FlashDriver {
public:
    using FlashAddress = uint32_t;
    using FlashData = uint32_t;
    using FlashEraseLength = uint32_t;

    /**
     * Wait period before an EFC transaction is skipped.
     */
    static constexpr uint16_t TimeoutTicks = 1000;

    /**
     * Size of a flash page in Bytes.
     */
    static constexpr uint32_t FLASH_PAGE_SIZE = 512;

    /**
     * Size of 4 words (128 bits) in Bytes.
     */
    static constexpr uint32_t QUAD_WORD_SIZE = 16;

    /**
     * Starting adress of available Flash memory.
     */
    static constexpr uint32_t startAddress = 0x5F0000;

    /**
     * End limit of available Flash memory.
     */
    static constexpr uint32_t endAddress = 0x600000;

    /**
     * Constructor to create an instance of the class and initialize the EFC peripheral.
     */
    FlashDriver();

    /**
     * Write function that writes 128 bits. Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased.
     * @param data pointer to 128-bit data buffer.
     * @param address FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    static EFC_ERROR QuadWordWrite(FlashData* data, FlashAddress address);

    /**
     * Write function that writes an entire page of 512 bytes. Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased.
     * @param data pointer to data buffer of size equivalent to page size.
     * @param address  	FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    static EFC_ERROR PageWrite(FlashData* data, FlashAddress address);

    /**
     * Reads length number of bytes from a given address in FLASH memory into
     * the user buffer.
     * @param data pointer to user data buffer.
     * @param length number of bytes to read.
     * @param address FLASH address to be read from.
     * @return member of the EFC_ERROR enum.
     */
    static EFC_ERROR Read(FlashData* data, FlashEraseLength length, FlashAddress address);

    /**
     * This function is used to erase a sector.
     * @param address FLASH address to be Erased.
     * @return member of the EFC_ERROR enum.
     */
    static EFC_ERROR SectorErase(FlashAddress address);

private:
    /**
     * Ensure Flash address used is correctly alligned.
     * @param address at which a function will be called
     * @param alignment TBD
     * @return
     */
    static bool isAligned(FlashAddress address, uint32_t alignment) {
        return (address % alignment) == 0;
    };

    /**
     * Ensure Flash addressed used is within defined limits.
     * @param address at which a function will be called
     * @return true is the address is valid, false if not
     */
    static bool isAddressSafe(FlashAddress address) {
        return (address >= startAddress && address < endAddress);
    };

    static void waitForResponse() {
        auto start = xTaskGetTickCount();
        while (EFC_IsBusy()) {
            if (xTaskGetTickCount() - start > TimeoutTicks) {
                LOG_ERROR << "EFC transaction failed";
                EFC_Initialize();
            }
            taskYIELD();
        }
    };

};
