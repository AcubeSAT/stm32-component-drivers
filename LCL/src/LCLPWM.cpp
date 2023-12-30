#include "LCLPWM.hpp"

LCLPWM::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
               PWMThreshold dutyCycles) : LCL(resetPin, setPin), pwmChannel(pwmChannel),
                                          pwmChannelMask(pwmChannelMask),
                                          voltageSetting(static_cast<uint16_t>(dutyCycles)) {

    setCurrentThreshold(voltageSetting);
    disableLCL();
}

void LCLPWM::enableLCL() {
    PIO_PinWrite(resetPin, true);
    PWM0_ChannelsStart(pwmChannelMask);
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

void LCLPWM::disableLCL() {
    PWM0_ChannelsStop(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}

void LCLPWM::setCurrentThreshold(uint16_t dutyCyclePercent) {
    PWM0_ChannelDutySet(pwmChannel, ConstantInPWMRegister * dutyCyclePercent/100);
}
