#ifndef MCP9808DRIVER_MCP9808_HPP
#define MCP9808DRIVER_MCP9808_HPP

#include "stm32l4xx_hal.h"
#include "MCP9808-constants.hpp"
#include <cstdint>
#include <arm_math.h>

class MCP9808 {
    /**
     * I2C handle provided by ST
     */
    I2C_HandleTypeDef* i2c;

    private:
        /**
        * Write a value to a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param data the data octets to be written
        */
        void writeReg(uint8_t addr, uint16_t &data);

        void setReg(uint8_t addr, uint16_t mask, uint16_t setting);

    public:
        /**
        * Initialize a new controller object for MCP9808
        * @param i2c handle to the appropriately-set and enabled I2C peripheral
        */
        MCP9808(I2C_HandleTypeDef* i2c);

        /**
        * Read a value from a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param buffer the variable where the data will be stored
        */
        void readReg(uint8_t addr, uint16_t &result);

        /**
         * Set the hysteresis temperature (THYST)
         * @param temp the hysteresis temperature (use the constants in MCP9808-constants.hpp)
         */
        void setHystTemp(uint16_t temp);

        /**
        * Enter/exit low power mode (SHDN - shutdown mode)
        * @param data the appropriate constant from MCP9808-constants.hpp
        */
        void setLowPwrMode(uint16_t data);

        /**
         * Get the current temperature reading (in Celsius)
         * @param result the variable where the result is going to be stored
         */
        void getTemp(float32_t &result);
};

#endif //MCP9808DRIVER_MCP9808_HPP
