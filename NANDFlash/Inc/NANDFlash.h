#ifndef STM32_COMPONENT_DRIVERS_NANDFLASH_H
#define STM32_COMPONENT_DRIVERS_NANDFLASH_H

#include <cstdint>
#include "stm32l4xx_hal.h"

class NANDFlash {
    /**
     * Pointer to the NAND Flash FSMC or FMC handle provided by ST's HAL library
     */
    NAND_HandleTypeDef * nand;
public:
    /**
     * Construct a new NAND flash peripheral
     * @param nand A pointer to the handle provided by HAL
     */
    explicit NANDFlash(NAND_HandleTypeDef *nand);

    inline void write(NAND_AddressTypeDef & address, uint8_t * buffer, uint16_t numPagesToWrite) {
        HAL_NAND_Erase_Block(nand, &address);
        HAL_NAND_Write_Page_8b(nand, &address, buffer, numPagesToWrite);
    }

    inline void read(NAND_AddressTypeDef & address, uint8_t * buffer, uint16_t numPagesToRead) {
        HAL_NAND_Read_Page_8b(nand, &address, buffer, numPagesToRead);
    }
};

#endif //STM32_COMPONENT_DRIVERS_NANDFLASH_H
