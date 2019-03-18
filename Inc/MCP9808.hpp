#ifndef MCP9808DRIVER_MCP9808_HPP
#define MCP9808DRIVER_MCP9808_HPP

#include "stm32l4xx_hal_i2c.h"
#include "stm32l4xx_hal_gpio.h"
#include <cstddef>
#include "MCP9808-constants.hpp"


class MCP9808 {
    /**
     * I2C handle provided by ST
     */
    I2C_HandleTypeDef* i2c;

    public:
        /**
        * Initialize a new controller object for MCP9808
        * @param i2c handle to the appropriately-set and enabled I2C peripheral
        */
        MCP9808(I2C_HandleTypeDef* i2c);

        /**
        * Write a value to a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param data the data octet to be written
        */
        void writeReg(uint8_t addr, uint8_t data);

        /**
        * Read a value from a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param buffer the variable where the data will be stored
        */
        void readReg(uint8_t addr, uint8_t buffer);

};

#endif //MCP9808DRIVER_MCP9808_HPP
