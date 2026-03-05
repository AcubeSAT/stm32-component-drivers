#include "LM75.hpp"

LM75Sensor::LM75Sensor() {
};

etl::expected<float,LM75Sensor::Error> LM75Sensor::getTemperature() {
    etl::array<uint8_t, 1> buffer{};
    buffer[0] = Lm75Reg;
    etl::array<uint8_t, 2> i2cData{};
    if (auto error = write(buffer); error != Error::NONE) {
        return etl::unexpected(error);
    }
    if (auto error = read(i2cData); error != Error::NONE) {
        return etl::unexpected(error);
    }
    float temperature = parseTemperature(i2cData[0], i2cData[1]);
    if (temperature < TempMin || temperature > TempMax) {
        return etl::unexpected(Error::INVALID_READ);
    }
    return temperature;
}



LM75Sensor::Error LM75Sensor::read(etl::span<uint8_t> i2cData) {

    return convertI2cError(HAL_I2C::readRegister<PeripheralNumber>(Lm75Addr, i2cData));
};

LM75Sensor::Error LM75Sensor::write(etl::span<uint8_t> buffer) {

    return convertI2cError(HAL_I2C::writeRegister<PeripheralNumber>(Lm75Addr, buffer));
};

LM75Sensor::Error convertI2cError(HAL_I2C::I2CError error) {
    switch (error) {
    case HAL_I2C::I2CError::NONE:
        return LM75Sensor::Error::NONE;
    case HAL_I2C::I2CError::BUSY:
        return LM75Sensor::Error::BUSY;
    case HAL_I2C::I2CError::TIMEOUT:
        return LM75Sensor::Error::TIMEOUT;
    case HAL_I2C::I2CError::INVALID_PARAMS:
        return LM75Sensor::Error::INVALID_PARAMS;
    case HAL_I2C::I2CError::OPERATION_ERROR:
        return LM75Sensor::Error::OPERATION_ERROR;
    default:
        return LM75Sensor::Error::UNKNOWN_ERROR;
    }
}


float LM75Sensor::parseTemperature(uint8_t msb, uint8_t lsb) {
    uint32_t raw = (static_cast<uint32_t>(msb) << 8) | lsb;
    bool sign = raw & (1 << 15);
    raw &= ~(1 << 15);
    raw >>= 7;
    return ((static_cast<int8_t>(!sign) * 2) - 1) * raw * 0.5f;
}
