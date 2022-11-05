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
     * @enum Devices protected by LCLs
     */
    enum class ProtectedDevices : uint8_t {
        NANDFlash = 0,
        MRAM = 1,
        CAN1 = 2,
        CAN2 = 3
    };

    /**
     * @enum PWM channels used to set the current threshold for the TLC555 timer.
     */
    enum class PWMChannel : uint8_t {
        PWMC0_PWMH0 = 0, ///< NAND Flash
        PWM0_PWMH1 = 1, ///< MRAM
        PWMC0_PWMH3 = 2, ///< CAN1
        PWMC0_PWMH2 = 3 ///< CAN2
    };

    /**
     * @enum GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum class ResetPins : uint8_t {
        MRAM = PIO_PIN_PC15, ///< NAND Flash
        NANDFlash = PIO_PIN_PE4, ///< MRAM
        CAN1 = PIO_PIN_PD24, ///< CAN1
        CAN2 = PIO_PIN_PA26 ///< CAN2
    };

    /**
     * @enum GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum class SetPins : uint8_t {
        MRAM = PIO_PIN_PC13, ///< MRAM
        NANDFlash = PIO_PIN_PA17, ///< NAND Flash
        CAN1 = PIO_PIN_PD26, ///< CAN1
        CAN2 = PIO_PIN_PD22 ///< CAN2
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
