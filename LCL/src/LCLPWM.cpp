#include "LCLPWM.hpp"
#include "HAL_PWM.hpp"

LCLPWM::LCLPWM(PWM_CHANNEL_NUM pwmChannel, PWM_CHANNEL_MASK pwmChannelMask, PIO_PIN resetPin, PIO_PIN setPin,
                uint8_t peripheralNumber, PWMThreshold dutyCycles) : LCL(resetPin, setPin), pwmChannel(pwmChannel),
                                                                    pwmChannelMask(pwmChannelMask),
                                                                    peripheralNumber(peripheralNumber),
                                                                    voltageSetting(static_cast<uint16_t>(dutyCycles)) {
    setCurrentThreshold(voltageSetting);
    disableLCL();
}

void LCLPWM::enableLCL() {
    PIO_PinWrite(resetPin, true);
    HAL_PWM::PWM_ChannelsStart<0>(pwmChannelMask);
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

void LCLPWM::disableLCL() {
    HAL_PWM::PWM_ChannelsStop<0>(pwmChannelMask);
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}

void LCLPWM::setCurrentThreshold(uint16_t dutyCyclePercent) {
    HAL_PWM::PWM_ChannelDutySet<0>(pwmChannel, ConstantInPWMRegister * dutyCyclePercent / 100);
}
