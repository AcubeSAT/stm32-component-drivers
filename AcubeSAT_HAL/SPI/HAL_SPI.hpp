#pragma once

#include <cstdint>
#include <plib_pio.h>
#include <plib_systick.h>
#include <etl/span.h>
#include "Logger.hpp"

#ifdef  SPI0_ENABLED
#include  ""peripheral/spi/spi_master/plib_spi0_master.h""
#endif
#ifdef  SPI1_ENABLED
#include  ""peripheral/spi/spi_master/plib_spi1_master.h""
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

    using SPI_CALLBACK = void (*)(uintptr_t context);

    struct SPI_CALLBACK_CONTEXT {
        PIO_PIN cs;
        SPI_CALLBACK userCallback;
        uintptr_t userContext;
    };

    static SPI_CALLBACK_CONTEXT spi0CallbackContext;
    static SPI_CALLBACK_CONTEXT spi1CallbackContext;

    void SPI0_TransferHandler(uintptr_t context) {
        PIO_PinWrite(spi0CallbackContext.cs, true);

        if (spi0CallbackContext.userCallback != nullptr) {
            spi0CallbackContext.userCallback(spi0CallbackContext.userContext);
        }
    }

    void SPI1_TransferHandler(uintptr_t context) {
        PIO_PinWrite(spi1CallbackContext.cs, true);

        if (spi1CallbackContext.userCallback != nullptr) {
            spi1CallbackContext.userCallback(spi1CallbackContext.userContext);
        }
    }

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

    template <PeripheralNumber peripheralNumber>
    static void registerCallback(SPI_CALLBACK userCallback, uintptr_t userContext) {
        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
#ifdef SPI0_ENABLED
            spi0CallbackContext.userCallback = userCallback;
            spi0CallbackContext.userContext = userContext;
            SPI0_CallbackRegister(SPI0_TransferHandler, (uintptr_t)&spi0CallbackContext);
#endif
    }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
#ifdef SPI1_ENABLED
            spi1CallbackContext.userCallback = userCallback;
            spi1CallbackContext.userContext = userContext;
            SPI1_CallbackRegister(SPI1_TransferHandler, (uintptr_t)&spi1CallbackContext);
#endif
    }
}

    template<PeripheralNumber peripheralNumber>
    SPIError waitForResponse(const uint16_t ms) {
        const auto start = SYSTICK_TimerCounterGet();
        while (isBusy<peripheralNumber>()) {
            if (SYSTICK_TimerCounterGet() - start > ms) {
                LOG_ERROR << "SPI" << peripheralNumber << " timed out";
                initialize<peripheralNumber>();
                return SPIError::TIMEOUT;
            }
        }
        return SPIError::NONE;
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIwriteRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& data, SPI_CALLBACK callback, uintptr_t context) {
        if (isBusy<peripheralNumber>()) {
            return SPIError::BUSY;
        }

        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
            spi0CallbackContext.cs = cs;
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
            spi1CallbackContext.cs = cs;
        }

        registerCallback<peripheralNumber>(callback, context);

        PIO_PinWrite(cs, false);

        if (!writeRegister<peripheralNumber>(data)) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        return SPIError::NONE;
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIReadRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& data, SPI_CALLBACK callback, uintptr_t context) {
        if (isBusy<peripheralNumber>()) {
            return SPIError::BUSY;
        }

        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
            spi0CallbackContext.cs = cs;
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
            spi1CallbackContext.cs = cs;
        }

        registerCallback<peripheralNumber>(callback, context);

        PIO_PinWrite(cs, false);

        if (!readRegister<peripheralNumber>(data)) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        return SPIError::NONE;
    }

    template<PeripheralNumber peripheralNumber>
    SPIError SPIWriteReadRegister(const PIO_PIN cs, etl::array<uint8_t, 4>& TransmitData, etl::array<uint8_t, 4>& ReceiveData, SPI_CALLBACK callback, uintptr_t context) {
        if (isBusy<peripheralNumber>()) {
            return SPIError::BUSY;
        }

        if constexpr (peripheralNumber == PeripheralNumber::SPI0) {
            spi0CallbackContext.cs = cs;
        }
        if constexpr (peripheralNumber == PeripheralNumber::SPI1) {
            spi1CallbackContext.cs = cs;
        }

        registerCallback<peripheralNumber>(callback, context);

        PIO_PinWrite(cs, false);

        if (!writeReadRegister<peripheralNumber>(TransmitData, ReceiveData)) {
            PIO_PinWrite(cs, true);
            return SPIError::WRITE_READ_ERROR;
        }

        return SPIError::NONE;
    }
}