#include "AD590_Driver.hpp"

void AD590::getTemperature() {

    AFEC0_ConversionStart();
    vTaskDelay(pdMS_TO_TICKS(1));

//        while(!AFEC0_ChannelResultIsReady(AFEC_CH0)){}

            uint16_t ADCconversion = AFEC0_ChannelResultGet(AFEC_CH0);
            float MCUtemperature = convertADCValueToTemperature(ADCconversion);

            etl::string<64> log = "";
            log += "The temperature of the MCU is: ";
            if (MCUtemperature < 0) {
                log += " -";
                etl::to_string(abs(MCUtemperature), log, etl::format_spec().precision(9), true);
            } else {
                etl::to_string(MCUtemperature, log, etl::format_spec().precision(9), true);
            }
            LOG_DEBUG << log.data() << " degrees Celsius";
            vTaskDelay(pdMS_TO_TICKS(1000));

}

float AD590::convertADCValueToTemperature(uint16_t ADCconversion){
    float voltageConversion = static_cast<float>(ADCconversion)/4096*3300;
    float currentConversion = voltageConversion/7.870;
    return currentConversion - offsetCurrent + referenceTemperature ;
}
