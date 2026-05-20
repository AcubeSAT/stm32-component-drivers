#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "Logger.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "Peripheral_Definitions.hpp"
#include "plib_twihs_master_common.h"

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
        if (Internal::errorGet<peripheralNumber>() != TWIHS_ERROR_NONE) {
            return I2CError::OPERATION_ERROR;
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
        if (Internal::errorGet<peripheralNumber>() != TWIHS_ERROR_NONE) {
            return I2CError::OPERATION_ERROR;
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
        if (Internal::errorGet<peripheralNumber>() != TWIHS_ERROR_NONE) {
            return I2CError::OPERATION_ERROR;
        }

        return I2CError::NONE;
    }
    namespace BitBang {

        /**
         * @brief Configuration for the software bit-bang I2C fallback.
         *
         * Describes which GPIO pins to use and how fast to toggle them.
         * The caller (e.g. AntS driver) provides the actual pin values —
         * HAL_I2C stays generic and never hardcodes hardware specifics.
         *
         * @note Both pins must support open-drain mode. On our case PB4/PB5
         * are open-drain capable and shared with TWIHS1.
         */
        struct Config {
            PIO_PIN  sclPin;   ///< GPIO pin for SCL (clock line)
            PIO_PIN  sdaPin;   ///< GPIO pin for SDA (data line)
            uint32_t delayUs;  ///< Half-period delay in microseconds.

        };
        namespace Internal {

            /**
             * @brief Busy-wait delay in microseconds.
             *
             * Used to control SCL clock speed during bit-bang.
             * On SAME70 at 300MHz, each NOP ≈ 3.3ns so we need
             * roughly 300 NOPs per microsecond.
             */
            inline void delay(uint32_t us) {
                volatile uint32_t count = us * 300;
                while (count--) {
                    __NOP();         ///< no operation
                }
            }

            /**
             * @brief Drive SCL high (release line — open drain).
             */
            inline void sclHigh(const Config& cfg) {
                PIO_PinInputEnable(cfg.sclPin);  ///< open-drain: release = input = pulled high
            }

            /**
             * @brief Drive SCL low.
             */
            inline void sclLow(const Config& cfg) {
                PIO_PinOutputEnable(cfg.sclPin);
                PIO_PinClear(cfg.sclPin);
            }

            /**
             * @brief Drive SDA high (release line — open drain).
             */
            inline void sdaHigh(const Config& cfg) {
                PIO_PinInputEnable(cfg.sdaPin);  ///< open-drain: release = input = pulled high
            }

            /**
             * @brief Drive SDA low.
             */
            inline void sdaLow(const Config& cfg) {
                PIO_PinOutputEnable(cfg.sdaPin);
                PIO_PinClear(cfg.sdaPin);
            }

            /**
             * @brief Read current SDA line state.
             * @return true if SDA is high, false if low.
             */
            inline bool sdaRead(const Config& cfg) {
                PIO_PinInputEnable(cfg.sdaPin);
                return PIO_PinRead(cfg.sdaPin);
            }

            /**
             * @brief Issue an I2C START condition.
             *
             * SDA falls while SCL is high — this is the START signal
             * that tells all slaves a transaction is beginning.
             *
             *   SDA: ‾‾‾\___
             *   SCL: ‾‾‾‾‾‾‾
             */
            inline void start(const Config& cfg) {
                sdaHigh(cfg); delay(cfg.delayUs);
                sclHigh(cfg); delay(cfg.delayUs);
                sdaLow(cfg);  delay(cfg.delayUs);  ///< SDA falls while SCL high → START
                sclLow(cfg);  delay(cfg.delayUs);
            }

            /**
             * @brief Issue an I2C STOP condition.
             *
             * SDA rises while SCL is high — this tells all slaves
             * the transaction is complete and the bus is free.
             *
             *   SDA: ___/‾‾‾
             *   SCL: ‾‾‾‾‾‾‾
             */
            inline void stop(const Config& cfg) {
                sdaLow(cfg);  delay(cfg.delayUs);
                sclHigh(cfg); delay(cfg.delayUs);
                sdaHigh(cfg); delay(cfg.delayUs);  ///< SDA rises while SCL high → STOP
            }

            /**
             * @brief Write one byte over bit-bang I2C and check for ACK.
             *
             * Sends 8 bits MSB first, then releases SDA and checks
             * whether the slave pulled it low (ACK) or left it high (NACK).
             *
             * @param byte the byte to send
             * @return true if ACK received, false if NACK
             */
            inline bool writeByte(const Config& cfg, uint8_t byte) {
                for (int8_t i = 7; i >= 0; i--) {
                    (byte & (1 << i)) ? sdaHigh(cfg) : sdaLow(cfg);
                    delay(cfg.delayUs);
                    sclHigh(cfg); delay(cfg.delayUs);
                    sclLow(cfg);  delay(cfg.delayUs);
                }
                // release SDA and read ACK from slave
                sdaHigh(cfg);
                sclHigh(cfg); delay(cfg.delayUs);
                bool ack = !sdaRead(cfg);  ///< ACK = slave pulls SDA LOW
                sclLow(cfg);  delay(cfg.delayUs);
                return ack;
            }

            /**
             * @brief Read one byte over bit-bang I2C.
             *
             * Releases SDA and clocks in 8 bits MSB first, then sends
             * ACK or NACK depending on whether more bytes are expected.
             *
             * @param sendAck true if more bytes follow, false for last byte
             * @return the byte read from the slave
             */
            inline uint8_t readByte(const Config& cfg, bool sendAck) {
                uint8_t byte = 0;
                sdaHigh(cfg);  // release SDA so slave can drive it
                for (int8_t i = 7; i >= 0; i--) {
                    sclHigh(cfg); delay(cfg.delayUs);
                    if (PIO_PinRead(cfg.sdaPin)) byte |= (1 << i);
                    sclLow(cfg);  delay(cfg.delayUs);
                }
                sendAck ? sdaLow(cfg) : sdaHigh(cfg);
                sclHigh(cfg); delay(cfg.delayUs);
                sclLow(cfg);  delay(cfg.delayUs);
                sdaHigh(cfg);  // release SDA
                return byte;
            }

        } // namespace Internal

    } // namespace BitBang
}

