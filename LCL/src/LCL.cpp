#include "LCL.hpp"

//void LCL::returnLCLStatus() {
//    PWM0_ChannelPeriodGet();
//    calculateCurrentThreshold();
//
//    return struct
//}

void LCL::enableLCL() {
    // Todo assert status of GPIOs/log
    PIO_PinWrite(resetPin, true); // RESET
    PWM0_ChannelsStart(pwmChannelMask);

    PIO_PinWrite(setPin, false); // enable set
    SYSTICK_DelayMs(100); // any better ideas?
    PIO_PinWrite(setPin, true); // disable set
    lclEnabled = true;
}


void LCL::disableLCL() {
    // Todo assert status of GPIOs/log
    PWM0_ChannelsStop(pwmChannelMask);
    PIO_PinWrite(PIO_PIN_PB2, false); // enable RESET
    lclEnabled = false;
}

void LCL::calculateVoltageThreshold() {

}

void LCL::calculateCurrentThreshold() {

}

//void LCL::changeCurrentThreshHold(uint16_t newDCurrentThreshold) {
//    changePWMDutyCycle(newDCurrentThreshold * some magic / other magic);
//}
