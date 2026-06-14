#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "etl/expected.h"

enum class LCLError : uint8_t {
    TIMEOUT,
    OUT_OF_BOUNDS
};

/**
 * @class A Latch-up Current Limiter (LCL) driver providing all the functionality for these protection circuits.
 * The LCLs are completely reprogrammable during flight.
 * The main components that constitute a Latch up Current Limiter are a TLC555 timer, an
 * Operational Amplifier, a P-MOSFET, a N-MOSFET.
 * The programmable logic requires a Pulse Width Modulation (PWM) signal or Digital to Analog Converter (DAC), a GPIO for Set and a GPIO for Reset Logic.
 */
class LCL {
protected:
    /**
     * The Reset Pin force disables the LCL when driven Low, overriding the Set Pin.
     * Drive High to start the sequence that enables the LCL.
     */
    const PIO_PIN resetPin = PIO_PIN_NONE;

    /**
     * The Set Pin force enables the LCL when driven Low. If the Set and Reset pins are High and the CONT voltage is
     * higher than the THRES voltage, the LCL maintains its status.
     */
    const PIO_PIN setPin = PIO_PIN_NONE;

    /**
     * Variable to store the software state of the LCL.
     * This keeps track of the intended state set by the MCU,
     * it will not automatically reflect if a hardware latch-up has been triggered.
     */
    bool lclStatus = false;

    /**
     * Constructor to set the necessary control pins for the LCL.
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     */
    LCL(PIO_PIN resetPin, PIO_PIN setPin) : resetPin(resetPin), setPin(setPin) {}

public:
    /**
     * Enable the LCL
     * @return LCLError on failure
     */
    [[nodiscard]] virtual etl::expected<void, LCLError> enableLCL() = 0;

    /**
     * Disable the LCL
     * @return LCLError on failure
     */
    [[nodiscard]] virtual etl::expected<void, LCLError> disableLCL() = 0;

    /**
     * Gets the expected software status of the LCL.
     * @note This does not reflect the actual hardware status of the LCL, which may be disabled due to a latch-up event.
     * @return true if the LCL has been enabled via software, false otherwise.
     */
    [[nodiscard]] bool returnLCLstatus() const {
        return lclStatus;
    }
};
