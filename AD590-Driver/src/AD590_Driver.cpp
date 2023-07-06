#include "AD590_Driver.hpp"

float AD590::getTemperature() {
    while (true) {
        AFEC0_ConversionStart();
        vTaskDelay(pdMS_TO_TICKS(1));
        if(AFEC0_ChannelResultIsReady(AFEC_CH0)){
            uint16_t ADCconversion = AFEC0_ChannelResultGet(AFEC_CH0);
//            float voltageConversion = static_cast<float>(ADCconversion) * PositiveVoltageReference / MaxADCValue;
            float voltageConversion = static_cast<float>(ADCconversion)/4096*3.3;
            float currentConversion = voltageConversion/7870;
            float MCUtemperature = currentConversion - offsetCurrent /* + referenceTemperatureAt25 */ ;
//        const float MCUtemperature = (voltageConversion - TypicalVoltageAt25) / TemperatureSensitivity + ReferenceTemperature;
            LOG_DEBUG << "The temperature of the MCU is: " << MCUtemperature << " degrees Celsius";
            CommonParameters::mcuTemperature.setValue(MCUtemperature);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
