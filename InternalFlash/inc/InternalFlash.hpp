#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "plib_efc.h"

/**
 * Class to interact with the Internal FLASH Memory of the MCU.
 *
 * @brief This class provides functions with a generic implementation based on the EFC peripheral library for easy integration into any code.
 */
class InternalFlash {
public:
    /**
     * Constructor to create an instance of the class and initialize the EFC peripheral.
     */
    InternalFlash();

    /**
     * Write function that writes 128 bits.
     * @param data pointer to 128-bit data buffer.
     * @param address FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    EFC_ERROR QuadWordWrite(uint32_t* data, uint32_t address);

    /**
     * Write function that writes an entire page of 512 bytes.
     * @param data pointer to data buffer of size equivalent to page size.
     * @param address  	FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    EFC_ERROR PageWrite(uint32_t* data, uint32_t address);

    /**
     * Reads length number of bytes from a given address in FLASH memory into
     * the user buffer.
     * @param data pointer to user data buffer.
     * @param length number of bytes to read.
     * @param address FLASH address to be read from.
     * @return member of the EFC_ERROR enum.
     */
    EFC_ERROR Read(uint32_t* data, uint32_t length, uint32_t address);

    /**
     * This function is used to erase a sector.
     * @param address FLASH address to be Erased.
     * @return member of the EFC_ERROR enum.
     */
    EFC_ERROR SectorErase(uint32_t address);
};