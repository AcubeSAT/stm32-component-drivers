#pragma once

#include <cstdint>
#include "Peripheral_Definitions.hpp"
#include "peripheral/afec/plib_afec0.h"
#include "peripheral/afec/plib_afec1.h"

/**
 * @brief HAL_AFEC namespace for controlling AFEC peripherals.
 *
 * This namespace provides a high-level interface for controlling AFEC peripherals.
 * It includes the functions: AFEC_ConversionStart, AFEC_Initialize, AFEC_CallbackRegister.
 */
namespace HAL_AFEC {
    /**
     * @brief Start AFEC conversion for a specific peripheral.
     *
     * This function starts AFEC conversion for the specified peripheral.
     */
    template<AFECPeripheral AfecPeripheral>
    void AFEC_ConversionStart();

    /**
     * @brief Initializes a specific AFEC peripheral.
     *
     * This function initializes a specific AFEC peripheral.
     */
    template<AFECPeripheral AfecPeripheral>
    void AFEC_Initialize();

    /**
     * @brief Registers the function to be called from interrupt for a specific peripheral.
     *
     * This function registers the function to be called from interrupt for a specific peripheral.
     */
    template<AFECPeripheral AfecPeripheral>
    void AFEC_CallbackRegister(AFEC_CALLBACK callback, uintptr_t context);
}
