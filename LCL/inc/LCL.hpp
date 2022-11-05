#pragma once

#ifndef ATSAM_COMPONENT_DRIVERS_LCL_HPP
#define ATSAM_COMPONENT_DRIVERS_LCL_HPP

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

    /**
     * PWM channels used to set the current threshold for the TLC555 timer.
     */
    enum PWMChannel : uint8_t {
        PWMC0_PWMH0 = 0, ///< NAND Flash
        PWM0_PWMH1 = 1, ///< MRAM
        PWMC0_PWMH3 = 2, ///< CAN1
        PWMC0_PWMH2 = 3 ///< CAN2
    };

    /**
     * GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum ResetPins : uint8_t {
        PC15 = 0, ///< NAND Flash
        PE4 = 1, ///< MRAM
        PD24 = 2, ///< CAN1
        PA26 = 3 ///< CAN2
    };

    /**
     * GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum SetPins : uint8_t {
        PC13 = 0, ///< MRAM
        PA17 = 1, ///< NAND Flash
        PD26 = 2, ///< CAN1
        PD22 = 3 ///< CAN2
    };

public:

    LCL();
    void returnLCLStatus();
    void openLCL();
    void closeLCL();
    void calculateVoltageThreshold();
    void calculateCurrentThreshold();
    void changeCurrentThreshHold();
};

#endif //ATSAM_COMPONENT_DRIVERS_LCL_HPP
