#pragma once

#include <cstdint>
#include <plib_pio.h>
#include <plib_systick.h>
#include <etl/span.h>
#include "ADCS_Definitions.hpp"
#ifdef  SPI0_ENABLED
#include  "peripheral/spi/spi_master/plib_spi0_master.h"
#endif
#ifdef  SPI1_ENABLED
#include  "peripheral/spi/spi_master/plib_spi1_master.h"
#endif

/**
 * Namespace to be used for communication with AcubeSAT's component that use SPI.
 */
namespace HAL_SPI {
    /**
     * Enum containing the errors that can appear during SPI transfers.
     */
    enum class SPIError : uint8_t {
        NONE             = 0,
        INVALID_ARGUMENT = 1,
        TIMEOUT          = 2,
        WRITE_READ_ERROR = 3,
        BUSY             = 4,
        UNDEFINED        = 255
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
        return;
    }

    /**
     * Helper function to map PeripheralNumber enum to the corresponding SPI function.
     *
     * @tparam peripheralNumber The SPI peripheral to check
     * @return true if the operation was succesful; false otherwise.
     */
    template <PeripheralNumber peripheralNumber>
    static bool isBusy() {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_IsTransmitterBusy();
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_IsTransmitterBusy();
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
     * @tparam N Size of data array in bytes.
     * @return true if the operation was succesful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    bool writeRegister(etl::array<uint8_t, N>& data) {
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
     * @tparam N Size of data array in bytes.
     * @return true if the operation was succesful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    bool readRegister(etl::array<uint8_t, N>& data) {
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
     * @tparam N Size of transmit/receive data arrays in bytes.
     * @return true if the operation was succesful; false otherwise.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    bool writeReadRegister(etl::array<uint8_t, N>& TransmitData, etl::array<uint8_t, N>& ReceiveData) {
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
    SPIError waitForResponse(const uint32_t timeoutMs) {
        auto start = xTaskGetTickCount();
        while (isBusy<peripheralNumber>()) {
            if (xTaskGetTickCount() - start > timeoutMs) {
                initialize<peripheralNumber>();
            }
            taskYIELD();
        }
    };

    /**
     * Function to only write data.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @tparam N Size of data array in bytes.
     * @param cs The pin used for chip select.
     * @param data An array containing the bytes to be written.
     * @return A member of the SPIError enum; NONE it the transfer was succesful.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    SPIError SPIwriteRegister(const PIO_PIN cs, etl::array<uint8_t, N>& data) {
        static_assert(N > 0, "Array size must be greater than zero");

        if (waitForResponse<peripheralNumber>(100) != SPIError::NONE) {
            return SPIError::TIMEOUT;
        }

        PIO_PinWrite(cs, false);

        auto error = writeRegister<peripheralNumber, N>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }

    /**
     * Function to only read data.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @tparam N Size of data array in bytes.
     * @param cs The pin used for chip select.
     * @param data An array to which the read data is copied.
     * @return A member of the SPIError enum; NONE it the transfer was succesful.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    SPIError SPIReadRegister(const PIO_PIN cs, etl::array<uint8_t, N>& data) {
        static_assert(N > 0, "Array size must be greater than zero");

        if (waitForResponse<peripheralNumber>(100) != SPIError::NONE) {
            return SPIError::TIMEOUT;
        }

        PIO_PinWrite(cs, false);

        auto error = readRegister<peripheralNumber, N>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }

    /**
     * Function to both write and read data on the same SPI transfer.
     *
     * @tparam peripheralNumber The chosen SPI peripheral.
     * @tparam N Size of transmit/receive data arrays in bytes.
     * @param cs The pin used for chip select.
     * @param TransmitData An array containing the bytes to be written.
     * @param ReceiveData An array to which the read data is copied.
     * @return A member of the SPIError enum; NONE it the transfer was succesful.
     */
    template<PeripheralNumber peripheralNumber, size_t N>
    SPIError SPIWriteReadRegister(const PIO_PIN cs, etl::array<uint8_t, N>& TransmitData, etl::array<uint8_t, N>& ReceiveData) {
        static_assert(N > 0, "Array size must be greater than zero");

        if (waitForResponse<peripheralNumber>(100) != SPIError::NONE) {
            return SPIError::BUSY;
        }

        PIO_PinWrite(cs, false);

        auto error = writeReadRegister<peripheralNumber, N>(TransmitData, ReceiveData);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        auto responseError = waitForResponse<peripheralNumber>(100);

        PIO_PinWrite(cs, true);

        return responseError;
    }
}