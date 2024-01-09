#include <type_traits>
#include "LCLPWM.hpp"

template
class LCLPWM<0>;

template
class LCLPWM<1>;

template<uint8_t PWMPeripheral>
LCLPWM<PWMPeripheral>::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin,
                              PIO_PIN setPin,
                              PWMThreshold dutyCycles) : LCL(resetPin, setPin), pwmChannel(pwmChannel),
                                                         pwmChannelMask(pwmChannelMask),
                                                         voltageSetting(static_cast<std::underlying_type<PWMThreshold>>(dutyCycles)) {
    setCurrentThreshold(voltageSetting);
    disableLCL();
}

template<uint8_t PWMPeripheral>
void LCLPWM<PWMPeripheral>::enableLCL() {
    PIO_PinWrite(resetPin, true);
    HAL_PWM::PWM_ChannelsStart<PWMPeripheral>(pwmChannelMask);
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

template<uint8_t PWMPeripheral>
void LCLPWM<PWMPeripheral>::disableLCL() {
    HAL_PWM::PWM_ChannelsStop<PWMPeripheral>(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}

template<uint8_t PWMPeripheral>
void LCLPWM<PWMPeripheral>::setCurrentThreshold(uint16_t dutyCyclePercent) {
    HAL_PWM::PWM_ChannelDutySet<PWMPeripheral>(pwmChannel, ConstantInPWMRegister * dutyCyclePercent / 100);
}
