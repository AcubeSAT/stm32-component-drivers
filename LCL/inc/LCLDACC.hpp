#pragma once

#include "LCL.hpp"
#include "peripheral/dacc/plib_dacc.h"
#include "etl/expected.h"

class LCLDACC : public LCL {
public:
    /**
     * The value for volts to write in the DACC_Channel
     * The final value is calculated using the following formula: V = Vref * ( value / resolution )
     * where: Vref=3.3V and resolution = 12 bit = 4096
     * CAN: 0.925V
     * NAND: 0.726V
     */
    enum DACThreshold : uint16_t {
        CAN = 1148,
        NAND = 901,
        DACDisableValue = 0
    };

    /**
     * Constructor to set the necessary control pins for the LCL.
     * @param dacChannel @see dacChannel
     * @param resetPin @see resetPin
     * @param setPin @see setPin
     * @param dacVolts @see DACVolts
     */
    LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin, uint16_t dacVolts);

    /**
     * Enable the LCL to monitor and protect the protected IC from over current.
     * Sequence to enable the LCL is as follows:
     * - Write to DACC_channel the desired voltage using the following formula: V = Vref * ( value / resolution )
     *   - where :
     *       Vref = 3.3V (exact measurement is required)
     *       resolution = 12 bit == 4096
     * - Pull the Reset Pin High to allow SR Latch state of the TLC555 to be driven by the internal comparators
     *   instead of being forced Low.
     * - Pull the Set Pin Low to force the SR Latch state of the TLC555 to logic High.
     * - After a brief delay that is necessary for the TLC555 to detect the pulse, pull the Set Pin High making the SR
     *   Latch retain its state. If it was High, the IC is powered on, if a current surge is detected, the SR Latch
     *   will be driven Low, cutting the power supply towards the protected IC and retain its state after the surge.
     * @note If a surge is detected and the power is cut, the MCU will have to call the @fn enableLCL to provide the IC
     *       with power again.
     * @return LCLError on failure
     */
    [[nodiscard]] etl::expected<void, LCLError> enableLCL() override;

    /**
     * Disable the LCL, cutting the supply voltage to the IC. To achieve this, the Reset Pin is driven Low to force the
     * SR Latch state to Low, cutting the power towards the IC and as an extra step, the DAC output is zeroed, setting
     * the current threshold to a small value, typically much smaller than the consumption of the protected IC.
     * @return LCLError on failure
     */
    [[nodiscard]] etl::expected<void, LCLError> disableLCL() override;

    /**
     * Changes the current threshold of the LCL by updating the DACC voltage output.
     * @param voltage The new threshold voltage value
     * @return LCLError on DACC timeout
     */
    etl::expected<void, LCLError> changeCurrentThreshold(uint16_t voltage);

    /**
     * This is a helper function that checks if the DACC data have been written successfully to the DACC channel.
     * The timeout ensures that the code will not get stuck in the loop.
     * @param voltage @see DACVolts
     * @return LCLError on DACC timeout
     */
   [[nodiscard]] etl::expected<void, LCLError> writeDACCDataWithTimeout(uint16_t voltage);

    /**
     * Calculates the analogue voltage produced for a given raw DAC value.
     * Formula: V = Vref * (dacValue / DACResolution)
     * @param dacValue The raw 12-bit DAC value
     * @return The corresponding voltage threshold in volts
     */
    [[nodiscard]] static float calculateVoltageThreshold(uint16_t dacValue) {
        return VRef * (static_cast<float>(dacValue) / static_cast<float>(DACResolution));
    }

    /**
     * Calculates the overcurrent trip level for a given DAC raw value and sense resistance.
     * @param dacValue The raw 12-bit DAC value
     * @param senseResistance The current-sense resistance in ohms
     * @return The trip current threshold in amperes
     */
    [[nodiscard]] static float calculateCurrentThreshold(uint16_t dacValue, float senseResistance) {
        return calculateVoltageThreshold(dacValue) / senseResistance;
    }
    
private:
    /**
     * The DACC channel used for setting the voltage of the LCL.
     */
    const DACC_CHANNEL_NUM dacChannel;

    /**
     * A variable to store the voltage setting (CAN or NAND)
     */
    uint16_t voltageSetting;

    /**
     * This variable is used to store the maximum time the task should wait for the DACC_CHANNEL to be ready to in ticks.
     * The number 1000ms was considered a good value arbitrarily
     */
    static constexpr TickType_t maxDelay = pdMS_TO_TICKS(1000);

    /**
     * A small delay to make sure signals have reached the pins before sending another signal to them
     */
    static constexpr TickType_t smallDelay = pdMS_TO_TICKS(10);

    /**
     * DAC reference voltage in volts (VDDANA / AVCC = 3.3 V).
     */
    static constexpr float VRef = 3.3f;

    /**
     * DAC resolution: 2^12 = 4096 steps for a 12-bit converter.
     */
    static constexpr uint16_t DACResolution = 4096U;
};
