#pragma once

#include <cstdint>
#include <plib_pio.h>
#include <plib_systick.h>
#include <etl/span.h>
#include  "peripheral/spi/spi_master/plib_spi0_master.h"
#include  "peripheral/spi/spi_master/plib_spi1_master.h"

/**
 * Namespace to be used for communication with AcubeSAT's component that use SPI.
 */
namespace HAL_SPI {
    /**
     * Enum containing the errors that can appear during SPI transfers.
     */
    enum class Error : uint8_t {
        NONE             = 0,
        INVALID_ARGUMENT = 1,
        TIMEOUT          = 2,
        WRITE_READ_ERROR = 3,
        BUSY             = 4
    };

    /**
     * Enum containing the instances of SPI that ATSAM provides.
     */
    enum class PeripheralNumber : uint8_t {
        SPI0,
        SPI1,
    };

     /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check.
     */
    template <PeripheralNumber peripheralNumber>
    static void initialize() {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            SPI0_Initialize();
#else
            return;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            SPI1_Initialize();
#else
            return;
#endif
        }
    }

    /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check
     * @return false only if the peripheral is ready
     */
    template <PeripheralNumber peripheralNumber>
    static bool isBusy() {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_IsTransmitterBusy();
#else
            return true;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_IsTransmitterBusy();
#else
            return true;
#endif
        }
        return true;
    }

    /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check.
     * @return true if the operation was successful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber>
    bool internalWriteRegister(etl::span<uint8_t> data) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_Write(data.data(), N);
#else
            return false;
#endif
    }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_Write(data.data(), N);
#else
            return false;
#endif
    }
        return false;
}

    /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check.
     * @return true if the operation was succesful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber>
    bool internalReadRegister(etl::span<uint8_t> data) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_Read(data.data(), N);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_Read(data.data(), N);
#else
            return false;
#endif
        }
        return false;
    }

    /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check.
     * @return true if the operation was successful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber>
    bool internalWriteReadRegister(etl::span<uint8_t> transmitData, etl::span<uint8_t> receiveData) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_WriteRead(TransmitData.data(), N, ReceiveData.data(), N);
#else
            return false;
#endif
    }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_WriteRead(TransmitData.data(), N, ReceiveData.data(), N);
#else
            return false;
#endif
    }
        return false;
}

    /**
     * Function to poll SPI peripheral and reset it if it is unresponsive after a time period.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @param timeoutMs Time period after which SPI peripheral is reseted if it is unresponsive.
     * @return NONE is the peripheral responded in time; TIMEOUT otherwise.
     */
    template<PeripheralNumber peripheralNumber>
    Error waitForResponse(const uint32_t timeoutMs) {
        auto start = xTaskGetTickCount();
        while (isBusy<peripheralNumber>()) {
            if (xTaskGetTickCount() - start > timeoutMs) {
                initialize<peripheralNumber>();
                return Error::TIMEOUT;
            }
            taskYIELD();
        }
        return Error::NONE;
    }

    /**
     * Function to only write data.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @param cs The pin used for chip select.
     * @param data An array containing the bytes to be written.
     * @return A member of the SPIError enum; NONE it the transfer was successful.
     */
    template<PeripheralNumber peripheralNumber>
    Error writeRegister(const PIO_PIN cs, etl::span<uint8_t> data) {
        if (waitForResponse<peripheralNumber>(100) != Error::NONE) {
            return Error::BUSY;
        }

        PIO_PinWrite(cs, false);

        auto error = internalWriteRegister<peripheralNumber>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return Error::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }

    /**
     * Function to only read data.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @param cs The pin used for chip select.
     * @param data An array to which the read data is copied.
     * @return A member of the SPIError enum; NONE it the transfer was successful.
     */
    template<PeripheralNumber peripheralNumber>
    Error readRegister(const PIO_PIN cs, etl::span<uint8_t> data) {

        if (waitForResponse<peripheralNumber>(100) != Error::NONE) {
            return Error::BUSY;
        }

        PIO_PinWrite(cs, false);

        auto error = internalReadRegister<peripheralNumber>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return Error::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }

    /**
     * Function to both write and read data on the same SPI transfer.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @param cs The pin used for chip select.
     * @param transmitData An array containing the bytes to be written.
     * @param receiveData An array to which the read data is copied.
     * @return A member of the SPIError enum; NONE it the transfer was successful.
     */
    template<PeripheralNumber peripheralNumber>
    Error writeReadRegister(const PIO_PIN cs, etl::span<uint8_t> transmitData, etl::span<uint8_t> receiveData) {
        if (waitForResponse<peripheralNumber>(100) != Error::NONE) {
            return Error::BUSY;
        }

        PIO_PinWrite(cs, false);

        auto error = internalWriteReadRegister<peripheralNumber>(transmitData, receiveData);

        if (not error) {
            PIO_PinWrite(cs, true);
            return Error::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }
}