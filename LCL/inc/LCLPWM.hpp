#pragma once

#include "LCL.hpp"
#include "HAL_PWM.hpp"

template<uint8_t n>
class LCLPWM : public LCL {
public:
    /**
     * A variable to store the voltage setting (CAN or NAND)
     */
    uint16_t voltageSetting;

    /**
     * This variable will store the value to set dutyCycles to 0%
     * In our current configuration we have:
     *      - CPOL bit is set to '0', which sets the polarity to LOW
     *      - CES bit is set to '0', which sets the interrupt to  occur at the end of the counter period.
     *      - Center Aligned Waveform , so the dutyCycle is calculated using half the Period
     *      - Period = 15000
     * So, with this configuration when we are setting the duty cycle we are actually setting the counter period
     * which in our case is the delay of the High signal and not the duration of it.
     * In this case a 7500 delay in a 7500 half period will set the High signal duration to 0.
     */
    uint16_t ConstantInPWMRegister = 7500;

    /**
    * The value for duty cycles % to write in the pwmChannel
    * In the current configuration the duty cycles % acts as a delay of the High signal
    * The final value is calculated using the following formula: V = Vmax * High_Signal_Duration%
    * where:
    *       - Vmax=3.3V
    *       - High_Signal_Duration% = 1 - duty cycles %
    *       - 16 bits resolution
    *
    * Current duty Cycle %  for the CAMERA = 50%
    */
    enum PWMThreshold : uint16_t {
        CAMERA = 50,
        PWMDisableValue = 100
    };

    /**
    * Constructor to set the necessary control pins for the LCL.
    * @param pwmChannel @see pwmChannel
    * @param pwmChannelMask @see pwmChannelMask
    * @param resetPin @see resetPin
    * @param setPin @see setPin
    * @param peripheralNumber @see peripheralNumber
    * @param dutyCycles duty cycle threshold
    */
    LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
           PWMThreshold dutyCycles = PWMDisableValue);

    /**
     * Enable the LCL to monitor and protect the protected IC from over current.
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
    void enableLCL() override;

    /**
     * Disable the LCL, cutting the supply voltage to the IC. To achieve this, the Reset Pin is driven Low to force the
     * SR Latch state to Low, cutting the power towards the IC and as an extra step, the PWM signal is closed, setting
     * the current threshold to a small value, typically much smaller than the consumption of the protected IC.
     * @note Configure the default state of the PWM signal to be Low (instead of High) when the channel is turned off.
     */
    void disableLCL() override;

    /**
     * Sets the duty cycle% of the PWM signal
     * @param dutyCyclePercent PWMThreshold
     */
    void setCurrentThreshold(uint16_t dutyCyclePercent);

private:
    /**
    * The Pulse Width Modulation (PWM) channel used for setting the CONT voltage of the LCL.
    */
    const PWM_CHANNEL_NUM pwmChannel;

    /**
     * The Pulse Width Modulation (PWM) channel mask used as a parameter for PWM peripheral functions.
     */
    const PWM_CHANNEL_MASK pwmChannelMask;

    /**
     * Stores the PWM peripheral (0 or 1)
     */
    static constexpr uint8_t peripheralNumber = n;
};
