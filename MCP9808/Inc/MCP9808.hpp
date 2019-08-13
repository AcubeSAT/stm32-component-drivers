#ifndef MCP9808DRIVER_MCP9808_HPP
#define MCP9808DRIVER_MCP9808_HPP

#include "I2CDevice.h"
#include "MCP9808-constants.hpp"
#include <cstdint>

/**
 * MCP9808 temperature sensor driver
 *
 * This is a simple, (almost) feature-complete, documented (to the best of my ability) driver to use the
 * MCP9808 sensor on STM32 microcontrollers. All STM32-specific functions are used solely within the file
 * <b>MCP9808-internal.cpp</b> to allow for portability.
 *
 * Almost all settings are accessible via the provided functions, apart from whatever is a TODO,
 * of course ;-) using the constants provided in each function's javadoc string.
 * Tell me if I've missed something.
 *
 * For more details about the operation of the sensor, see the datasheet found at:
 * http://ww1.microchip.com/downloads/en/DeviceDoc/25095A.pdf
 *
 * @todo Create a function which enables setting a custom address other than 0b000
 * @todo Create functions for setting the T_LOWER, T_UPPER and T_CRIT registers
 *
 * @author Grigoris Pavlakis <grigpavl@ece.auth.gr>
 */

class MCP9808 : public I2CDevice {
    public:
        MCP9808(I2C_HandleTypeDef * i2c);

        /**
         * Set the hysteresis temperature (THYST)
         * Available options are: 0, 1.5, 3, 6 degrees Celsius
         * @param temp one of: MCP9808_CONFIG_THYST_0C, MCP9808_CONFIG_THYST_1_5C, MCP9808_CONFIG_THYST_3C,
         * MCP9808_CONFIG_THYST_6C
         */
        void setHystTemp(uint16_t temp);

        /**
        * Enter/exit low power mode (SHDN - shutdown mode)
        * @param setting one of: MCP9808_CONFIG_LOWPWR_ENABLE, MCP9808_CONFIG_LOWPWR_DISABLE
        */
        void setLowPwrMode(uint16_t setting);

        /**
        * Set locking status of the critical temperature (TCRIT) register
        * @param setting one of: MCP9808_CONFIG_TCRIT_LOCK_ENABLE, MCP9808_CONFIG_TCRIT_LOCK_DISABLE
        */
        void setCritTempLock(uint16_t setting);

        /**
        * Set locking status of the temperature window (T_UPPER, T_LOWER) registers
        * @param setting one of: MCP9808_CONFIG_WINLOCK_ENABLE, MCP9808_CONFIG_WINLOCK_DISABLE
        */
        void setTempWinLock(uint16_t setting);

        /**
         * Enable or disable temperature alerts.
         * If enabled, alert output is asserted as a comparator/interrupt or critical temperature output
         * @param setting one of: MCP9808_CONFIG_ALERT_ENABLE, MCP9808_CONFIG_ALERT_DISABLE
         */
        void setAlertStatus(uint16_t setting);

        /**
         * Enable or disable alert control mode.
         * @param setting one of: MCP9808_CONFIG_ALERT_CONTROL_ENABLE, MCP9808_CONFIG_ALERT_CONTROL_DISABLE
         */
        void setAlertControl(uint16_t setting);

        /**
         * Select the event for which an alert will be emitted, if triggered.
         * If set to MCP9808_CONFIG_ALERT_SELECT_CRITONLY, then an alert is emitted only when
         * T_ambient > T_crit.
         * @param setting one of: MCP9808_CONFIG_ALERT_SELECT_CRITONLY, CONFIG_ALERT_SELECT_ALL
         */
        void setAlertSelection(uint16_t setting);

        /**
         * Set the polarity of the emitted alert (active-low or active-high)
         * @param setting one of: MCP9808_CONFIG_ALERT_POLARITY_ACTIVE_HI, MCP9808_CONFIG_ALERT_POLARITY_ACTIVE_LOW
         */
        void setAlertPolarity(uint16_t setting);

        /**
         * Set the alert mode (comparator or interrupt output)
         * @param setting one of: MCP9808_CONFIG_ALERT_MODE_IRQ, MCP9808_CONFIG_ALERT_MODE_COMPARATOR
         */
        void setAlertMode(uint16_t setting);

        /**
        * Set the interrupts to be cleared on the next read attempt (namely, a temperature
        * reading or a command in general)
        */
        void clearInterrupts();

        /**
         * Set the measurement resolution.
         * @param setting one of: MCP9808_RES_0_50C, MCP9808_RES_0_25C, MCP9808_RES_0_125C, MCP9808_RES_0_0625C
         */
        void setResolution(uint16_t setting);

        /**
         * Get the current temperature reading (in Celsius)
         * @return The temperature
         */
        float getTemp();
};

#endif //MCP9808DRIVER_MCP9808_HPP
