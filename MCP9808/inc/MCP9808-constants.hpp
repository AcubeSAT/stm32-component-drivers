#pragma once

/**
 * @author Grigoris Pavlakis <grigpavl@ece.auth.gr>
 * @author Dimitrios Sourlantzis <sourland@ece.auth.gr>
 */

#ifndef MCP9808DRIVER_MCP9808_CONSTANTS_HPP
#define MCP9808DRIVER_MCP9808_CONSTANTS_HPP

/**
 * User constants - FOR USE IN FUNCTION CALLS AND CONFIGURATION
 */

/**
 * An enumeration containing all the device's register addresses.
 */
typedef enum {
    MCP9808_REG_RFU = 0x00u,
    MCP9808_REG_CONFIG = 0x01u,
    MCP9808_REG_TUPPER = 0x02u,
    MCP9808_REG_TLOWER = 0x03u,
    MCP9808_REG_TCRIT = 0x04u,
    MCP9808_REG_TEMP = 0x05u,
    MCP9808_REG_MFGID = 0x06u,
    MCP9808_REG_DEVID = 0x07u,
    MCP9808_REG_RESOLUTION = 0x08u
} registerAddresses;

/**
 * An enumeration containing configuration constants used only for configuration operations to avoid overwriting critical data
 */
typedef enum {
    MCP9808_CONFIG_THYST_0C = 0x000u,
    MCP9808_CONFIG_THYST_1_5C = 0x200u,
    MCP9808_CONFIG_THYST_3C = 0x400u,
    MCP9808_CONFIG_THYST_6C = 0x600u
} hysteresisTemperatureOptions;

/**
 * An enumeration containing all low-power mode (SHDN) options.
 */
typedef enum {
    MCP9808_CONFIG_LOWPWR_ENABLE = 0x100,
    MCP9808_CONFIG_LOWPWR_DISABLE = 0x000
} lowPowerMode;

/**
 * An enumeration containing all critical temperature register locking options.
 */
typedef enum {
    MCP9808_CONFIG_TCRIT_LOCK_ENABLE = 0x80,
    MCP9808_CONFIG_TCRIT_LOCK_DISABLE = 0x00
} criticalTemperatureRegisterLocking;

/**
 * An enumeration containing all WINLOCK T_upper and T_lower temperature window locking options.
 */
typedef enum {
    MCP9808_CONFIG_WINLOCK_ENABLE = 0x40,
    MCP9808_CONFIG_WINLOCK_DISABLE = 0x00
} temperatureWindowLocking;

/**
 * A variable containing a value to set interrupts to be cleared on next read of CONFIG register.
 */
inline auto const MCP9808_CONFIG_IRQ_CLEAR = 0x20;

/**
 *  An enumeration containing all output status options, see datasheet p. 19.
 */
typedef enum {
    MCP9808_CONFIG_ALERT_ENABLE = 0x10,
    MCP9808_CONFIG_ALERT_DISABLE = 0x00
} alertOutputStatus;

/**
 * An enumeration containing all the output control options.
 */
typedef enum {
    MCP9808_CONFIG_ALERT_CONTROL_DISABLE = 0x08,
    MCP9808_CONFIG_ALERT_CONTROL_ENABLE = 0x00
} alertOutputControl;

/**
 * An enumeration containing all the alert output selection options.
 */
typedef enum {
    MCP9808_CONFIG_ALERT_SELECT_CRITONLY = 0x04,
    MCP9808_CONFIG_ALERT_SELECT_ALL = 0x00
} alertOutputSelection;

/**
 * An enumeration containing all the alert output modes.
 */
typedef enum {
    MCP9808_CONFIG_ALERT_MODE_IRQ = 0x01,
    MCP9808_CONFIG_ALERT_MODE_COMPARATOR = 0x00
} alertOutputMode;

/**
 * An enumeration containing all the polarity of alerts options.
 */
typedef enum {
    MCP9808_CONFIG_ALERT_POLARITY_ACTIVE_HIGH = 0x02,
    MCP9808_CONFIG_ALERT_POLARITY_ACTIVE_LOW = 0x00
} alertPolarity;

/**
 * An enumeration containing all the measurement resolution options.
 */
typedef enum {
    MCP9808_RES_0_50C = 0x00,
    MCP9808_RES_0_25C = 0x01,
    MCP9808_RES_0_125C = 0x02,
    MCP9808_RES_0_0625C = 0x03
} measurementResolution;

/**
 * User defined I2C address bits A2-A1-A0, see datasheet for acceptable values
 */
inline const auto MCP9808_I2C_USER_ADDR = 0x00;

/**
 * System constants - INTERNAL USE ONLY!
 */

/**
 * An enumeration containing all the configuration masks.
 */
typedef enum {
    MCP9808_CONFIG_TCRIT_LOCK_MASK = 0xFF7Fu,
    MCP9808_CONFIG_WINLOCK_MASK = 0xFFBFu,
    MCP9808_CONFIG_IRQ_CLEAR_MASK = 0xFFDFu,
    MCP9808_CONFIG_ALERT_STATUS_MASK = 0xFFEFu,
    MCP9808_CONFIG_ALERT_CONTROL_MASK = 0xFFF7u,
    MCP9808_CONFIG_ALERT_SELECT_MASK = 0xFFFBu,
    MCP9808_CONFIG_ALERT_POLARITY_MASK = 0xFFFDu,
    MCP9808_CONFIG_ALERT_MODE_MASK = 0xFFFEu,
    MCP9808_RES_MASK = 0x00FC
} masks;

/**
 * A variable containing the base slave bus address manufacturer-defined, used on the I2C bus.
 */
inline const auto MCP9808_I2C_BASE_ADDR = 0x18u;

/**
 * A variable containing a bit mask to enable changes only to address bits 2-4 which are user-settable.
 */
inline const auto MCP9808_I2C_USER_ADDR_MASK = 0x78u;

/**
 * A variable contating the custom bus address for usage in read-write requests.
 */
inline const auto MCP9808_I2C_BUS_ADDR = static_cast<uint16_t>(MCP9808_I2C_BASE_ADDR & MCP9808_I2C_USER_ADDR_MASK |
                                                               MCP9808_I2C_USER_ADDR);

/**
 * A variable containing the manufacturer's ID.
 */
inline const auto MCP9808_MANUFACTURER_ID = 0x0054u;

/**
 * A variable containing the hysteresis temperature bit mask.
 */
inline const auto MCP9808_CONFIG_THYST_MASK = 0xF9FFu;

/**
 * A variable contatining the low-power SHDN mode bit mask.
 */
inline const auto MCP9808_CONFIG_SHDN_MASK = 0xFEFFu;


#endif //MCP9808DRIVER_MCP9808_CONSTANTS_HPP
