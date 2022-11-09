#include "LCL.hpp"

//void LCL::returnLCLStatus() {
//    PWM0_ChannelPeriodGet();
//    calculateCurrentThreshold();
//
//    return struct
//}

void LCL::openLCL() {
    // Todo assert status of GPIOs/log
    PIO_PinWrite(controlPins.resetPin, true); // RESET
    PWM0_ChannelsStart(controlPins.pwmChannelMask);

    PIO_PinWrite(controlPins.setPin, false); // enable set
    SYSTICK_DelayMs(100); // any better ideas?
    PIO_PinWrite(controlPins.setPin, true); // disable set
    lclOn = true;
}


void LCL::closeLCL() {
    // Todo assert status of GPIOs/log
    PWM0_ChannelsStop(controlPins.pwmChannelMask);
    PIO_PinWrite(PIO_PIN_PB2, false); // enable RESET
    lclOn = false;
}

void LCL::calculateVoltageThreshold() {

}

void LCL::calculateCurrentThreshold() {

}

void LCL::changeCurrentThreshHold(uint16_t newDutyCycle) {
    PWM0_ChannelDutySet(controlPins.pwmChannelNumber, newDutyCycle);
}
