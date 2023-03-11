#include "LCL.hpp"

void LCL::enableLCL() {
    // Todo assert status of GPIOs/log
    PIO_PinWrite(resetPin, true); // RESET
    PWM0_ChannelsStart(pwmChannelMask);

    PIO_PinWrite(setPin, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    PIO_PinWrite(setPin, true);
    lclEnabled = true;
}


void LCL::disableLCL() {
    // Todo assert status of GPIOs/log
    PWM0_ChannelsStop(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    lclEnabled = false;
}
