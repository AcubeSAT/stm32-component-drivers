#pragma once

#include <cstdint>
#include "definitions.h"

namespace LCLDefinitions {

    /**
     *
     */
    struct LCLControlPins {
        uint8_t resetPin = static_cast<uint8_t>(0);
        uint8_t setPin = static_cast<uint8_t>(0);
        uint8_t pwmChannelMask = static_cast<uint8_t>(0);
        uint8_t pwmChannelNumber = static_cast<uint8_t>(0);
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

    LCLDefinitions::LCLControlPins& controlPins;

    uint16_t pwmDutyCycle = 0;

    // inline float currentThreshold = 0;
public:

    LCL(LCLDefinitions::LCLControlPins& controlPinsStruct) : controlPins(controlPinsStruct) {}

    void returnLCLStatus();

    void openLCL();

    void closeLCL();

    void calculateVoltageThreshold();

    void calculateCurrentThreshold();

    void changeCurrentThreshHold();
};
