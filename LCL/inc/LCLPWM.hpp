#pragma once

#include "LCL.hpp"
#include "peripheral/pwm/plib_pwm0.h"

class LCLPWM : public LCL {
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
    /**
     * A variable to store the voltage setting (CAN or NAND)
     */
    uint16_t voltageSetting;

    /**
    * The value for duty cycles to write in the pwmChannel
    * The final value is calculated using the following formula: V = Vmax * Duty_Cycles
    * where: Vmax=3.3V
    * 16 bits resolution
    * Curent duty Cycle % = 50%
    */
    enum PWMThreshold : uint16_t {
        CAMERA = 0x8000,
        PWMDisableValue = 0

    };

    /**
    * Constructor to set the necessary control pins for the LCL.
    * @param pwmChannel @see pwmChannel
    * @param pwmChannelMask @see pwmChannelMask
    * @param resetPin @see resetPin
    * @param setPin @see setPin
    * @param dutyCycles duty cycle threshold
    */
    LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
           PWMThreshold dutyCycles = PWMDisableValue);

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

    /**
     * Sets the duty cycle of the PWM signal
     * @param dutyCycle duty cycle threshold
     * */
    void setCurrentThreshold(uint16_t dutyCycle);
};
