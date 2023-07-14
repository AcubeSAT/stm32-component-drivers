#pragma once

#include "peripheral/afec/plib_afec0.h"

/**
 * AD590 temperature sensor driver
 *
 * This is a simple driver to use the AD590 sensor on ATSAMV71Q21B microcontrollers.
 *
 * For more details about the operation of the sensor, see the datasheets found at:
 * https://www.mouser.com/catalog/specsheets/intersil_fn3171.pdf
 * and https://www.analog.com/media/en/technical-documentation/data-sheets/ad590.pdf
 */

class AD590 {
private:
    inline constexpr offsetCurrent = 273.2;
    inline constexpr referenceTemperature = 298.2;
public:

    /**
     * Converts the voltage to current and finally to temperature in Celsius.
     * @param ADCconversion The ADC conversion result
     * @return The temperature in Celsius
     */
    float convertADCValueToTemperature(uint16_t ADCconversion);

    /**
     * Gets the analog temperature from the AD590 temperature sensor, converts it to digital and prints it.
     */
    void getTemperature();
};

