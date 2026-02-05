#include "LM75.hpp"
#include <Logger.hpp>

LM75Sensor::LM75Sensor() {
};

void LM75Sensor::updateTemp() {
    etl::array<uint8_t, 4> buf = Lm75Reg;
    write(buf);
    read(buf);
    temp = parseTemperature(buf[0],buf[1]);
}


void LM75Sensor::logTemp() {
    LOG_INFO << "The temperature is" << temp << "degC";
}

LM75Sensor::Error LM75Sensor::read(etl::span<uint8_t> buf[0]) {
    //placeholder
    return switchErrorRead(HAL_I2C::readRegister<HAL_I2C::PeripheralNumber::TWIHS0>(Lm75Addr, buf));
};

LM75Sensor::Error LM75Sensor::write(etl::span<uint8_t> i2cData) {
    //placeholder
    return switchErrorWrite(HAL_I2C::writeRegister<HAL_I2C::PeripheralNumber::TWIHS0>(Lm75Addr, i2cData));
};

LM75Sensor::Error switchErrorRead(HAL_I2C::I2CError error) {
    switch (error) {

    default:return LM75Sensor::Error::UNKNOWN_ERROR;
    }
}

LM75Sensor::Error switchErrorWrite(HAL_I2C::I2CError error) {
    switch (error) {

    default:return LM75Sensor::Error::UNKNOWN_ERROR;
    }
}

float LM75Sensor::parseTemperature(uint8_t msb, uint8_t lsb)
{
    int16_t raw = (msb << 8) | lsb;
    raw >>= 7;
    return raw * 0.5f;
}
