#include "AD590.hpp"

float AD590::getTemperature() {
        return convertADCValueToTemperature(adcResult);
}

float AD590::convertADCValueToTemperature(uint16_t ADCconversion){
    const float voltageConversion = static_cast<float>(ADCconversion)/numOfBits*voltageValue;
    const float currentConversion = voltageConversion/resistorValue;
    return currentConversion - offsetCurrent + referenceTemperature ;
}
