#pragma once

#include <cstdint>
#include <etl/span.h>
#include <etl/expected.h>
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"
#include "Peripheral_Definitions.hpp"

/**
 * The MCP9808_TWI_PORT definition is used to select which TWI peripheral of the ATSAMV71Q21B MCU will be used.
 * By giving the corresponding value to MCP9808_TWI_PORT, the user can choose between TWI0, TWI1 or TWI2 respectively.
 * For the OBC microcontroller MCP9808_TWI_PORT = 1,
 * For the ADCS microcontroller and ATSAMV71 development board, MCP9808_TWI_PORT = 2.
 * Each subsystem shall define MCP9808_TWI_PORT in a platform specific header file.
 */
#if MCP9808_TWI_PORT == 0

#include "plib_twihs0_master.h"
#define MCP9808_TWIHS_Write TWIHS0_Write
#define MCP9808_TWIHS_ErrorGet TWIHS0_ErrorGet
#define MCP9808_TWIHS_Read TWIHS0_Read
#define MCP9808_TWIHS_Initialize TWIHS0_Initialize
#define MCP9808_TWIHS_IsBusy TWIHS0_IsBusy

#elif MCP9808_TWI_PORT == 1

#include "plib_twihs1_master.h"
#define MCP9808_TWIHS_Write TWIHS1_Write
#define MCP9808_TWIHS_ErrorGet TWIHS1_ErrorGet
#define MCP9808_TWIHS_Read TWIHS1_Read
#define MCP9808_TWIHS_Initialize TWIHS1_Initialize
#define MCP9808_TWIHS_IsBusy TWIHS1_IsBusy

#elif MCP9808_TWI_PORT == 2

#include "plib_twihs2_master.h"

#define MCP9808_TWIHS_Write TWIHS2_Write
#define MCP9808_TWIHS_ErrorGet TWIHS2_ErrorGet
#define MCP9808_TWIHS_Read TWIHS2_Read
#define MCP9808_TWIHS_Initialize TWIHS2_Initialize
#define MCP9808_TWIHS_IsBusy TWIHS2_IsBusy
#endif

/**
 * MCP9808 temperature sensor driver by Grigorios Pavlakis and Dimitrios Sourlantzis
 *
 * This is a simple driver to use the MCP9808 sensor on ATSAMV71Q21B microcontrollers. All Microchip-specific
 * functions are used solely within the core read and write functions.
 *
 * For more details about the operation of the sensor, see the datasheet found at:
 * http://ww1.microchip.com/downloads/en/DeviceDoc/25095A.pdf
 *
 * This is a modified version of the already existed driver writen for STM32 microcontrollers by Grigorios Pavlakis and can be found at:
 * https://gitlab.com/acubesat/obc/atsam-component-drivers/-/tree/old-stm32
 */
class MCP9808 {
public:
    /**
     * Set the I2C address depending on the pin configuration of the physical device
     * @see I2C_USER_ADDRESS
     * @param i2cUserAddress user selectable address
     */
    explicit MCP9808(uint8_t i2cUserAddress) : I2C_USER_ADDRESS(i2cUserAddress) {}

    /**
     * Configuration constants used only for configuration operations to avoid overwriting critical data, refer to datasheet section 5.1.1.
     */

    /**
     * Hysteresis temperature options.
     */
    enum HysteresisTemperatureOptions {
        THYST_0C = 0x000u,
        THYST_1_5C = 0x200u,
        THYST_3C = 0x400u,
        THYST_6C = 0x600u
    };

    /**
     * Every measurement resolution option.
     */
    enum MeasurementResolution {
        RES_0_50C = 0x00,
        RES_0_25C = 0x01,
        RES_0_125C = 0x02,
        RES_0_0625C = 0x03
    };

    /**
     * Specifies the type of error occured in reading and writing operations
     */
    enum class Error : uint8_t {
        ERROR_NONE = TWIHS_ERROR_NONE,
        ERROR_NACK = TWIHS_ERROR_NACK,
        READ_REQUEST_FAILED,
        WRITE_REQUEST_FAILED
    };

    /**
     * Enter low power mode (SHDN - shutdown mode)
     */
    etl::expected<void, Error> enableLowPowerMode();

