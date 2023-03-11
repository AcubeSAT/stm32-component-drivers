#pragma once

#include <cstdint>
#include <etl/string.h>
#include "definitions.h"
#include "peripheral/pio/plib_pio.h"
#include "peripheral/pwm/plib_pwm0.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @class A Latchup Current Limiter driver providing all the functionality for these protection circuits.
 * The LCLs are completely programmable during flight.
 * For now the class contains functionality only for the On-Board Computer Subsystem but
 * the Science Unit will make use of LCL circuits as well.
 * The main components that constitute a Latchup Current Limiter are a TLC555 timer, an
 * Operational Amplifier, a P-MOSFET, a N-MOSFET
 */
class LCL {
private:

    const PWM_CHANNEL_MASK pwmChannelMask;
    const PWM_CHANNEL_NUM pwmChannel;
    const PIO_PIN resetPin = PIO_PIN_NONE;
    const PIO_PIN setPin = PIO_PIN_NONE;
    uint16_t pwmDutyCycle = 0;
    uint16_t pwmFrequency = 0;
    bool lclEnabled = false;
    float currentThreshold = 0;
public:

    LCL(PWM_CHANNEL_MASK pwmChannelMask, PWM_CHANNEL_NUM pwmChannel, PIO_PIN resetPin, PIO_PIN setPin)
        : pwmChannelMask(pwmChannelMask), pwmChannel(pwmChannel), resetPin(resetPin), setPin(setPin) {
        PIO_PinWrite(resetPin, false);
        PIO_PinWrite(setPin, true);
    }

    inline void changePWMDutyCycle(uint16_t newDutyCycle) {
        pwmDutyCycle = newDutyCycle;
        PWM0_ChannelDutySet(pwmChannel, newDutyCycle);
    }

    void enableLCL();

    void disableLCL();

    void returnLCLStatus();

    void calculateVoltageThreshold();

    void calculateCurrentThreshold();

    void changeCurrentThreshHold();
};
