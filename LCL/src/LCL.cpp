#include "LCL.hpp"

void LCL::openLCL() {
    PWM0_ChannelsStart(PWM_CHANNEL_0_MASK);
    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
    SYSTICK_DelayMs(500);
    PIO_PinWrite(PIO_PIN_PB3, false);
    SYSTICK_DelayMs(100);
    PIO_PinWrite(PIO_PIN_PB3, true);
}