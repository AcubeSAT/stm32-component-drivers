#pragma once

#include "HAL_AFEC.hpp"

/**
 * AFECGeneral driver
 *
 * This is a driver to use the AFEC peripherals.
 *
 * @warning if we want to use any of the  channels we need to first enable them from Harmony Configuration
 */
template<AFECPeripheral AfecPeripheral>
class AFECGeneral {
public:
    /**
     * Constructor for the AFECGeneral class.
     *
     * @tparam afecPeripheral The AFEC Peripheral that is being used (either AFEC0 or AFEC1).
     * @param adcChannelNum Number of the AFEC channel that is being used.
     */
    explicit AFECGeneral(AFEC_CHANNEL_NUM afecChannelNum) : afecChannelNum(afecChannelNum) {}

    /**
     *  Sets the ADC Result of the Afec channel of the AFECGerneral instance
     */
    void setAdcResult(uint16_t adcResult);

    /**
     * @return the ADC Result of the Afec channel of the AFECGerneral instance
     */
    uint16_t getAdcResult();

    /**
     * @return The channel Number of the AFECGerneral instance
     */
    AFEC_CHANNEL_NUM getADCChannelNum();

protected:
    /**
     *  The result of the ADC conversion of the AFECGerneral instance
     */
    uint16_t adcResult;

    /**
     * Number of the AFEC peripheral channel being used.
     */
    const AFEC_CHANNEL_NUM afecChannelNum;
};