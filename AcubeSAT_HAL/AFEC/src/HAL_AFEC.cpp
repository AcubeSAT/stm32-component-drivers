#include "HAL_AFEC.hpp"

namespace HAL_AFEC {
    template<AFECPeripheral AfecPeripheral>
    void AFEC_ConversionStart() {
        static_assert(AfecPeripheral == AFECPeripheral::AFEC0 || AfecPeripheral == AFECPeripheral::AFEC1, "Template parameter must be AFECPeripheral::AFEC0 or AFECPeripheral::AFEC1");
    }

    template<>
    void AFEC_ConversionStart<AFECPeripheral::AFEC0>() {
        AFEC0_ConversionStart();
    }

    template<>
    void AFEC_ConversionStart<AFECPeripheral::AFEC1>() {
        AFEC1_ConversionStart();
    }

    template<AFECPeripheral AfecPeripheral>
    void AFEC_Initialize() {
        static_assert(AfecPeripheral == AFECPeripheral::AFEC0 || AfecPeripheral == AFECPeripheral::AFEC1, "Template parameter must be AFECPeripheral::AFEC0 or AFECPeripheral::AFEC1");
    }

    template<>
    void AFEC_Initialize<AFECPeripheral::AFEC0>() {
        AFEC0_Initialize();
    }

    template<>
    void AFEC_Initialize<AFECPeripheral::AFEC1>() {
        AFEC1_Initialize();
    }

    template<AFECPeripheral AfecPeripheral>
    void AFEC_CallbackRegister(AFEC_CALLBACK callback, uintptr_t context) {
        static_assert(AfecPeripheral == AFECPeripheral::AFEC0 || AfecPeripheral == AFECPeripheral::AFEC1, "Template parameter must be 0 or 1");
    }

    template<>
    void AFEC_CallbackRegister<AFECPeripheral::AFEC0>(AFEC_CALLBACK callback, uintptr_t context) {
        AFEC0_CallbackRegister(callback, reinterpret_cast<uintptr_t>(nullptr));
    }

    template<>
    void AFEC_CallbackRegister<AFECPeripheral::AFEC1>(AFEC_CALLBACK callback, uintptr_t context) {
        AFEC1_CallbackRegister(callback, reinterpret_cast<uintptr_t>(nullptr));
    }
}