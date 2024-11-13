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

    /**
     * Wait period before an EFC transaction is skipped.
     */
    static constexpr uint16_t TimeoutTicks = 1000;


    using FlashAddress = uint32_t;
    using FlashData = uint32_t;
    using FlashEraseLength = uint32_t;
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