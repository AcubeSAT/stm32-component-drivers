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

public:
    /**
     * Constructor to set the necessary control pins for the LCL.
     * @param pwmChannel @see pwmChannel
     * @param pwmChannelMask @see pwmChannelMask
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     */
    LCL(PIO_PIN resetPin, PIO_PIN setPin);
};

class LCLPWM : public LCL{
private:
    /**
    * The Pulse Width Modulation (PWM) channel used for setting the CONT voltage of the LCL.
    */
    const PWM_CHANNEL_NUM pwmChannel;

    /**
     * The Pulse Width Modulation (PWM) channel mask used as a parameter for PWM peripheral functions.
     */
    const PWM_CHANNEL_MASK pwmChannelMask;
public:

    LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin);
    /**
     * Enable to LCL to monitor and protect the protected IC from over current.
     * Sequence to enable the LCL is as follows:
     * - Enable the PWM signal with predefined frequency and duty cycle to form the desired current threshold.
     * - Pull the Reset Pin High to allow SR Latch state of the TLC555 to be driven but the internal comparators
     * instead of being forced Low.
     * - Pull the Set Pin Low to force the SR Latch state of the TLC555 to logic High.
     * - After a brief delay that is necessary for the TLC555 to detect the pulse, pull the Set Pin High making the SR
     * Latch retain its state. If the it was High, the IC is powered on, if a current surge is detected, the SR Latch
     * will be driven Low, cutting the power supply towards the protected IC and retain its state after the surge.
     * @note If a surge is detected and the power is cut, the MCU will have to call the @fn enableLCL to provide the IC
     * with power again.
     */
    void enableLCL();

    /**
     * Disable the LCL, cutting the supply voltage to the IC. To achieve this, the Reset Pin is driven Low to force the
     * SR Latch state to Low, cutting the power towards the IC and as an extra step, the PWM signal is closed, setting
     * the current threshold to a small value, typically much smaller than the consumption of the protected IC.
     * @note Configure the default state of the PWM signal to be Low (instead of High) when the channel is turned off.
     */
    void disableLCL();
};

class LCLDACC:public LCL{
private:
    const DACC_CHANNEL_NUM dacChannel = DACC_CHANNEL_0;

public:
    LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin);
    void enableLCL();
    void disableLCL();
};
