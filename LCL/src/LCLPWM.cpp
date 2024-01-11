#include "LCLPWM.hpp"
#include "Logger.hpp"

template
class LCLPWM<0>;

template
class LCLPWM<1>;

template<uint8_t PWMPeripheral>
LCLPWM<PWMPeripheral>::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin,
                              PIO_PIN setPin,
                              PWMThreshold dutyCyclePercent) : LCL(resetPin, setPin), pwmChannel(pwmChannel),
                                                               pwmChannelMask(pwmChannelMask),
                                                               voltageSetting(
                                                                       static_cast<std::underlying_type_t<PWMThreshold>>(dutyCyclePercent)) {
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
    if (dutyCyclePercent <= PWMDisableValue) {
        HAL_PWM::PWM_ChannelDutySet<PWMPeripheral>(pwmChannel, ConstantInPWMRegister * dutyCyclePercent / PWMDisableValue);
    } else {
        LOG_ERROR << "dutyCyclePercent is out of bounds (0-100)";
    }
}
