#pragma once
#include <cstdint>
#include <HAL_I2C.hpp>
#include <etl/span.h>

class LM75Sensor {
public:

    enum class Error {
        NONE,
        /**
         * Internal error during I2C write or read
         */
        OPERATION_ERROR,
        /**
         * Provided parameters were invalid
         */
        INVALID_PARAMS,
        /**
         * The operation took to long to complete
         */
        TIMEOUT,
        /**
         * A previous operation is still ongoing
         */
        BUSY,
        /**
         * Temperature value is out of boundary
         */
        INVALID_READ,
        UNKNOWN_ERROR
    };

    LM75Sensor();
    etl::expected<float, Error> getTemperature();

    /** Takes the 2 byte binary value and converts it to decimal according to the datasheet
     * @note Bits 7-15 contain the values to be converted, 0-6 are irrelevant.
     */

private:
    /** Maximum temperature that can be measured by LM75 **/
    constexpr static uint8_t TempMax = 125;
    /** Minimum temperature that can be measured by LM75 **/
    constexpr static uint8_t TempMin = -55;
    /** Address of LM75 device **/
    constexpr static uint8_t Lm75Addr = 0x48;
    /** Address of register holding the temperature read **/
    constexpr static uint8_t Lm75Reg = 0x00;
    constexpr static auto peripheralNumber = HAL_I2C::PeripheralNumber::TWIHS0;
    uint16_t tempRead;
    float temp;
    float parseTemperature(uint8_t msb, uint8_t lsb);
    Error write(etl::span<uint8_t> buf);
    Error read(etl::span<uint8_t> buf);
    Error convertI2cError(HAL_I2C::I2CError error);

};



