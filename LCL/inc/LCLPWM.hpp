#pragma once

#include "LCL.hpp"
#include "HAL_PWM.hpp"

template<uint8_t PWMPeripheral>
class LCLPWM : public LCL {
public:
    /**
     * A variable to store the voltage setting.
     */
    uint16_t voltageSetting = PWMDisableValue;

    /**
     * This variable will store the value to set dutyCycles to 0%.
     * In our current configuration, we have:
     * - CPOL bit is set to '0', which sets the polarity to LOW.
     * - Left Aligned Waveform
     * - Period = 15000
     * So, with this configuration when we are setting the duty cycle we are actually setting the counter period,
     * which in our case is the delay of the High signal and not the duration of it.
     * In this case a 15000 delay in a 15000 period will set the High signal duration to 0.
     */
    static constexpr uint16_t ConstantInPWMRegister = 15000U;

    /**
     * The value for duty cycles % to write in the pwmChannel.
     * In the current configuration, the duty cycles % acts as a delay of the High signal.
     * The final value is calculated using the following formula: V = Vmax * High_Signal_Duration%,
     * where:
     * - Vmax = 3.3V
     * - High_Signal_Duration% = 1 - duty Cycle % (ex. 1 - 0.5 for CAMERA)
     * - 16 bits resolution
     *
     * Current duty Cycle % for the CAMERA = 50%
     */
    enum PWMThreshold : uint16_t {
        CAMERA = 50,
        PWMDisableValue = 100
    };

    /**
     * Constructor to set the necessary control pins for the LCL.
     * @tparam PWMPeripheral @see peripheralNumber
     * @param pwmChannel @see pwmChannel
     * @param pwmChannelMask @see pwmChannelMask
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     * @param dutyCycles duty cycle threshold
     */
    LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
           PWMThreshold dutyCycles = PWMDisableValue);

    /**
     * Enable the LCL to monitor and protect the protected IC from overcurrent.
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
     * Sets the duty cycle% of the PWM signal.
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
};
