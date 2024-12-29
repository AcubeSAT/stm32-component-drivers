#include "HAL_PWM.hpp"

namespace HAL_PWM {
    template<PeripheralNumber peripheral>
    void PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask) {}

    template<>
    void PWM_ChannelsStart<PeripheralNumber::Peripheral_0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStart(channelMask);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask) {}

    template<>
    void PWM_ChannelsStop<PeripheralNumber::Peripheral_0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStop(channelMask);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {}

    template<>
    void PWM_ChannelDutySet<PeripheralNumber::Peripheral_0>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM0_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<PeripheralNumber peripheral>
    uint16_t PWM_ChannelPeriodGet(PWM_CHANNEL_NUM pwmChannel) {}

    template<>
    uint16_t PWM_ChannelPeriodGet<PeripheralNumber::Peripheral_0>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM0_ChannelPeriodGet(pwmChannel);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelPeriodSet(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {}

    template<>
    void PWM_ChannelPeriodSet<PeripheralNumber::Peripheral_0>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM0_ChannelPeriodSet(pwmChannel, period);
    }

#ifdef HAL_PWM_1
    template<>
    void PWM_ChannelsStart<PeripheralNumber::Peripheral_1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStart(channelMask);
    }

    template<>
    void PWM_ChannelsStop<PeripheralNumber::Peripheral_1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStop(channelMask);
    }

    template<>
    void PWM_ChannelDutySet<PeripheralNumber::Peripheral_1>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM1_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<>
    uint16_t PWM_ChannelPeriodGet<PeripheralNumber::Peripheral_1>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM1_ChannelPeriodGet(pwmChannel);
    }

    template<>
    void PWM_ChannelPeriodSet<PeripheralNumber::Peripheral_1>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM1_ChannelPeriodSet(pwmChannel, period);
    }
#endif
}