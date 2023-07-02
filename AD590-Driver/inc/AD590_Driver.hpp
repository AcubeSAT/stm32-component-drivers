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
public:

    /**
     * Gets the analog temperature from the AD590 temperature sensor and converts it to digital.
     * @return The temperature as a float
     */
    float getTemperature();
};

