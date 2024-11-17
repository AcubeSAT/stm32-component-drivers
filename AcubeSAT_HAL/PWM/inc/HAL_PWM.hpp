#pragma once

#include <cstdint>
#include "peripheral/pwm/plib_pwm0.h"
#include "peripheral/pwm/plib_pwm1.h"

enum class PeripheralNumber : uint8_t {
 Peripheral_0 ,
 Peripheral_1
};

/**
 * @brief HAL_PWM namespace for controlling PWM peripherals.
 *
 * This namespace provides a high-level interface for controlling PWM peripherals.
 * It includes functions for starting and stopping channels and setting duty cycles.
 */
namespace HAL_PWM {
    /**
     * @brief Start PWM channels for a specific peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param channelMask The mask indicating which channels to start.
     *
     * This function starts PWM channels for the specified peripheral.
     */
    template<PeripheralNumber peripheral>
    void PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask);

    /**
     * @brief Stop PWM channels for a specific peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param channelMask The mask indicating which channels to stop.
     *
     * This function stops PWM channels for the specified peripheral.
     */
    template<PeripheralNumber peripheral>
    void PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask);

    /**
     * @brief Set duty cycle for a specific channel of a peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param pwmChannel The PWM channel number.
     * @param dutyCycle The duty cycle value (0 to 65535).
     *
     * This function sets the duty cycle for a specific channel of the specified peripheral.
     */
    template<PeripheralNumber peripheral>
    void PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle);

    /**
     * @brief Gets period for a specific channel of a peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param pwmChannel The PWM channel number.
     */
    template<PeripheralNumber peripheral>
    uint16_t PWM_ChannelPeriodGet(PWM_CHANNEL_NUM pwmChannel);

    /**
     * @brief Sets period for a specific channel of a peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param pwmChannel The PWM channel number.
     * @param period The period value (0 to 65535).
     */
    template<PeripheralNumber peripheral>
    void PWM_ChannelPeriodSet(PWM_CHANNEL_NUM pwmChannel, uint16_t period);
}
