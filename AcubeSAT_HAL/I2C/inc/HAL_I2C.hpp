#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "Logger.hpp"
#include "FreeRTOS.h"
#include "task.h"
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
    enum class I2CError : uint8_t {
        NONE,
        /**
         * Internal error during I2C write or read
         */
        OPERATION_ERROR,
        /**
         * Provided parameters were invalid
         */
        INVALID_PARAMS,
        /**
         * The operation took to long to complete
         */
        TIMEOUT,
        /**
         * A previous operation is still ongoing
         */
        BUSY
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
    static constexpr TickType_t TimeoutTicks = 100;

    namespace Internal {
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
        inline bool writeReadRegister(uint16_t deviceAddress, uint8_t * writeData, size_t writeSize, uint8_t* readData, size_t readSize) {
            if constexpr (peripheralNumber == PeripheralNumber::TWIHS0) {
#ifdef TWIHS0_ENABLED
                return TWIHS0_WriteRead(deviceAddress, writeData, writeSize, readData, readSize);

#else
                return false;
#endif
            }
            if constexpr (peripheralNumber == PeripheralNumber::TWIHS1) {
#ifdef TWIHS1_ENABLED
                return TWIHS1_WriteRead(deviceAddress,  writeData, writeSize, readData, readSize);
#else
                return false;
#endif
            }
            if constexpr (peripheralNumber == PeripheralNumber::TWIHS2) {
#ifdef TWIHS2_ENABLED
                return TWIHS2_WriteRead(deviceAddress,  writeData, writeSize, readData, readSize);
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
         * @return Returns false in case of timeout
         *
         * This function checks the bus status to prevent the program from hanging if the I2C device
         * is unresponsive. If the bus remains busy past the TIMEOUT_TICKS threshold, it resets
         * the I2C hardware and returns a timeout error.
         */
        template<PeripheralNumber peripheralNumber>
        inline bool waitForResponse() {
            auto start = xTaskGetTickCount();
            while (isBusy<peripheralNumber>()) {
                if (xTaskGetTickCount() - start > TimeoutTicks) {
                    LOG_ERROR << "I2C timed out";
                    initialize<peripheralNumber>();
                    return false;
                }
            }
            return true;
        }
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
    inline I2CError writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData) {
        if (i2cData.empty()) {
            LOG_ERROR << "I2C data cannot be empty";
            return I2CError::INVALID_PARAMS;
        }

        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::BUSY;
        }
        if (not Internal::writeRegister<peripheralNumber>(deviceAddress, i2cData.data(), i2cData.size())) {
            auto error = Internal::errorGet<peripheralNumber>();
            LOG_INFO << "I2C write transaction failed with error code: " << error;
            return I2CError::OPERATION_ERROR;
        }

        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::TIMEOUT;
        }
        return I2CError::NONE;
    }

    /**
     * @brief Reads data from a specific I2C device register.
     *
     * @param deviceAddress The I2C address of the target device.
     * @param data A span for the buffer that will be populated with the read data.
     * @return I2CError Returns an I2CError indicating success or failure of the read operation.
     *
     * This function initiates a read transaction on the I2C bus. If the bus is busy, it waits
     * for the read operation to complete. In case of a failure during the read, it retrieves and logs
     * the error code and returns I2CError::ReadError.
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError readRegister(uint8_t deviceAddress, etl::span<uint8_t> data) {
        if (data.size() == 0) {
            LOG_ERROR << "I2C data cannot be empty";
            return I2CError::INVALID_PARAMS;
        }

        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::BUSY;
        }
        if (not Internal::readRegister<peripheralNumber>(deviceAddress, data.data(), data.size())) {
            auto error = Internal::errorGet<peripheralNumber>();
            LOG_INFO << "I2C write transaction failed with error code: " << error;
            return I2CError::OPERATION_ERROR;
        }
        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::TIMEOUT;
        }
        return I2CError::NONE;
    }

    /**
     * @brief Performs a combined I2C write/read operation
     *
     * @param deviceAddress The I2C address of the target device.
     * @param writeData A span to a buffer containing the data to write.
     * @param readData A span to a buffer that will be populated with the read data.
     * @return I2CError Returns an I2CError indicating success or failure of the operation.
     *
     * This function performs a combined I2C write/read operation. It waits for the bus to be ready before initiating each transaction
     * and handles timeouts and errors appropriately.
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError writeReadRegister(uint8_t deviceAddress,
                              const etl::span<uint8_t> writeData,
                              etl::span<uint8_t> readData) {
        if (writeData.size() == 0 || readData.size() == 0) {
            LOG_ERROR << "I2C data cannot be empty";
            return I2CError::INVALID_PARAMS;
        }
        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::BUSY;
        }
        if (not Internal::writeReadRegister<peripheralNumber>(deviceAddress, writeData.data(), writeData.size(), readData.data(), readData.size())) {
            auto error =Internal::errorGet<peripheralNumber>();
            LOG_INFO << "I2C write/read transaction failed with error code: " << error;
            return I2CError::OPERATION_ERROR;
        }

        if (not Internal::waitForResponse<peripheralNumber>()) {
            return I2CError::TIMEOUT;
        }

        return I2CError::NONE;
    }
}

