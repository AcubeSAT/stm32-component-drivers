#include "LCL.hpp"

LCL::LCL() {

}

void LCL::openLCL() {
    PWM0_ChannelsStart(PWM_CHANNEL_0_MASK); PWM0_ChannelsStop() PWM0_ChannelPeriodGet
    (PWM_CHANNEL_0) PWM0_ChannelDutySet(PWM_CHANNEL_0, duty0);
    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
    SYSTICK_DelayMs(500);
    PIO_PinWrite(PIO_PIN_PB3, false);
    SYSTICK_DelayMs(100);
    PIO_PinWrite(PIO_PIN_PB3, true);
}