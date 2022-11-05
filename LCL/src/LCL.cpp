#include "LCL.hpp"

NANDFlashLCLProtection::NANDFlashLCLProtection() {
//    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
//    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
}

void NANDFlashLCLProtection::returnLCLStatus() {
    PWM0_ChannelPeriodGet();
    calculateCurrentThreshold();

    return struct
}

void NANDFlashLCLProtection::openLCL() {
    // Todo assert status of GPIOs/log
    PIO_PinWrite(PIO_PIN_PB2, true); // RESET
    PIO_PinWrite(PIO_PIN_PB3, true); // SET/TRIG
    PWM0_ChannelsStart(PWM_CHANNEL_0_MASK);

    PIO_PinWrite(PIO_PIN_PB3, false); // enable set
    SYSTICK_DelayMs(100);
    PIO_PinWrite(PIO_PIN_PB3, true); // disable set
}

void NANDFlashLCLProtection::closeLCL() {
    // Todo assert status of GPIOs/log
    PWM0_ChannelsStop();
    PIO_PinWrite(PIO_PIN_PB2, false); // enable RESET
}

void NANDFlashLCLProtection::calculateVoltageThreshold() {

}

void NANDFlashLCLProtection::calculateCurrentThreshold() {

}

void NANDFlashLCLProtection::changeCurrentThreshHold() {
    PWM0_ChannelDutySet(PWM_CHANNEL_0, duty0);

}
