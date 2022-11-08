#include "LCL.hpp"

//void LCL::returnLCLStatus() {
//    PWM0_ChannelPeriodGet();
//    calculateCurrentThreshold();
//
//    return struct
//}

//void LCL::openLCL() {
//    // Todo assert status of GPIOs/log
//    PIO_PinWrite(controlPins::resetPin, true); // RESET
//    PIO_PinWrite(controlPins::setPin, true); // SET/TRIG
////    PWM0_ChannelsStart(PWM_CHANNEL_0_MASK);
//
//    PIO_PinWrite(controlPins::setPin, false); // enable set
//    SYSTICK_DelayMs(100);
//    PIO_PinWrite(controlPins::setPin, true); // disable set
//}
//
//void LCL::closeLCL() {
//    // Todo assert status of GPIOs/log
//    PWM0_ChannelsStop();
//    PIO_PinWrite(PIO_PIN_PB2, false); // enable RESET
//}
//
//void LCL::calculateVoltageThreshold() {
//
//}
//
//void LCL::calculateCurrentThreshold() {
//
//}
//
//void LCL::changeCurrentThreshHold() {
//    PWM0_ChannelDutySet(PWM_CHANNEL_0, duty0);
//
//}
