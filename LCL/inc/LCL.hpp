#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "peripheral/pwm/plib_pwm0.h"
#include "peripheral/dacc/plib_dacc.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @class A Latch-up Current Limiter (LCL) driver providing all the functionality for these protection circuits.
 * The LCLs are completely reprogrammable during flight.
 * The main components that constitute a Latch up Current Limiter are a TLC555 timer, an
 * Operational Amplifier, a P-MOSFET, a N-MOSFET.
 * The programmable logic requires a Pulse Width Modulation (PWM) signal, a GPIO for Set and a GPIO for Reset Logic.
 */
class LCL {

public:
    /**
     * Constructor to set the necessary control pins for the LCL.
     * @param pwmChannel @see pwmChannel
     * @param pwmChannelMask @see pwmChannelMask
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     */
    LCL() = default;

    virtual void enableLCL() = 0;

    virtual void disableLCL() = 0;
};
