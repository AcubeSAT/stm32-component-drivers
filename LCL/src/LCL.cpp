#include "LCL.hpp"

LCL::LCL() {
//    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
//    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
}

void LCL::returnLCLStatus() {
    PWM0_ChannelPeriodGet();
    calculateCurrentThreshold();

    return struct
}

void LCL::openLCL() {
    // Todo assert status of GPIOs/log
    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
    PWM0_ChannelsStart(PWM_CHANNEL_0_MASK);

    PIO_PinWrite(PIO_PIN_PB3, false); // enable set
    SYSTICK_DelayMs(100);
    PIO_PinWrite(PIO_PIN_PB3, true); // disable set
}

void LCL::closeLCL() {
    // Todo assert status of GPIOs/log
    PWM0_ChannelsStop();
    PIO_PinWrite(PIO_PIN_PB2, false); // enable RESET
}

void LCL::calculateVoltageThreshold() {

}

void LCL::calculateCurrentThreshold() {

}

void LCL::changeCurrentThreshHold() {
    PWM0_ChannelDutySet(PWM_CHANNEL_0, duty0);

}
