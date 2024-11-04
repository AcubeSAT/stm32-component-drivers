#include "HAL_PWM.hpp"

typedef enum : uint8_t {
    peripheral_0 = 0,
    peripheral_1 = 1
  }peripheralNumber;

namespace HAL_PWM {
    template<peripheralNumber peripheral>
    void PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask) {
        static_assert(peripheral == peripheral_0 || peripheral == peripheral_1 , "Template parameter must be 0 or 1");
    }

    template<>
    void PWM_ChannelsStart<0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStart(channelMask);
    }

    template<>
    void PWM_ChannelsStart<1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStart(channelMask);
    }

    template<peripheralNumber peripheral>
    void PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask) {
        static_assert(peripheral == peripheral_0 || peripheral == peripheral_1, "Template parameter must be 0 or 1");
    }

    template<>
    void PWM_ChannelsStop<0>(PWM_CHANNEL_MASK channelMask) {
        PWM0_ChannelsStop(channelMask);
    }

    template<>
    void PWM_ChannelsStop<1>(PWM_CHANNEL_MASK channelMask) {
        PWM1_ChannelsStop(channelMask);
    }

    template<peripheralNumber peripheral>
    void PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        static_assert(peripheral == peripheral_0 || peripheral == peripheral_1, "Template parameter must be 0 or 1");
    }

    template<>
    void PWM_ChannelDutySet<0>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM0_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<>
    void PWM_ChannelDutySet<1>(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle) {
        PWM1_ChannelDutySet(pwmChannel, dutyCycle);
    }

    template<peripheralNumber peripheral>
    uint16_t PWM_ChannelPeriodGet(PWM_CHANNEL_NUM pwmChannel) {
        static_assert(peripheral == peripheral_0 || peripheral == peripheral_1, "Template parameter must be 0 or 1");
    }

    template<>
    uint16_t PWM_ChannelPeriodGet<0>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM0_ChannelPeriodGet(pwmChannel);
    }

    template<>
    uint16_t PWM_ChannelPeriodGet<1>(PWM_CHANNEL_NUM pwmChannel) {
        return PWM1_ChannelPeriodGet(pwmChannel);
    }

    template<peripheralNumber peripheral>
    void PWM_ChannelPeriodSet(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        static_assert(peripheral == peripheral_0 || peripheral == peripheral_1    , "Template parameter must be 0 or 1");
    }

    template<>
    void PWM_ChannelPeriodSet<0>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM0_ChannelPeriodSet(pwmChannel, period);
    }

    template<>
    void PWM_ChannelPeriodSet<1>(PWM_CHANNEL_NUM pwmChannel, uint16_t period) {
        PWM1_ChannelPeriodSet(pwmChannel, period);
    }
}