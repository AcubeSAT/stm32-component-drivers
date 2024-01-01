#include "HAL_PWM.hpp"

template<uint8_t PeripheralNumber>
void HAL_PWM::PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask) {
    static_assert(PeripheralNumber == 0 || PeripheralNumber == 1, "Template parameter must be 0 or 1");
}

template<>
void HAL_PWM::PWM_ChannelsStart<0>(PWM_CHANNEL_MASK channelMask) {
    PWM0_ChannelsStart(channelMask);
}

template<>
void HAL_PWM::PWM_ChannelsStart<1>(PWM_CHANNEL_MASK channelMask) {
    PWM1_ChannelsStart(channelMask);
}

template<uint8_t PeripheralNumber>
void HAL_PWM::PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask) {
    static_assert(PeripheralNumber == 0 || PeripheralNumber == 1, "Template parameter must be 0 or 1");
}

template<>
void HAL_PWM::PWM_ChannelsStop<0>(PWM_CHANNEL_MASK channelMask) {
    PWM0_ChannelsStop(channelMask);
}

template<>
void HAL_PWM::PWM_ChannelsStop<1>(PWM_CHANNEL_MASK channelMask) {
    PWM1_ChannelsStop(channelMask);
}

template<uint8_t PeripheralNumber>
void HAL_PWM::PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t duty) {
    static_assert(PeripheralNumber == 0 || PeripheralNumber == 1, "Template parameter must be 0 or 1");
}

template<>
void HAL_PWM::PWM_ChannelDutySet<0>(PWM_CHANNEL_NUM pwmChannel, uint16_t duty) {
    PWM0_ChannelDutySet(pwmChannel, duty);
}

template<>
void HAL_PWM::PWM_ChannelDutySet<1>(PWM_CHANNEL_NUM pwmChannel, uint16_t duty) {
    PWM1_ChannelDutySet(pwmChannel, duty);
}

