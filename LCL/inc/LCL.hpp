#pragma once

#include <cstdint>
#include "definitions.h"
#include "peripheral/pio/plib_pio.h"

namespace LCLDefinitions {

    /**
     * @enum Devices protected by LCLs, used for logging information
     */
    enum class ProtectedDevices : uint8_t;

    /**
     * @enum PWM channels used to set the current threshold for the TLC555 timer.
     * PWM0 of ATSAMV71 is used for all thresholds.
     */
    enum class PWMChannelMasks : PWM_CHANNEL_MASK;

    /**
     * @enum PWM channels used to set the current threshold for the TLC555 timer.
     */
    enum class PWMChannelNumbers : PWM_CHANNEL_NUM;

    /**
     * @enum GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum class ResetPins : PIO_PIN;

    /**
     * @enum GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum class SetPins : PIO_PIN;

    /**
     *
     */
    struct LCLControlPins {
        ResetPins resetPin = static_cast<ResetPins>(0);
        SetPins setPin = static_cast<SetPins>(0);
        PWMChannelMasks pwmChannelMask = static_cast<PWMChannelMasks>(0);
        PWMChannelNumbers pwmChannelNumber = static_cast<PWMChannelNumbers>(0);
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

    LCLDefinitions::LCLControlPins &controlPins;

    LCLDefinitions::ProtectedDevices protectedDevice = static_cast<LCLDefinitions::ProtectedDevices>(255);
    uint16_t pwmDutyCycle = 0;
    uint16_t pwmFrequency = 0;
    bool lclOn = false;
    // inline float currentThreshold = 0;
public:

    LCL(LCLDefinitions::LCLControlPins &controlPinsStruct) : controlPins(controlPinsStruct) {
        PIO_PinWrite(controlPins.resetPin, false);
        PIO_PinWrite(controlPins.setPin, true);
    }

    inline void changePWMDutyCycle(uint16_t newDutyCycle) {
        pwmDutyCycle = newDutyCycle;
        PWM0_ChannelDutySet(controlPins.pwmChannelNumber, newDutyCycle);
    }

    void returnLCLStatus();

    void openLCL();

    void closeLCL();

    void calculateVoltageThreshold();

    void calculateCurrentThreshold();

    void changeCurrentThreshHold();
};
