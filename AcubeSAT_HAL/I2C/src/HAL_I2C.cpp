#include "HAL_I2C.hpp"

namespace HAL_I2C {

    template<uint8_t peripheralNumber>
    inline static I2CError waitForResponse() {
        static_assert(peripheralNumber == 0 || peripheralNumber == 1 || peripheralNumber == 2,
                      "Template parameter must be 0 or 1 or 2");

    }

    template<>
    inline static I2CError waitForResponse<0>() {
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
    inline static I2CError waitForResponse<1>() {
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
    inline static I2CError waitForResponse<2>() {
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

    template<uint8_t peripheralNumber>
    static etl::expected<void, I2CError> writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        static_assert(peripheralNumber == 0 || peripheralNumber == 1 || peripheralNumber == 2,
                      "Template parameter must be 0 or 1 or 2");
    }

    template<>
    static etl::expected<void, I2CError> writeRegister<0>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
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
    static etl::expected<void, I2CError> writeRegister<1>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
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
    static etl::expected<void, I2CError> writeRegister<2>(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return etl::unexpected(I2CError::InvalidParams);
        }
        if (waitForResponse<2>() == I2CError::Timeout) {
            return etl::unexpected(I2CError::Timeout);
        }
        if (!TWIHS2_Write(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = TWIHS2_ErrorGet();
            LOG_INFO << "I2C write transaction failed with error code: : " << error;
            return etl::unexpected(I2CError::WriteError);
        }
        return {};
    }

    template<uint8_t BUFFER_SIZE, uint8_t peripheralNumber>
    static etl::expected<void, I2CError>
    readRegisterImpl(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
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

    template<uint8_t peripheralNumber, uint8_t BUFFER_SIZE>
    static etl::expected<void, I2CError> readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data) {
        return readRegisterImpl<BUFFER_SIZE, peripheralNumber>(deviceAddress, data);
    }

}