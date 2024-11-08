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
    inline static I2CError waitForResponse();

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
    static etl::expected<void, I2CError> writeRegister(uint8_t deviceAddress, etl::span<uint8_t> i2cData);

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
    static etl::expected<void, I2CError> readRegister(uint8_t deviceAddress, etl::array<uint8_t, BUFFER_SIZE> &data);

}