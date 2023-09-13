#pragma once

#include <cstdint>
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"
#include "Peripheral_Definitions.hpp"
#include "peripheral/afec/plib_afec0.h"

/**
 * AD590 temperature sensor driver
 *
 * This is a simple driver to use the AD590 sensor on ATSAMV71Q21B microcontrollers.
 *
 * For more details about the operation of the sensor, see the datasheets found at:
 * https://www.mouser.com/catalog/specsheets/intersil_fn3171.pdf
 * and https://www.analog.com/media/en/technical-documentation/data-sheets/ad590.pdf
 *
 * ------BREAKOUT PINOUT---------
 *
 * GND to GND
 * 5V to 5V
 *
 * Without op-amp :
 * TEMP-RAW to PD30 (EXT2 on the ATSAMV71)
 *
 * With op-amp :
 * TEMP-OPAMP to PD30 (EXT2 on the ATSAMV71)
 * OPAMP-VCC to 3V3
 *
 *
 */

#if AD590_AFEC_Peripheral == 0

#include "peripheral/afec/plib_afec0.h"
#define AD590_AFEC_ChannelResultGet AFEC0_ChannelResultGet
#define AD590_AFEC_ConversionStart AFEC0_ConversionStart
#define AD590_AFEC_ChannelResultIsReady AFEC0_ChannelResultIsReady


#elif AD590_AFEC_Peripheral == 1

#include "peripheral/afec/plib_afec1.h"
#define AD590_AFEC_ChannelResultGet AFEC1_ChannelResultGet
#define AD590_AFEC_ConversionStart AFEC1_ConversionStart
#define AD590_AFEC_ChannelResultIsReady AFEC1_ChannelResultIsReady

#endif


class AD590 {
private:

    /**
     * Nominal Current Output at 25°C (298.2 K)
     */
    inline static constexpr float offsetCurrent = 298.2;

    /**
     * Reference temperature constant in Celsius
     */
    inline static constexpr float referenceTemperature = 25;

    /**
     * Value of the resistor that maps the current to 3.3 voltage and the difference of sampling.
     */
    static float resistorValue ;

public:

    AD590(){
        resistorValue = 7.870;
    }

    /**
     * Converts the voltage to current and finally to temperature in Celsius.
     * @param ADCconversion The ADC conversion result
     * @return The temperature in Celsius
     */
    float convertADCValueToTemperature(uint16_t ADCconversion);

    /**
     * Gets the analog temperature from the AD590 temperature sensor, converts it to digital and prints it.
     */
    void getTemperature(uint16_t adc_ch0);
};

