#ifndef STM32_COMPONENT_DRIVERS_I2CPERIPHERAL_H
#define STM32_COMPONENT_DRIVERS_I2CPERIPHERAL_H

#include "stm32l4xx_hal.h"

/**
 * A generic class for an external I2C device on the STM32L4
 */
class I2CDevice {
private:
    /**
     * I2C handle provided by HAL
     */
    I2C_HandleTypeDef *i2c;

protected:
    /**
     * The I2C device address specified by the peripheral
     *
     * The **7 most significant bits** of this address are used. The least significant bit is not
     * taken into account, as I2C addresses are 7 bits wide.
     */
    uint8_t deviceAddress;

    /**
     * Initialize a new controller object for this device
     * @param i2c HAL handle to the appropriately-set and enabled I2C peripheral
     */
    I2CDevice(I2C_HandleTypeDef *i2c);

    /**
     * Write a value to a register
     *
     * @param addr the address of the desired register
     * @param data the data octets to be written
     */
    template<typename DATATYPE>
    void writeReg(uint8_t addr, DATATYPE data);

    /**
     * Safely change a setting on the register
     * This is the recommended function to use when changing settings,
     * and is used in all public functions that change settings.
     *
     * @param addr the address of the desired register
     * @param mask the appropriate bitmask to access the particular
     * setting bit or group of bits
     * @param setting the new value of the setting to be changed
     */
    template<typename DATATYPE>
    void setReg(uint8_t addr, DATATYPE mask, DATATYPE setting);

    /**
     * Read a value from a register
     * @param addr the address of the desired register
     * @param buffer the variable where the data will be stored
     */
    template<typename DATATYPE>
    DATATYPE readReg(uint8_t addr);

public:

};

#endif // STM32_COMPONENT_DRIVERS_I2CPERIPHERAL_H
