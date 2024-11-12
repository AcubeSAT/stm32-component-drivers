#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"
#include "plib_systick.h"
#include "Peripheral_Definitions.hpp"

#ifdef TWIHS0_ENABLED
#include "plib_twihs0_master.h"
#endif
#ifdef TWIHS1_ENABLED
#include "plib_twihs1_master.h"
#endif
#ifdef TWIHS2_ENABLED

#include "plib_twihs2_master.h"

#endif

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
    * @enum Peripheral
    * @brief Enumeration to represent different I2C peripheral numbers.
    */
    enum class PeripheralNumber : uint8_t {
        TWIHS0 = 0,
        TWIHS1 = 1,
        TWIHS2 = 2
    };

    /**
     * Timeout duration in ticks for I2C operations.
     * */
    static constexpr uint8_t TIMEOUT_TICKS = 100;

    /**
     * @brief Helper functions to map PeripheralNumber enum to specific TWIHS functions
     * @tparam peripheralNumber The I2C peripheral to check, specified by the PeripheralNumber enum.
     * @return true if the operation was sucessful; false otherwise.
     * */
    template<PeripheralNumber peripheralNumber>
    inline bool isBusy() {
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
            return TWIHS0_IsBusy();
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
            return TWIHS1_IsBusy();
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
            return TWIHS2_IsBusy();
#else
            return false;
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    inline void initialize() {
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
            TWIHS0_Initialize();
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
            TWIHS1_Initialize();
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
            TWIHS2_Initialize();
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    inline bool writeRegister(uint16_t deviceAddress, uint8_t *data, size_t size) {
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
            return TWIHS0_Write(deviceAddress, data, size);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
            return TWIHS1_Write(deviceAddress, data, size);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
            return TWIHS2_Write(deviceAddress, data, size);
#else
            return false;
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    inline bool readRegister(uint8_t deviceAddress, uint8_t *data, size_t size) {
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
            return TWIHS0_Read(deviceAddress, data, size);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
            return TWIHS1_Read(deviceAddress, data, size);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
            return TWIHS2_Read(deviceAddress, data, size);
#else
            return false;
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    inline uint32_t errorGet() {
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
            return TWIHS0_ErrorGet();
#else
            return 0;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
            return TWIHS1_ErrorGet();
#else
            return 0;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
            return TWIHS2_ErrorGet();
#else
            return 0;
#endif
        }
    }

    /**
     * @brief Waits for the I2C bus to become available, with a timeout mechanism.
     *
     * @return I2CError Returns I2CError::Timeout if the bus is busy beyond the allowed timeout.
     *
     * This function checks the bus status to prevent the program from hanging if the I2C device
     * is unresponsive. If the bus remains busy past the TIMEOUT_TICKS threshold, it resets
     * the I2C hardware and returns a timeout error.
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError waitForResponse() {
        auto start = SYSTICK_TimerCounterGet();
        while (isBusy<peripheralNumber>()) {
            if (SYSTICK_TimerCounterGet() - start > TIMEOUT_TICKS) {
                LOG_ERROR << "I2C timed out";
                initialize<peripheralNumber>();
                return I2CError::Timeout;
            }
        }
        return I2CError::None;
    }

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
    template<PeripheralNumber peripheralNumber>
    inline etl::expected<void, I2CError> writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (!writeRegister<peripheralNumber>(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = errorGet<peripheralNumber>();
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

    template<PeripheralNumber peripheralNumber, uint8_t BUFFER_SIZE>
    inline etl::expected<void, I2CError> readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
        static_assert(BUFFER_SIZE > 0, "Buffer size must be greater than zero");
        if (waitForResponse<peripheralNumber>() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!readRegister<peripheralNumber>(deviceAddress, data.data(), data.size())) {
            auto error = errorGet<peripheralNumber>();
            LOG_INFO << "I2C write transaction failed with error code: " << error;
            return etl::unexpected(I2CError::ReadError);
        }
        return {};
    }
}