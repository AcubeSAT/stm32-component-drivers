#include "HAL_I2C.hpp"


 I2CError HAL_I2C::writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData){
    waitForResponse(); // Ensure bus is not busy
    if (!TWIHS_Write(deviceAddress, i2cData.data(), i2cData.size())) {
         auto error = TWIHS_ErrorGet();
         LOG_INFO << "I2C was not able to perform Write transaction, error: "<<error;
         return I2CError::WriteError;
    }

    return I2CError::None;
}
