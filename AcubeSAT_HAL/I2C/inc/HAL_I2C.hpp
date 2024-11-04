#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"
#include "HAL_Definitions.hpp"

#include "plib_twihs2_master.h"  //add ifdefs


#define TWIHS_Write TWIHS2_Write
#define TWIHS_ErrorGet TWIHS2_ErrorGet
#define TWIHS_Read TWIHS2_Read
#define TWIHS_Initialize TWIHS2_Initialize
#define TWIHS_IsBusy TWIHS2_IsBusy

class HAL_I2C {
public:
    explicit HAL_I2C(uint16_t i2cAddress) : I2C_USER_ADDRESS(i2cAddress) {};

    static I2CError writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData);

    template<uint8_t BUFFER_SIZE>
    static I2CError readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE>& data){
        if (!TWIHS_Read(deviceAddress, data.data(), data.size())) {
            waitForResponse(); // Wait for completion
            LOG_INFO << "I2C was not able to perform READ transaction";
            return I2CError::HALError;
        }
        return I2CError::None ; // Return the read data

    };


private:
    const uint8_t I2C_USER_ADDRESS = 0x00;
    static constexpr uint8_t TIMEOUT_TICKS = 100;

    /**
     * Function that prevents hanging when a I2C device is not responding.
     */
    inline static void waitForResponse() {
        auto start = xTaskGetTickCount();
        while (TWIHS_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "Temperature sensor with address  has timed out";
                TWIHS_Initialize();
            }
            taskYIELD();
        }
    };


};