    /**
     * Exit low power mode (SHDN - shutdown mode)
     */
    etl::expected<void, Error> disableLowPowerMode();

    /**
     * Enable locking of the critical temperature (TCRIT) register
     */
    etl::expected<void, Error> enableCriticalTemperatureLock();

    /**
     * Disable locking of the critical temperature (TCRIT) register
     */
    etl::expected<void, Error> disableCriticalTemperatureLock();

    /**
     * Enable locking of the temperature window (T_UPPER, T_LOWER) registers
     */
    etl::expected<void, Error> enableTemperatureWindowLock();

    /**
     * Disable locking of the temperature window (T_UPPER, T_LOWER) registers
     */
    etl::expected<void, Error> disableTemperatureWindowLock();

    /**
     * Enable temperature alerts.
     * Alert output is asserted as a comparator/interrupt or critical temperature
     */
    etl::expected<void, Error> enableAlertStatus();

    /**
     * Disable temperature alerts.
     * Alert output is not asserted by the device (power-up default)
     */
    etl::expected<void, Error> disableAlertStatus();

    /**
     * Enable alert control mode.
     */
    etl::expected<void, Error> enableAlertControl();

    /**
     * Disable alert control mode.
     */
    etl::expected<void, Error> disableAlertControl();

    /**
     * An alert is emitted only when T_ambient > T_crit (T_UPPER and T_LOWER temperature boundaries are disabled)
     */
    etl::expected<void, Error> setAlertSelectionOnCriticalTemperature();

    /**
     * Alert output for T_UPPER, T_LOWER and T_CRIT (power-up default)
     */
    etl::expected<void, Error> setAlertSelectionOnAll();

    /**
     * Set the polarity of the emitted alert active-high
     */
    etl::expected<void, Error> setAlertPolarityActiveHigh();

    /**
     * Set the polarity of the emitted alert active-low
     */
    etl::expected<void, Error> setAlertPolarityActiveLow();

    /**
     * Set the alert mode to comparator output
     */
    etl::expected<void, Error> setAlertModeComparator();

    /**
     * Set the alert mode to interrupt output
     */
    etl::expected<void, Error> setAlertModeInterrupt();

    /**
     * Set the hysteresis temperature (THYST)
     * Available options are: 0, 1.5, 3, 6 degrees Celsius
     * @param option desired hysteresis temperature option
     */
    etl::expected<void, Error> setHysteresisTemperature(HysteresisTemperatureOptions option);

    /**
     * Set the interrupts to be cleared on the next read attempt (namely, a temperature
     * reading or a command in general)
     */
    etl::expected<void, Error> clearInterrupts();

    /**
     * Set the measurement resolution. Since the bits of interest in are located in the less significant byte, and
     * the High-Speed Two-Wired Interface (TWIHS) protocol reads MSB-first, while the register fits only 8 bits, t
     * he input is shifted by 8 bits to transfer the data bits to the MSB part and thus store them.
     * @param setting the desired measurement resolution option
     */
    etl::expected<void, Error> setResolution(MeasurementResolution setting);

    /**
     * Set upper temperature limit
     * @param data the desired upper temperature limit with format as specified at page 21 of the datasheet
     */
    etl::expected<void, Error> setUpperTemperatureLimit(float temp);

    /**
     * Set lower temperature limit
     * @param data the desired lower temperature limit with format as specified at page 21 of the datasheet
     */
    etl::expected<void, Error> setLowerTemperatureLimit(float temp);

    /**
     * Set critical temperature limit
     * @param data the desired critical temperature limit with format as specified at page 21 of the datasheet
     */
    etl::expected<void, Error> setCriticalTemperatureLimit(float temp);

    /**
     * Get the current temperature reading (in Celsius)
     * @returns the current temperature
     */
    etl::expected<float, Error> getTemperature();

    /**
     * Check the Manufacturer ID register against the expected value.
     * @return Returns true if the device is connected and responds correctly.
     */
    etl::expected<bool, Error> isDeviceConnected();

