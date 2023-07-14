#include "AD590_Driver.hpp"

void AD590::getTemperature() {
    while (true) {
        AFEC0_ConversionStart();
        vTaskDelay(pdMS_TO_TICKS(1));
        if(AFEC0_ChannelResultIsReady(AFEC_CH0)){
            uint16_t ADCconversion = AFEC0_ChannelResultGet(AFEC_CH0);
//            float voltageConversion = static_cast<float>(ADCconversion) * PositiveVoltageReference / MaxADCValue;
            float MCUtemperature = convertADCValueToTemperature(ADCconversion);
//        const float MCUtemperature = (voltageConversion - TypicalVoltageAt25) / TemperatureSensitivity + ReferenceTemperature;
            LOG_DEBUG << "The temperature of the MCU is: " << MCUtemperature << " degrees Celsius";
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

float AD590::convertADCValueToTemperature(uint16_t ADCconversion){
    float voltageConversion = static_cast<float>(ADCconversion)/4096*3.3;
    float currentConversion = voltageConversion/7870;
    return currentConversion - offsetCurrent /* + referenceTemperatureAt25 */ ;
}