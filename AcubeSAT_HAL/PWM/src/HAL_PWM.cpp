#include "HAL_PWM.hpp"

namespace HAL_PWM {
    template<PeripheralNumber peripheral>
    void PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask) {}

    template<>
    void PWM_ChannelsStart<0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStart(channelMask);
    }

    template<>
    void PWM_ChannelsStart<1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStart(channelMask);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask) {}

    template<>
    void PWM_ChannelsStop<0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStop(channelMask);
    }

    template<>
    void PWM_ChannelsStop<1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStop(channelMask);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {}

    template<>
    void PWM_ChannelDutySet<0>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM0_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<>
    void PWM_ChannelDutySet<1>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM1_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<PeripheralNumber peripheral>
    uint16_t PWM_ChannelPeriodGet(PWM_CHANNEL_NUM pwmChannel) {}

    template<>
    uint16_t PWM_ChannelPeriodGet<0>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM0_ChannelPeriodGet(pwmChannel);
    }

    template<>
    uint16_t PWM_ChannelPeriodGet<1>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM1_ChannelPeriodGet(pwmChannel);
    }

    template<PeripheralNumber peripheral>
    void PWM_ChannelPeriodSet(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {}

    template<>
    void PWM_ChannelPeriodSet<0>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM0_ChannelPeriodSet(pwmChannel, period);
    }

    template<>
    void PWM_ChannelPeriodSet<1>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM1_ChannelPeriodSet(pwmChannel, period);
    }
}