    /**
     * Getter function
     * @return the I2C_USER_ADDRESS private variable
     */
    inline uint8_t getI2CUserAddress() const {
        return I2C_USER_ADDRESS;
    }

private:
    /**
     * Low-power mode (SHDN) options.
     */
    enum LowPowerMode {
        LOWPWR_ENABLE = 0x100,
        LOWPWR_DISABLE = 0x000
    };

    /**
     * Critical temperature register locking options.
     */
    enum CriticalTemperatureRegisterLock {
        TCRIT_LOCK_ENABLE = 0x80,
        TCRIT_LOCK_DISABLE = 0x00
    };

    /**
     * WINLOCK T_upper and T_lower temperature window locking options.
     */
    enum TemperatureWindowLock {
        WINLOCK_ENABLE = 0x40,
        WINLOCK_DISABLE = 0x00
    };

    /**
     * Output status options, see datasheet p. 19.
     */
    enum AlertStatus {
        ALERT_ENABLE = 0x10,
        ALERT_DISABLE = 0x00
    };

    /**
     * Every output control option.
     */
    enum AlertControl {
        ALERT_CONTROL_DISABLE = 0x08,
        ALERT_CONTROL_ENABLE = 0x00
    };

    /**
     * Every alert output selection option.
     */
    enum AlertSelection {
        ALERT_SELECT_CRITONLY = 0x04,
        ALERT_SELECT_ALL = 0x00
    };

    /**
     * Every alert output modes.
     */
    enum AlertMode {
        ALERT_MODE_IRQ = 0x01,
        ALERT_MODE_COMPARATOR = 0x00
    };

    /**
     * Every polarity of alerts option.
     */
    enum AlertPolarity {
        ALERT_POLARITY_ACTIVE_HIGH = 0x02,
        ALERT_POLARITY_ACTIVE_LOW = 0x00
    };

    /**
     * The maximum number of bytes to write via High Speed Two-Wired Interface
     */
    enum NumOfBytesToTransfer : uint8_t {
        TRANSFER_1BYTE = 1,
        TRANSFER_2BYTES = 2,
        TRANSFER_3BYTES = 3
    };

    /**
     * Wait period before a sensor read is skipped
     */
    const uint8_t TIMEOUT_TICKS = 100;

    /**
     * User constants - FOR USE IN FUNCTION CALLS AND CONFIGURATION
     */

    /**
     * MCP9808 temperature sensor register addresses.
     */
    enum class Register : uint8_t {
        REG_RFU = 0x00u,
        REG_CONFIG = 0x01u,
        REG_TUPPER = 0x02u,
        REG_TLOWER = 0x03u,
        REG_TCRIT = 0x04u,
        REG_TEMP = 0x05u,
        REG_MFGID = 0x06u,
        REG_DEVID = 0x07u,
        REG_RESOLUTION = 0x08u
    };

    /**
     * The required value in order to set interrupts to be cleared on next read of CONFIG register.
     */
    static constexpr uint8_t IRQ_CLEAR = 0x20;

    /**
     * User defined I2C address bits A2-A1-A0, see datasheet for acceptable values
     */
    const uint8_t I2C_USER_ADDRESS = 0x00;

    /**
     * System constants - INTERNAL USE ONLY!
     */

    /**
     * Configuration masks
     */
    enum class Mask : uint16_t {
        TCRIT_LOCK_MASK = 0xFF7Fu,
        WINLOCK_MASK = 0xFFBFu,
        IRQ_CLEAR_MASK = 0xFFDFu,
        ALERT_STATUS_MASK = 0xFFEFu,
        ALERT_CONTROL_MASK = 0xFFF7u,
        ALERT_SELECT_MASK = 0xFFFBu,
        ALERT_POLARITY_MASK = 0xFFFDu,
        ALERT_MODE_MASK = 0xFFFEu,
        RES_MASK = 0x00FC,
        THYST_MASK = 0xF9FFu,
        SHDN_MASK = 0xFEFFu,
        TUPPER_TLOWER_TCRIT_MASK = 0x1FFCu
    };

    /**
     * Base slave bus address manufacturer-defined, used on the I2C bus.
     */
    const uint8_t I2C_BASE_ADDRESS = 0x18u;

    /**
     * Bit mask to enable changes only to addresses bits 2-4 which are user-settable.
     */
    const uint8_t I2C_USER_ADDRESS_MASK = 0x78u;

