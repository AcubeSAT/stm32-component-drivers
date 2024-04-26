#pragma once

#include <cstdint>
#include "peripheral/pwm/plib_pwm0.h"
#include "peripheral/pwm/plib_pwm1.h"

/**
 * @brief HAL_PWM class for controlling PWM peripherals.
 *
 * This class provides a high-level interface for controlling PWM peripherals.
 * It includes functions for starting and stopping channels and setting duty cycles.
 */
class HAL_PWM {
public:
    /**
     * @brief Start PWM channels for a specific peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param channelMask The mask indicating which channels to start.
     *
     * This function starts PWM channels for the specified peripheral.
     */
    template<uint8_t peripheralNumber>
    static void PWM_ChannelsStart(PWM_CHANNEL_MASK channelMask);

    /**
     * @brief Stop PWM channels for a specific peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param channelMask The mask indicating which channels to stop.
     *
     * This function stops PWM channels for the specified peripheral.
     */
    template<uint8_t peripheralNumber>
    static void PWM_ChannelsStop(PWM_CHANNEL_MASK channelMask);

    /**
     * @brief Set duty cycle for a specific channel of a peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param pwmChannel The PWM channel number.
     * @param dutyCycle The duty cycle value (0 to 65535).
     *
     * This function sets the duty cycle for a specific channel of the specified peripheral.
     */
    template<uint8_t peripheralNumber>
    static void PWM_ChannelDutySet(PWM_CHANNEL_NUM pwmChannel, uint16_t dutyCycle);

    /**
     * @brief Gets period foρ a specific channel of a peripheral.
     *
     * @tparam peripheralNumber The peripheral number (0 or 1).
     * @param pwmChannel The PWM channel number.
     */
    template<uint8_t peripheralNumber>
    static uint16_t PWM_ChannelPeriodGet(PWM_CHANNEL_NUM pwmChannel);

    /**
   * @brief Sets period for a specific channel of a peripheral.
   *
   * @tparam peripheralNumber The peripheral number (0 or 1).
   * @param pwmChannel The PWM channel number.
   */
    template<uint8_t peripheralNumber>
    static void PWM_ChannelPeriodSet(PWM_CHANNEL_NUM pwmChannel, uint16_t period);
};
