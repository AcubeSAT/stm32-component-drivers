#include "LM75.hpp"
#include <Logger.hpp>

LM75Sensor::LM75Sensor() {
};

void LM75Sensor::updateTemp() {
    etl::span<uint8_t, 4> buf;
    write(buf);
    read(buf);
}


void LM75Sensor::logTemp() {
    LOG_INFO << "The temperature is" << temp << "degC";
}

LM75Sensor::Error LM75Sensor::read(etl::span<uint8_t> buf) {
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