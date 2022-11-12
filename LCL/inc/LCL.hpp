#pragma once

#include <cstdint>
#include <etl/string.h>
#include "definitions.h"
#include "peripheral/pio/plib_pio.h"
#include "peripheral/pwm/plib_pwm0.h"

namespace LCLDefinitions {

    /**
     *
     */
    struct LCLDeviceControl {
        etl::string<9> protectedDevice;
        PWM_CHANNEL_MASK pwmChannelMask;
        PWM_CHANNEL_NUM pwmChannelNumber;
        PIO_PIN resetPin;
        PIO_PIN setPin;
    };
}

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

    LCLDefinitions::LCLDeviceControl &controlPins;

    uint16_t pwmDutyCycle = 0;
    uint16_t pwmFrequency = 0;
    bool lclOn = false;
    // inline float currentThreshold = 0;
public:

    LCL(LCLDefinitions::LCLDeviceControl &controlPinsStruct) : controlPins(controlPinsStruct) {
        PIO_PinWrite(controlPins.resetPin, false);
        PIO_PinWrite(controlPins.setPin, true);
    }

    inline void changePWMDutyCycle(uint16_t newDutyCycle) {
        pwmDutyCycle = newDutyCycle;
        PWM0_ChannelDutySet(controlPins.pwmChannelNumber, newDutyCycle);
    }

    void returnLCLStatus();

    void enableLCL();

    void disableLCL();

    void calculateVoltageThreshold();

    void calculateCurrentThreshold();

    void changeCurrentThreshHold();
};
