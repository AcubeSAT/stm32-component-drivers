#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"

#include "plib_twihs2_master.h"  //add ifdefs

#define TWIHS_Write TWIHS2_Write
#define TWIHS_ErrorGet TWIHS2_ErrorGet
#define TWIHS_Read TWIHS2_Read
#define TWIHS_Initialize TWIHS2_Initialize
#define TWIHS_IsBusy TWIHS2_IsBusy

class HAL_I2C {
public:
    enum class I2CError {
        None,           // No error
        WriteError,
        ReadError,
        HALError,       // HAL error occurred
        InvalidParams,  // Invalid input parameters
        Timeout         // Operation timed out
    };

    explicit HAL_I2C(uint16_t i2cAddress) : I2C_USER_ADDRESS(
            i2cAddress) {}; //TODO: decide if we are going to use I2C objects

    inline static I2CError writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        waitForResponse(); // Ensure bus is not busy
        if (!TWIHS_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS_ErrorGet();
            LOG_INFO << "I2C was not able to perform WRITE transaction, error: " << error;
            return I2CError::WriteError;
        }

        return I2CError::None;
    }

    template<uint8_t BUFFER_SIZE>
    inline static I2CError readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
        if (!TWIHS_Read(deviceAddress, data.data(), data.size())) {
            waitForResponse(); // Wait for completion
            LOG_INFO << "I2C was not able to perform READ transaction";
            return I2CError::ReadError;
        }
        return I2CError::None; // Return the read data

    };

private:
    const uint8_t I2C_USER_ADDRESS = 0x00;
    static constexpr uint8_t TIMEOUT_TICKS = 100;

    /**
     * Function that prevents hanging when a I2C device is not responding.
     */
    inline static I2CError waitForResponse() {
        volatile auto start = xTaskGetTickCount();
        while (TWIHS_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "Temperature sensor with address  has timed out";
                return I2CError::Timeout;
                TWIHS_Initialize();
            }
            taskYIELD();
        }
        return I2CError::None;
    };
};
