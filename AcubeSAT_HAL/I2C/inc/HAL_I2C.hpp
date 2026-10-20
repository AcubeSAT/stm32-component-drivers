#pragma once

#include <cstdint>
#include <etl/expected.h>
#include "etl/span.h"
#include "Logger.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "Peripheral_Definitions.hpp"
#include "plib_twihs_master_common.h"
#include "plib_systick.h"

#ifdef TWIHS0_ENABLED
#include "plib_twihs0_master.h"
#endif
#ifdef TWIHS1_ENABLED
#include "plib_twihs1_master.h"
#endif
#ifdef TWIHS2_ENABLED
#include "plib_twihs2_master.h"
#endif

/**
 * Set to 1 to make the ...WithFallback functions always use bit-bang, for bench testing.
 * Only the ...WithFallback functions are affected. Define it for the whole build, e.g.
 * add_compile_definitions(HAL_I2C_FORCE_BITBANG=1), never in a single file: files built
 * with different values would get different versions of the same inline functions.
 */
#ifndef HAL_I2C_FORCE_BITBANG
#define HAL_I2C_FORCE_BITBANG 0
#endif
#if HAL_I2C_FORCE_BITBANG
#pragma message("HAL_I2C_FORCE_BITBANG=1: the WithFallback functions always use bit-bang")
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
         * A previous operation is still ongoing, or (bit-bang) SDA is held low by something else
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
         * The caller (e.g. the AntS driver) provides the actual pin values,
         * so HAL_I2C stays generic and never hardcodes hardware specifics.
         *
         * delayUs: delay in microseconds between line transitions, sets the
         * SCL speed. Defaults to 5 us, callers can pass a larger value
         * for a slower, more tolerant bus.
         *
         * @note Open-drain is emulated by switching each pin between input (released)
         * and output low, so any PIO pin works and no open-drain mode is needed.
         * Both pins must belong to the TWIHS peripheral when a transfer starts,
         * because they are given back to it at the end. On the OBC these are
         * PB4/PB5, shared with TWIHS1.
         */
        struct Config {
            PIO_PIN  sclPin;
            PIO_PIN  sdaPin;
            uint8_t delayUs = 5;
        };

        /**
         * Longest time in microseconds a slave may hold SCL low (clock stretching)
         * on a single clock before the transfer is aborted with I2CError::TIMEOUT.
         * A failing transfer can take longer than this in total, because the
         * STOP sent afterwards waits for SCL as well.
         */
        static constexpr uint16_t ClockStretchTimeoutUs = 1000;

        /**
         * Maximum number of SCL pulses sent to free a slave that holds SDA low.
         * A slave stuck in the middle of a byte releases SDA after at most 9 clocks.
         */
        static constexpr uint8_t BusRecoveryPulses = 9;

        namespace Internal {

            /**
             * @brief Busy-wait delay in microseconds.
             *
             * Used to control SCL clock speed during bit-bang.
             *
             * @note SYSTICK_DelayUs returns immediately if SysTick is not running, so
             * bit-bang must only be used from a task, after the scheduler has started.
             *
             * @param us the time to wait in microseconds
             */
            inline void delay(uint8_t us) {
                SYSTICK_DelayUs(us);
            }

            /**
             * @brief Release SCL so the external pull-up pulls it high.
             *
             * The pin is switched to input, so SCL is never actively driven high (open-drain).
             * A slave can still hold it low, see releaseSclAndWait().
             */
            inline void releaseScl(const Config& cfg) {
                PIO_PinInputEnable(cfg.sclPin);
            }

            /**
             * @brief Drive SCL low.
             *
             * The output latch is cleared before enabling the output,
             * so the pin never drives the line high, even briefly.
             */
            inline void driveSclLow(const Config& cfg) {
                PIO_PinClear(cfg.sclPin);
                PIO_PinOutputEnable(cfg.sclPin);
            }

            /**
             * @brief Release SDA so the external pull-up pulls it high.
             *
             * The pin is switched to input, so SDA is never actively driven high (open-drain).
             */
            inline void releaseSda(const Config& cfg) {
                PIO_PinInputEnable(cfg.sdaPin);
            }

            /**
             * @brief Drive SDA low.
             *
             * The output latch is cleared before enabling the output,
             * so the pin never drives the line high, even briefly.
             */
            inline void driveSdaLow(const Config& cfg) {
                PIO_PinClear(cfg.sdaPin);
                PIO_PinOutputEnable(cfg.sdaPin);
            }

            /**
             * @brief Read current SCL line state.
             * @return true if SCL is high, false if low.
             */
            inline bool readScl(const Config& cfg) {
                return PIO_PinRead(cfg.sclPin);
            }

            /**
             * @brief Read current SDA line state.
             * @return true if SDA is high, false if low.
             */
            inline bool readSda(const Config& cfg) {
                return PIO_PinRead(cfg.sdaPin);
            }

            /**
             * @brief Release SCL and wait until the line actually reads high.
             *
             * A slave may hold SCL low to make the master wait (clock stretching).
             * Waiting here keeps the next bit from starting while the slave is not ready.
             *
             * @return true when SCL is high, false if it stayed low for ClockStretchTimeoutUs
             */
            inline bool releaseSclAndWait(const Config& cfg) {
                releaseScl(cfg);
                for (uint16_t waitedUs = 0; waitedUs < ClockStretchTimeoutUs; waitedUs++) {
                    if (readScl(cfg)) {
                        return true;
                    }
                    delay(1);
                }
                return readScl(cfg);
            }

            /**
             * @brief Get the PIO controller registers of the port a pin belongs to.
             *
             * Uses the same pin numbering as plib_pio.h: port = pin / 32, bit = pin % 32.
             */
            inline pio_registers_t* getPioRegisters(PIO_PIN pin) {
                return reinterpret_cast<pio_registers_t*>(PIOA_BASE_ADDRESS + (0x200U * (pin >> 5U)));
            }

            /**
             * @brief Get the bit mask of a pin inside its port.
             */
            inline uint32_t getPinMask(PIO_PIN pin) {
                return 0x1UL << (pin & 0x1FU);
            }

            /**
             * @brief Take SCL and SDA from the TWIHS peripheral and give them to the PIO controller.
             *
             * While a pin belongs to the peripheral, the PIO output functions have no effect on it.
             * Both lines are released and their output latches cleared first, so the pins start
             * released and are never driven high.
             */
            inline void takePins(const Config& cfg) {
                releaseScl(cfg);
                releaseSda(cfg);
                PIO_PinClear(cfg.sclPin);
                PIO_PinClear(cfg.sdaPin);
                getPioRegisters(cfg.sclPin)->PIO_PER = getPinMask(cfg.sclPin);
                getPioRegisters(cfg.sdaPin)->PIO_PER = getPinMask(cfg.sdaPin);
            }

            /**
             * @brief Give SCL and SDA back to the TWIHS peripheral.
             *
             * The peripheral function selected at startup is kept by the PIO controller,
             * so the pins go straight back to TWIHS. The TWIHS peripheral itself may
             * need to be initialized again before it is used.
             */
            inline void returnPins(const Config& cfg) {
                releaseScl(cfg);
                releaseSda(cfg);
                getPioRegisters(cfg.sclPin)->PIO_PDR = getPinMask(cfg.sclPin);
                getPioRegisters(cfg.sdaPin)->PIO_PDR = getPinMask(cfg.sdaPin);
            }

            /**
             * @brief Issue an I2C START condition.
             *
             * SDA falls while SCL is high — this is the START signal
             * that tells all slaves a transaction is beginning.
             *
             *   SDA: ‾‾‾\___
             *   SCL: ‾‾‾‾‾‾‾
             *
             * @return I2CError::NONE on success, I2CError::TIMEOUT if SCL stayed low
             */
            inline I2CError start(const Config& cfg) {
                releaseSda(cfg);
                delay(cfg.delayUs);
                if (not releaseSclAndWait(cfg)) {
                    return I2CError::TIMEOUT;
                }
                delay(cfg.delayUs);
                driveSdaLow(cfg);
                delay(cfg.delayUs);
                driveSclLow(cfg);
                delay(cfg.delayUs);
                return I2CError::NONE;
            }

            /**
             * @brief Issue an I2C STOP condition.
             *
             * SDA rises while SCL is high — this tells all slaves
             * the transaction is complete and the bus is free.
             * Expects SCL to be low when called.
             *
             *   SDA: ___/‾‾‾
             *   SCL: ‾‾‾‾‾‾‾
             *
             * @return I2CError::NONE on success, I2CError::TIMEOUT if SCL stayed low
             */
            inline I2CError stop(const Config& cfg) {
                driveSdaLow(cfg);
                delay(cfg.delayUs);
                if (not releaseSclAndWait(cfg)) {
                    return I2CError::TIMEOUT;
                }
                delay(cfg.delayUs);
                releaseSda(cfg);
                delay(cfg.delayUs);
                return I2CError::NONE;
            }

            /**
             * @brief Make sure the bus is free before a transfer, and try to free it if not.
             *
             * Both lines read high when the bus is idle. A slave that was reset or
             * interrupted in the middle of a byte can keep holding SDA low. Clocking SCL
             * up to BusRecoveryPulses times lets it finish that byte and release SDA,
             * and a STOP then returns every slave to idle.
             *
             * @return I2CError::NONE if the bus is free, I2CError::TIMEOUT if SCL is held low,
             * I2CError::BUSY if SDA is still held low after recovery
             */
            inline I2CError freeBus(const Config& cfg) {
                releaseSda(cfg);
                if (not releaseSclAndWait(cfg)) {
                    return I2CError::TIMEOUT;
                }
                delay(cfg.delayUs);
                if (readSda(cfg)) {
                    return I2CError::NONE;
                }

                for (uint8_t pulse = 0; pulse < BusRecoveryPulses && not readSda(cfg); pulse++) {
                    driveSclLow(cfg);
                    delay(cfg.delayUs);
                    if (not releaseSclAndWait(cfg)) {
                        return I2CError::TIMEOUT;
                    }
                    delay(cfg.delayUs);
                }
                if (not readSda(cfg)) {
                    return I2CError::BUSY;
                }

                driveSclLow(cfg);
                delay(cfg.delayUs);
                return stop(cfg);
            }

            /**
             * @brief Write one byte over bit-bang I2C and check for ACK.
             *
             * Sends 8 bits MSB first, then releases SDA and checks
             * whether the slave pulled it low (ACK) or left it high (NACK).
             *
             * Every 1 bit is read back while SCL is high. If SDA reads low even though
             * it was released, something else is holding the line and the byte is aborted.
             * Without this check, an SDA stuck low would look like an ACK.
             *
             * @param byte the byte to send
             * @return I2CError::NONE on ACK, I2CError::OPERATION_ERROR on NACK,
             * I2CError::BUSY if SDA is held low by something else,
             * I2CError::TIMEOUT if SCL stayed low
             */
            inline I2CError writeByte(const Config& cfg, uint8_t byte) {
                for (int8_t i = 7; i >= 0; i--) {
                    const bool bit = (byte & (1 << i)) != 0;
                    if (bit) {
                        releaseSda(cfg);
                    } else {
                        driveSdaLow(cfg);
                    }
                    delay(cfg.delayUs);
                    if (not releaseSclAndWait(cfg)) {
                        return I2CError::TIMEOUT;
                    }
                    delay(cfg.delayUs);
                    if (bit && not readSda(cfg)) {
                        driveSclLow(cfg);
                        return I2CError::BUSY;
                    }
                    driveSclLow(cfg);
                    delay(cfg.delayUs);
                }
                releaseSda(cfg);
                delay(cfg.delayUs);
                if (not releaseSclAndWait(cfg)) {
                    return I2CError::TIMEOUT;
                }
                delay(cfg.delayUs);
                const bool ack = not readSda(cfg);
                driveSclLow(cfg);
                delay(cfg.delayUs);
                if (not ack) {
                    return I2CError::OPERATION_ERROR;
                }
                return I2CError::NONE;
            }

            /**
             * @brief Read one byte over bit-bang I2C.
             *
             * Releases SDA and clocks in 8 bits MSB first, then sends
             * ACK or NACK depending on whether more bytes are expected.
             *
             * After releasing SDA it waits before raising SCL, so the slave's first bit
             * has settled. Otherwise, if SDA rises later than SCL, the slave sees SDA rise
             * while SCL is high, which is a STOP, and the rest of the byte reads as 1s.
             *
             * @param sendAck true if more bytes follow, false for last byte
             * @return the byte read from the slave, or I2CError::TIMEOUT if SCL stayed low
             */
            inline etl::expected<uint8_t, I2CError> readByte(const Config& cfg, bool sendAck) {
                uint8_t byte = 0;
                releaseSda(cfg);
                delay(cfg.delayUs);
                for (int8_t i = 7; i >= 0; i--) {
                    if (not releaseSclAndWait(cfg)) {
                        return etl::unexpected<I2CError>(I2CError::TIMEOUT);
                    }
                    delay(cfg.delayUs);
                    if (PIO_PinRead(cfg.sdaPin)) {
                        byte |= (1 << i);
                    }
                    driveSclLow(cfg);
                    delay(cfg.delayUs);
                }
                if (sendAck) {
                    driveSdaLow(cfg);
                } else {
                    releaseSda(cfg);
                }
                delay(cfg.delayUs);
                if (not releaseSclAndWait(cfg)) {
                    return etl::unexpected<I2CError>(I2CError::TIMEOUT);
                }
                delay(cfg.delayUs);
                driveSclLow(cfg);
                delay(cfg.delayUs);
                releaseSda(cfg);
                return byte;
            }

            /**
             * @brief Build the address byte sent right after START.
             *
             * The 7-bit device address goes in bits [7:1] and the R/W flag in bit 0
             * (0 = write, 1 = read), the same way the TWIHS plib uses the address.
             */
            inline uint8_t makeAddressByte(uint8_t deviceAddress, bool read) {
                return static_cast<uint8_t>((deviceAddress << 1) | static_cast<uint8_t>(read));
            }

            /**
             * @brief Send START (or repeated START) and the address byte.
             *
             * Also works as a repeated START in the middle of a transfer, because after
             * an ACK SCL is low and SDA is released, which is what start() expects.
             *
             * @param read true to address the device for reading, false for writing
             * @return I2CError::NONE if the device ACKed its address, otherwise the error
             */
            inline I2CError startAndAddress(const Config& cfg, uint8_t deviceAddress, bool read) {
                const I2CError result = start(cfg);
                if (result != I2CError::NONE) {
                    return result;
                }
                return writeByte(cfg, makeAddressByte(deviceAddress, read));
            }

            /**
             * @brief Send every byte of data, stopping at the first error.
             *
             * @return I2CError::NONE if every byte was ACKed, otherwise the first error
             */
            inline I2CError writeBytes(const Config& cfg, etl::span<uint8_t> data) {
                I2CError result = I2CError::NONE;
                for (size_t i = 0; i < data.size() && result == I2CError::NONE; i++) {
                    result = writeByte(cfg, data[i]);
                }
                return result;
            }

            /**
             * @brief Read data.size() bytes, stopping at the first error.
             *
             * Every byte is ACKed except the last one, which is NACKed so the device
             * stops sending.
             *
             * @return I2CError::NONE if every byte was read, otherwise the first error
             */
            inline I2CError readBytes(const Config& cfg, etl::span<uint8_t> data) {
                I2CError result = I2CError::NONE;
                for (size_t i = 0; i < data.size() && result == I2CError::NONE; i++) {
                    const bool isLastByte = (i == data.size() - 1);
                    const auto byte = readByte(cfg, not isLastByte);
                    if (byte.has_value()) {
                        data[i] = byte.value();
                    } else {
                        result = byte.error();
                    }
                }
                return result;
            }

            /**
             * @brief Send STOP and pick the error to report for the whole transfer.
             *
             * The STOP is always attempted so the slaves are left idle, even if an
             * earlier step failed.
             *
             * @param result the result of the transfer before the STOP
             * @return result if it is an error, otherwise the result of the STOP
             */
            inline I2CError stopAndReport(const Config& cfg, I2CError result) {
                const I2CError stopResult = stop(cfg);
                if (result != I2CError::NONE) {
                    return result;
                }
                return stopResult;
            }

            /**
             * @brief Run a full write transfer: free the bus, START, address, data, STOP.
             *
             * If freeBus fails, nothing is sent and its error is returned. If a later step
             * fails, the remaining bytes are not sent but a STOP is still attempted so the
             * slaves are left idle. The first error is returned.
             */
            inline I2CError writeTransfer(const Config& cfg, uint8_t deviceAddress, etl::span<uint8_t> data) {
                I2CError result = freeBus(cfg);
                if (result != I2CError::NONE) {
                    return result;
                }

                result = startAndAddress(cfg, deviceAddress, false);
                if (result == I2CError::NONE) {
                    result = writeBytes(cfg, data);
                }
                return stopAndReport(cfg, result);
            }

            /**
             * @brief Run a full read transfer: free the bus, START, address, data, STOP.
             *
             * If freeBus fails, nothing is sent and its error is returned. If a later step
             * fails, a STOP is still attempted so the slaves are left idle.
             * The first error is returned.
             */
            inline I2CError readTransfer(const Config& cfg, uint8_t deviceAddress, etl::span<uint8_t> data) {
                I2CError result = freeBus(cfg);
                if (result != I2CError::NONE) {
                    return result;
                }

                result = startAndAddress(cfg, deviceAddress, true);
                if (result == I2CError::NONE) {
                    result = readBytes(cfg, data);
                }
                return stopAndReport(cfg, result);
            }

            /**
             * @brief Run a full write/read transfer: free the bus, START, address (write),
             * writeData, repeated START, address (read), readData, STOP.
             *
             * There is no STOP between the write and the read, so no other master can
             * take the bus in between. This is the same sequence as the TWIHS writeRead.
             * If freeBus fails, nothing is sent and its error is returned. If a later step
             * fails, a STOP is still attempted so the slaves are left idle.
             * The first error is returned.
             */
            inline I2CError writeReadTransfer(const Config& cfg, uint8_t deviceAddress,
                                              etl::span<uint8_t> writeData, etl::span<uint8_t> readData) {
                I2CError result = freeBus(cfg);
                if (result != I2CError::NONE) {
                    return result;
                }

                result = startAndAddress(cfg, deviceAddress, false);
                if (result == I2CError::NONE) {
                    result = writeBytes(cfg, writeData);
                }
                if (result == I2CError::NONE) {
                    result = startAndAddress(cfg, deviceAddress, true);
                }
                if (result == I2CError::NONE) {
                    result = readBytes(cfg, readData);
                }
                return stopAndReport(cfg, result);
            }
        }

        /**
         * @brief Writes data to an I2C device over bit-bang.
         *
         * Takes the pins from the TWIHS peripheral, frees the bus if a slave is
         * holding it, sends START, the address byte with the write flag, every byte
         * of data and STOP, and then gives the pins back to the peripheral.
         *
         * @param cfg the pins and delay to use
         * @param deviceAddress the 7-bit I2C address of the target device
         * @param data the bytes to send
         * @return I2CError::NONE on success, I2CError::INVALID_PARAMS if data is empty,
         * I2CError::OPERATION_ERROR if the device did not ACK, I2CError::BUSY if SDA is
         * held low by something else, I2CError::TIMEOUT if SCL is held low
         */
        inline I2CError write(const Config& cfg, uint8_t deviceAddress, etl::span<uint8_t> data) {
            if (data.empty()) {
                LOG_ERROR << "I2C data cannot be empty";
                return I2CError::INVALID_PARAMS;
            }

            Internal::takePins(cfg);
            const I2CError result = Internal::writeTransfer(cfg, deviceAddress, data);
            Internal::returnPins(cfg);

            if (result != I2CError::NONE) {
                LOG_ERROR << "Bit-bang I2C write to device " << deviceAddress
                          << " failed with error code: " << static_cast<uint32_t>(result);
            }
            return result;
        }

        /**
         * @brief Reads data from an I2C device over bit-bang.
         *
         * Takes the pins from the TWIHS peripheral, frees the bus if a slave is
         * holding it, sends START and the address byte with the read flag, reads
         * data.size() bytes, sends STOP, and then gives the pins back to the peripheral.
         *
         * @param cfg the pins and delay to use
         * @param deviceAddress the 7-bit I2C address of the target device
         * @param data the buffer to fill, its size sets how many bytes are read
         * @return I2CError::NONE on success, I2CError::INVALID_PARAMS if data is empty,
         * I2CError::OPERATION_ERROR if the device did not ACK its address, I2CError::BUSY
         * if SDA is held low by something else, I2CError::TIMEOUT if SCL is held low
         */
        inline I2CError read(const Config& cfg, uint8_t deviceAddress, etl::span<uint8_t> data) {
            if (data.empty()) {
                LOG_ERROR << "I2C data cannot be empty";
                return I2CError::INVALID_PARAMS;
            }

            Internal::takePins(cfg);
            const I2CError result = Internal::readTransfer(cfg, deviceAddress, data);
            Internal::returnPins(cfg);

            if (result != I2CError::NONE) {
                LOG_ERROR << "Bit-bang I2C read from device " << deviceAddress
                          << " failed with error code: " << static_cast<uint32_t>(result);
            }
            return result;
        }

        /**
         * @brief Writes data to an I2C device and then reads from it over bit-bang.
         *
         * Takes the pins from the TWIHS peripheral, frees the bus if a slave is
         * holding it, sends START, the address byte with the write flag and every byte
         * of writeData, then a repeated START, the address byte with the read flag,
         * reads readData.size() bytes, sends STOP, and gives the pins back to the peripheral.
         * Typically used to write a register pointer and then read that register.
         *
         * @param cfg the pins and delay to use
         * @param deviceAddress the 7-bit I2C address of the target device
         * @param writeData the bytes to send before the repeated START
         * @param readData the buffer to fill, its size sets how many bytes are read
         * @return I2CError::NONE on success, I2CError::INVALID_PARAMS if either buffer is empty,
         * I2CError::OPERATION_ERROR if the device did not ACK, I2CError::BUSY if SDA is
         * held low by something else, I2CError::TIMEOUT if SCL is held low
         */
        inline I2CError writeRead(const Config& cfg, uint8_t deviceAddress,
                                  etl::span<uint8_t> writeData, etl::span<uint8_t> readData) {
            if (writeData.empty() || readData.empty()) {
                LOG_ERROR << "I2C data cannot be empty";
                return I2CError::INVALID_PARAMS;
            }

            Internal::takePins(cfg);
            const I2CError result = Internal::writeReadTransfer(cfg, deviceAddress, writeData, readData);
            Internal::returnPins(cfg);

            if (result != I2CError::NONE) {
                LOG_ERROR << "Bit-bang I2C write/read with device " << deviceAddress
                          << " failed with error code: " << static_cast<uint32_t>(result);
            }
            return result;
        }
    }

    /**
     * @brief Writes data to an I2C device, falling back to bit-bang if the TWIHS peripheral fails.
     *
     * Tries writeRegister() on the TWIHS peripheral first. If it returns an error, the same
     * write is repeated over bit-bang on the pins in cfg. With HAL_I2C_FORCE_BITBANG set to 1
     * the peripheral is skipped and bit-bang is always used.
     *
     * @tparam peripheralNumber the TWIHS peripheral to try first
     * @param cfg the bit-bang pins and delay, used if the peripheral fails
     * @param deviceAddress the 7-bit I2C address of the target device
     * @param data the bytes to send
     * @return I2CError::NONE if either path succeeded, otherwise the bit-bang error
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError writeRegisterWithFallback(const BitBang::Config& cfg, uint8_t deviceAddress,
                                              etl::span<uint8_t> data) {
        if constexpr (HAL_I2C_FORCE_BITBANG == 0) {
            if (writeRegister<peripheralNumber>(deviceAddress, data) == I2CError::NONE) {
                return I2CError::NONE;
            }
            LOG_INFO << "TWIHS write to device " << deviceAddress << " failed, retrying over bit-bang";
        }
        return BitBang::write(cfg, deviceAddress, data);
    }

    /**
     * @brief Reads data from an I2C device, falling back to bit-bang if the TWIHS peripheral fails.
     *
     * Tries readRegister() on the TWIHS peripheral first. If it returns an error, the same
     * read is repeated over bit-bang on the pins in cfg. With HAL_I2C_FORCE_BITBANG set to 1
     * the peripheral is skipped and bit-bang is always used.
     *
     * @tparam peripheralNumber the TWIHS peripheral to try first
     * @param cfg the bit-bang pins and delay, used if the peripheral fails
     * @param deviceAddress the 7-bit I2C address of the target device
     * @param data the buffer to fill, its size sets how many bytes are read
     * @return I2CError::NONE if either path succeeded, otherwise the bit-bang error
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError readRegisterWithFallback(const BitBang::Config& cfg, uint8_t deviceAddress,
                                             etl::span<uint8_t> data) {
        if constexpr (HAL_I2C_FORCE_BITBANG == 0) {
            if (readRegister<peripheralNumber>(deviceAddress, data) == I2CError::NONE) {
                return I2CError::NONE;
            }
            LOG_INFO << "TWIHS read from device " << deviceAddress << " failed, retrying over bit-bang";
        }
        return BitBang::read(cfg, deviceAddress, data);
    }

    /**
     * @brief Performs a combined write/read, falling back to bit-bang if the TWIHS peripheral fails.
     *
     * Tries writeReadRegister() on the TWIHS peripheral first. If it returns an error, the same
     * transfer is repeated over bit-bang on the pins in cfg. With HAL_I2C_FORCE_BITBANG set to 1
     * the peripheral is skipped and bit-bang is always used.
     *
     * @tparam peripheralNumber the TWIHS peripheral to try first
     * @param cfg the bit-bang pins and delay, used if the peripheral fails
     * @param deviceAddress the 7-bit I2C address of the target device
     * @param writeData the bytes to send before the repeated START
     * @param readData the buffer to fill, its size sets how many bytes are read
     * @return I2CError::NONE if either path succeeded, otherwise the bit-bang error
     */
    template<PeripheralNumber peripheralNumber>
    inline I2CError writeReadRegisterWithFallback(const BitBang::Config& cfg, uint8_t deviceAddress,
                                                  etl::span<uint8_t> writeData, etl::span<uint8_t> readData) {
        if constexpr (HAL_I2C_FORCE_BITBANG == 0) {
            if (writeReadRegister<peripheralNumber>(deviceAddress, writeData, readData) == I2CError::NONE) {
                return I2CError::NONE;
            }
            LOG_INFO << "TWIHS write/read with device " << deviceAddress << " failed, retrying over bit-bang";
        }
        return BitBang::writeRead(cfg, deviceAddress, writeData, readData);
    }
}
