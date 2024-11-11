#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"

#include "plib_twihs1_master.h"
#include "plib_twihs0_master.h"
#include "plib_twihs2_master.h"


namespace HAL_I2C {
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
    template<uint8_t peripheralNumber>
    inline I2CError waitForResponse() {
        static_assert(peripheralNumber == 0 || peripheralNumber == 1 || peripheralNumber == 2,
                      "Template parameter must be 0 or 1 or 2");
        return I2CError::None;
    }

    template<>
    inline I2CError waitForResponse<0>() {
        auto start = xTaskGetTickCount();
        while (TWIHS0_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "I2C timed out ";
                TWIHS0_Initialize();
                return I2CError::Timeout;
            }
            taskYIELD();
        }
        return I2CError::None;
    };

    template<>
    inline I2CError waitForResponse<1>() {
        auto start = xTaskGetTickCount();
        while (TWIHS1_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "I2C timed out ";
                TWIHS1_Initialize();
                return I2CError::Timeout;
            }
            taskYIELD();
        }
        return I2CError::None;
    };

    template<>
    inline I2CError waitForResponse<2>() {
        auto start = xTaskGetTickCount();
        while (TWIHS2_IsBusy()) {
            if (xTaskGetTickCount() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "I2C timed out ";
                TWIHS2_Initialize();
                return I2CError::Timeout;
            }
            taskYIELD();
        }
        return I2CError::None;
    };

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
    template<uint8_t peripheralNumber>
    inline etl::expected<void, I2CError> writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        static_assert(peripheralNumber == 0 || peripheralNumber == 1 || peripheralNumber == 2,
                      "Template parameter must be 0 or 1 or 2");
        return {};
    }

    template<>
    inline etl::expected<void, I2CError> writeRegister<0>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (waitForResponse<0>() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS0_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS0_ErrorGet();
            LOG_INFO << "I2C write transaction failed with error code: : " << error;
            return etl::unexpected(I2CError::WriteError);
        }
        return {};

    }

    template<>
    inline etl::expected<void, I2CError> writeRegister<1>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (waitForResponse<1>() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS1_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS1_ErrorGet();
            LOG_INFO << "I2C write transaction failed with error code: : " << error;
            return etl::unexpected(I2CError::WriteError);
        }
        return {};

    }

    template<>
    inline etl::expected<void, I2CError> writeRegister<2>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (waitForResponse<2>() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS2_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS2_ErrorGet();
            LOG_INFO << "I2C write transaction failed with error code: " << error;
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

    template<uint8_t peripheralNumber, uint8_t BUFFER_SIZE>
    inline etl::expected<void, I2CError>
    readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
        static_assert(BUFFER_SIZE > 0, "Buffer size must be greater than zero");
        // Call appropriate TWIHS read function based on peripheralNumber
        if constexpr (peripheralNumber == 0) {
            if (waitForResponse<0>() == I2CError::Timeout) {
                return etl::unexpected(I2CError::Timeout);
            }
            if (!TWIHS0_Read(deviceAddress, data.data(), data.size())) {
                return etl::unexpected(I2CError::ReadError);
            }
        } else if constexpr (peripheralNumber == 1) {
            if (waitForResponse<1>() == I2CError::Timeout) {
                return etl::unexpected(I2CError::Timeout);
            }
            if (!TWIHS1_Read(deviceAddress, data.data(), data.size())) {
                return etl::unexpected(I2CError::ReadError);
            }
        } else if constexpr (peripheralNumber == 2) {
            if (waitForResponse<2>() == I2CError::Timeout) {
                return etl::unexpected(I2CError::Timeout);
            }
            if (!TWIHS2_Read(deviceAddress, data.data(), data.size())) {
                return etl::unexpected(I2CError::ReadError);
            }
        } else {
            static_assert(peripheralNumber <= 2, "Invalid peripheral number");
        }

        return {};
    }

}