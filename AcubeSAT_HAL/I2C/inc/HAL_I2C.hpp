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
    /**
    * @enum I2CError
    * @brief Enumeration to represent various I2C error states.
    */
    enum class I2CError {
        None,
        WriteError,
        ReadError,
        InvalidParams,
        Timeout
    };

    explicit HAL_I2C(uint16_t i2cAddress) : I2C_USER_ADDRESS(
            i2cAddress) {}; //TODO: decide if we are going to use I2C objects

    /**
    * @brief Writes data to a specific I2C device register.
    *
    * @param deviceAddress The I2C address of the target device.
    * @param i2cData The data to be written to the device register.
    * @return I2CError Returns an I2CError indicating success or failure of the write operation.
    *
    * This function initiates a write transaction on the I2C bus. If the bus is busy, it waits
    * for the bus to become available. In case of a failure during the write, it retrieves and logs
    * the error code and returns I2CError::WriteError.
    */
    static etl::expected<void, I2CError> writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (waitForResponse() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS_ErrorGet();
            LOG_INFO << "I2C write transaction failed with error code: : " << error;
            return etl::unexpected(I2CError::WriteError);
        }
        return {};
    }

    /**
     * @brief Reads data from a specific I2C device register.
     *
     * @tparam BUFFER_SIZE The size of the buffer to hold the data.
     * @param deviceAddress The I2C address of the target device.
     * @param data A reference to an array to hold the read data.
     * @return I2CError Returns an I2CError indicating success or failure of the read operation.
     *
     * This function initiates a read transaction on the I2C bus. If the bus is busy, it waits
     * for the read operation to complete. In case of a failure during the read, it retrieves and logs
     * the error code and returns I2CError::ReadError.
     */
    template<uint8_t BUFFER_SIZE>
    static etl::expected<void, I2CError> readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
        static_assert(BUFFER_SIZE > 0, "Buffer size must be greater than zero");
        if (waitForResponse() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS_Read(deviceAddress, data.data(), data.size())) {
            auto error = TWIHS_ErrorGet();
            LOG_ERROR << "I2C read transaction failed with error code: " << error;
            return etl::unexpected(I2CError::ReadError);
        }
        return {};
    }

private:
    const uint8_t I2C_USER_ADDRESS = 0x00;

    /**
     * Timeout duration in ticks for I2C operations.
     * */
    static constexpr uint8_t TIMEOUT_TICKS = 100;

    /**
     * @brief Waits for the I2C bus to become available, with a timeout mechanism.
     *
     * @return I2CError Returns I2CError::Timeout if the bus is busy beyond the allowed timeout.
     *
     * This function checks the bus status to prevent the program from hanging if the I2C device
     * is unresponsive. If the bus remains busy past the TIMEOUT_TICKS threshold, it resets
     * the I2C hardware and returns a timeout error.
     */
    inline static I2CError waitForResponse() {
        auto start = xTaskGetTickCount();
        while (TWIHS_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "I2C timed out ";
                TWIHS_Initialize();
                return I2CError::Timeout;
            }
            taskYIELD();
        }
        return I2CError::None;
    };
};