    /**
     * Custom bus address for usage in read-write requests.
     */
    const uint8_t I2C_BUS_ADDRESS = static_cast<uint16_t>(I2C_BASE_ADDRESS & I2C_USER_ADDRESS_MASK |
                                                          I2C_USER_ADDRESS);

    /**
     * Manufacturer's ID.
     */
    static constexpr uint8_t MANUFACTURER_ID = 0x0054u;

    /**
     * High Speed Two-Wired Interface transaction error
     */
    TWIHS_ERROR error;

    /**
     * Enter/exit low power mode (SHDN - shutdown mode)
     * @param setting the desired low power mode option
     */
    etl::expected<void, Error> setLowPowerMode(LowPowerMode setting);

    /**
     * Set locking status of the critical temperature (TCRIT) register
     * @param setting the desired critical temperature locking option
     */
    etl::expected<void, Error> setCriticalTemperatureLock(CriticalTemperatureRegisterLock setting);

    /**
     * Set locking status of the temperature window (T_UPPER, T_LOWER) registers
     * @param setting the desired locking status option
     */
    etl::expected<void, Error> setTemperatureWindowLock(TemperatureWindowLock setting);

    /**
     * Enable or disable temperature alerts.
     * If enabled, alert output is asserted as a comparator/interrupt or critical temperature
     * @param setting the desired alert status option
     */
    etl::expected<void, Error> setAlertStatus(AlertStatus setting);

    /**
     * Enable or disable alert control mode.
     * @param setting the desired alert control option
     */
    etl::expected<void, Error> setAlertControl(AlertControl setting);

    /**
     * Select the event for which an alert will be emitted, if triggered.
     * If set to CONFIG_ALERT_SELECT_CRITONLY, then an alert is emitted only when
     * T_ambient > T_crit.
     * @param setting the desired alert selection option
     */
    etl::expected<void, Error> setAlertSelection(AlertSelection setting);

    /**
     * Set the polarity of the emitted alert (active-low or active-high)
     * @param setting desired alert polarity option
     */
    etl::expected<void, Error> setAlertPolarity(AlertPolarity setting);

    /**
     * Set the alert mode (comparator or interrupt output)
     * @param setting the desired alert mode option
     */
    etl::expected<void, Error> setAlertMode(AlertMode setting);

    /**
     * Write a value to a register. Microchip-specific functions are used.
     * NOTE: this writes data as they are, so be careful!
     *
     * @param data the data octets to be written
     * @param numOfBytes the number of bytes to be written
     */
    etl::expected<void, Error>  writeRegister(etl::span<uint8_t> data);

    /**
     * Read a value from a register. About register reading operations
     * in MCP9808, refer to documentation page 21, figure 5.3. Microchip-specific functions are used.
     * @param address the address of the desired register
     * @param data a variable to save the data from the desired register
     */
    etl::expected<uint16_t, Error> readRegister(Register address);

    /**
     * Safely change a setting on the register
     * This is the recommended function to use when changing settings,
     * and is used in all public functions that change settings.
     *
     * @param address the address of the desired register
     * @param mask the appropriate bitmask to access the particular
     * setting bit or group of bits
     * @param setting the new value of the setting to be changed
     * (also found in mcp9808-constants.hpp)
     */
    etl::expected<void, Error> setRegister(Register address, Mask mask, uint16_t setting);

    /**
     * Function that prevents hanging when a I2C device is not responding.
     */
    inline void waitForResponse() const {
        const auto Start = xTaskGetTickCount();
        while (MCP9808_TWIHS_IsBusy()) {
            if (xTaskGetTickCount() - Start > TIMEOUT_TICKS) {
                LOG_ERROR << "Temperature sensor with address " << I2C_USER_ADDRESS
                          << " has timed out";
                MCP9808_TWIHS_Initialize();
            }
            taskYIELD()
        }
    };

    /**
     * Converts floating point number to the binary representation
     * specified at page 22 of the datasheet.
     *
     * @param f the floating point number to be converted
     * @return the binary representation
     */
    static uint16_t floatConversion(float floatToConvert);
};
