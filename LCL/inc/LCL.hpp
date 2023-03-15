#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "peripheral/pwm/plib_pwm0.h"
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
private:
    /**
     * The Pulse Width Modulation (PWM) channel used for setting the CONT voltage of the LCL.
     */
    const PWM_CHANNEL_NUM pwmChannel;

    /**
     * The Pulse Width Modulation (PWM) channel mask used as a parameter for PWM peripheral functions.
     */
    const PWM_CHANNEL_MASK pwmChannelMask;

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

public:
    /**
     * Constructor to set the necessary control pins for the LCL.
     * @param pwmChannel @see pwmChannel
     * @param pwmChannelMask @see pwmChannelMask
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     */
    LCL(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin);

    /**
     * Enable to LCL to monitor and protect the IC from over current.
     */
    void enableLCL();

    /**
     * Disable the LCL, cutting the supply voltage to the IC.
     */
    void disableLCL();
};
