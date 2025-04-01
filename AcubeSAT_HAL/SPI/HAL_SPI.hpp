#pragma once

#include <cstdint>
#include <plib_pio.h>
#include <plib_systick.h>
#include <etl/span.h>
#include "Logger.hpp"

#ifdef  SPI0_ENABLED
#include  "peripheral/spi/spi_master/plib_spi0_master.h"
#endif
#ifdef  SPI1_ENABLED
#include  "peripheral/spi/spi_master/plib_spi1_master.h"
#endif

namespace HAL_SPI {
    enum class SPIError : uint8_t {
        NONE             = 0,
        INVALID_ARGUMENT = 1,
        TIMEOUT          = 2,
        WRITE_READ_ERROR = 3,
        BUSY             = 4,
        UNDEFINED        = 255
    };

    enum class PeripheralNumber : uint8_t {
        SPI0,
        SPI1,
    };

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
    }

    template <PeripheralNumber peripheralNumber>
    static void initialize() {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            SPI0_Initialize();
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            SPI1_Initialize();
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    bool writeRegister(etl::array<uint8_t, 4>& data) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_Write(data.data(), 4);
#else
            return false;
#endif
    }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_Write(data.data(), 4);
#else
            return false;
#endif
    }
}

    template<PeripheralNumber peripheralNumber>
    bool readRegister(etl::array<uint8_t, 4>& data) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_Read(data.data(), 4);
#else
            return false;
#endif
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_Read(data.data(), 4);
#else
            return false;
#endif
        }
    }

    template<PeripheralNumber peripheralNumber>
    bool writeReadRegister(etl::array<uint8_t, 4>& TransmitData, etl::array<uint8_t, 4>& ReceiveData) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            return SPI0_WriteRead(TransmitData.data(), 4, ReceiveData.data(), 4);
#else
            return false;
#endif
    }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            return SPI1_WriteRead(TransmitData.data(), 4, ReceiveData.data(), 4);
#else
            return false;
#endif
    }
}

    template<PeripheralNumber peripheralNumber>
    SPIError waitForResponse(const uint32_t timeoutMs) {
        SYSTICK_TimerStart();
        const auto start = SYSTICK_TimerCounterGet()/300000;

        while (isBusy<peripheralNumber>()) {
            if (SYSTICK_TimerCounterGet()/300000 - start > timeoutMs) {
                initialize<peripheralNumber>();
                return SPIError::TIMEOUT;
            }
        }
        return SPIError::NONE;
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIwriteRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& data) {
        PIO_PinWrite(cs, false);

        auto error = writeRegister<peripheralNumber>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        PIO_PinWrite(cs, true);

        return waitForResponse<peripheralNumber>(100);
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIReadRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& data) {
        PIO_PinWrite(cs, false);

        auto error = readRegister<peripheralNumber>(data);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        PIO_PinWrite(cs, true);

        return waitForResponse<peripheralNumber>(100);
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIWriteReadRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& TransmitData, etl::array<uint8_t, 4>& ReceiveData) {
        PIO_PinWrite(cs, false);

        auto error = writeReadRegister<peripheralNumber>(TransmitData, ReceiveData);

        if (not error) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        PIO_PinWrite(cs, true);

        return waitForResponse<peripheralNumber>(100);
    }
}