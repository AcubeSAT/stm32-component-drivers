#include "LCLPWM.hpp"

LCLPWM::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
               uint16_t dutyCycle)
        : LCL(resetPin, setPin), pwmChannel(pwmChannel), pwmChannelMask(pwmChannelMask) {
    if (dutyCycle != 0) {
        setCurrentThreshold(dutyCycle);
    }
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

void LCLPWM::setCurrentThreshold(uint16_t dutyCycle) {
    PWM0_ChannelDutySet(pwmChannel, dutyCycle);
}

