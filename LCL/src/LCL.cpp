#include "LCL.hpp"

LCL::LCL(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin)
        : pwmChannel(pwmChannel), pwmChannelMask(pwmChannelMask), resetPin(resetPin), setPin(setPin) {
    disableLCL();
}

void LCL::enableLCL() {
    PIO_PinWrite(resetPin, true);
    PWM0_ChannelsStart(pwmChannelMask);
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}


void LCL::disableLCL() {
    PWM0_ChannelsStop(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}
