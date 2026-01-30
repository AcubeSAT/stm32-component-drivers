#pragma once
#include <cstdint>
#include <HAL_I2C.hpp>
#include <etl/span.h>

class LM75Sensor{
public:

    enum class Error {
        /**
         * The sensor could not read any data at this time
         */
        NO_DATA_AVAILABLE,
        /**
         * Sensor catastrophic failure, reset required
         */
        HARDWARE_FAULT,
        UNKNOWN_ERROR
    };

    LM75Sensor();

    Error write(etl::span<uint8_t> i2cData);
    Error read(etl::span<uint8_t> buf);
    void updateTemp();
    void logTemp();
    float parseTemperature(uint8_t MSB, uint8_t LSB);
private:
    constexpr static uint8_t Lm75Addr = 0x48;
    constexpr static uint8_t  Lm75Reg = 0x00;
    uint16_t tempRead;
    float temp;
    Error lastError;
    Error switchErrorWrite(HAL_I2C::I2CError error);
    Error switchErrorRead(HAL_I2C::I2CError error);

};



