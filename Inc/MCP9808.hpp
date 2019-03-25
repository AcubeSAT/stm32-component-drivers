#ifndef MCP9808DRIVER_MCP9808_HPP
#define MCP9808DRIVER_MCP9808_HPP

#include "stm32l4xx_hal.h"
#include "MCP9808-constants.hpp"
#include <cstdint>
#include <arm_math.h>

class MCP9808 {
    /**
     * I2C handle provided by ST
     */
    I2C_HandleTypeDef* i2c;

    private:
        /**
        * Write a value to a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param data the data octets to be written
        */
        void writeReg(uint8_t addr, uint16_t &data);

        void setReg(uint8_t addr, uint16_t mask, uint16_t setting);

    public:
        /**
        * Initialize a new controller object for MCP9808
        * @param i2c handle to the appropriately-set and enabled I2C peripheral
        */
        MCP9808(I2C_HandleTypeDef* i2c);

        /**
        * Read a value from a register (see the constants in MCP9808-constants.hpp)
        * @param addr the address of the desired register
        * @param buffer the variable where the data will be stored
        */
        void readReg(uint8_t addr, uint16_t &result);

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
         * @param setting one of: CONFIG_ALERT_POLARITY_ACTIVE_HI, CONFIG_ALERT_POLARITY_ACTIVE_LOW
         */
        void setAlertPolarity(uint16_t setting);

        /**
         * Set the alert mode (comparator or interrupt output)
         * @param setting one of: CONFIG_ALERT_MODE_IRQ, CONFIG_ALERT_MODE_COMPARATOR
         */
        void setAlertMode(uint16_t setting);

        /**
        * Set the interrupts to be cleared on the next read attempt (namely, a temperature
        * reading or a command in general)
        */
        void clearInterrupts();

        /**
         * Get the current temperature reading (in Celsius)
         * @param result the variable where the result is going to be stored
         */
        void getTemp(float32_t &result);
};

#endif //MCP9808DRIVER_MCP9808_HPP
