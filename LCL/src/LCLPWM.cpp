#include "LCLPWM.hpp"
#include "HAL_PWM.hpp"

template<uint8_t n>
LCLPWM<n>::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
                  uint8_t peripheralNumber, PWMThreshold dutyCycles) : LCL(resetPin, setPin), pwmChannel(pwmChannel),
                                                                       pwmChannelMask(pwmChannelMask),
                                                                       peripheralNumber(peripheralNumber),
                                                                       voltageSetting(
                                                                               static_cast<uint16_t>(dutyCycles)) {
    setCurrentThreshold(voltageSetting);
    disableLCL();
}

template<uint8_t n>
void LCLPWM<n>::enableLCL() {
    PIO_PinWrite(resetPin, true);
    HAL_PWM::PWM_ChannelsStart<peripheralNumber>(pwmChannelMask);
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

template<uint8_t n>
void LCLPWM<n>::disableLCL() {
    HAL_PWM::PWM_ChannelsStop<peripheralNumber>(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}

template<uint8_t n>
void LCLPWM<n>::setCurrentThreshold(uint16_t dutyCyclePercent) {
    HAL_PWM::PWM_ChannelDutySet<peripheralNumber>(pwmChannel, ConstantInPWMRegister * dutyCyclePercent / 100);
}
