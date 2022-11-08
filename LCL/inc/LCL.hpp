#pragma once

#include "LCLDefinitions.hpp"

namespace LCLDefinitions {

    /**
     *
     */
    struct LCLControlPins;
}

/**
 * @class A Latchup Current Limiter driver providing all the functionality for these protection circuits.
 * The LCLs are completely programmable during flight.
 * For now the class contains functionality only for the On-Board Computer Subsystem but
 * the Science Unit will make use of LCL circuits as well.
 * The main components that constitute a Latchup Current Limiter are a TLC555 timer, an
 * Operational Amplifier, a P-MOS MOSFET, a N-MOS MOSFET
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
