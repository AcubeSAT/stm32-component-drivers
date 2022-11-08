#pragma once

#ifndef ATSAM_COMPONENT_DRIVERS_LCL_HPP
#define ATSAM_COMPONENT_DRIVERS_LCL_HPP

namespace LCLDefinitions {

    /**
     *
     */
    struct LCLControlPins {
        ResetPin resetPin = 0;
        SetPins setPin = 0;
        PWMChannelMasks pwmChannelMask = 0;
        PWMChannelNumbers pwmChannelNumber = 0;
    };

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
     * PWM0 of ATSAMV71 is used for all thresholds.
     */
    enum class PWMChannelMasks : uint8_t {
        NANDFlash = PWM_CHANNEL_0_MASK, ///< NAND Flash
        MRAM = PWM_CHANNEL_1_MASK, ///< MRAM
        CAN1 = PWM_CHANNEL_3_MASK, ///< CAN1
        CAN2 = PWM_CHANNEL_2_MASK ///< CAN2
    };

    /**
     * @enum PWM channels used to set the current threshold for the TLC555 timer.
     */
    enum class PWMChannelNumbers : uint8_t {
        NANDFlash = PWM_CHANNEL_0, ///< NAND Flash
        MRAM = PWM_CHANNEL_1, ///< MRAM
        CAN1 = PWM_CHANNEL_3, ///< CAN1
        CAN2 = PWM_CHANNEL_2 ///< CAN2
    };

    /**
     * @enum GPIOs used reset the LCL, specifically the TLC555 IC.
     * Active low.
     * Initial value given by a pull-up resistor to prevent undefined behavior during MCU
     * reset/start-up.
     */
    enum class ResetPins : uint8_t {
        NANDFlash = PIO_PIN_PC15, ///< NAND Flash
        MRAM = PIO_PIN_PE4, ///< MRAM
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
        NANDFlash = PIO_PIN_PC13, ///< MRAM
        MRAM = PIO_PIN_PA17, ///< NAND Flash
        CAN1 = PIO_PIN_PD26, ///< CAN1
        CAN2 = PIO_PIN_PD22 ///< CAN2
    };
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

    LCLDefinitions::LCLControlPins controlPins = {0};

    inline uint16_t pwmDutyCycle = 0;

    // inline float currentThreshold = 0;
public:

    void returnLCLStatus(LCLDefinitions::LCLControlPins) : ;

    void openLCL();

    void closeLCL();

    void calculateVoltageThreshold();

    void calculateCurrentThreshold();

    void changeCurrentThreshHold();
};

#endif //ATSAM_COMPONENT_DRIVERS_LCL_HPP
