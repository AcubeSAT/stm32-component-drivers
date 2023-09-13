#include "AD590.hpp"

void AD590::getTemperature(uint16_t adc_ch0) {

        float MCUtemperature = convertADCValueToTemperature(adc_ch0);

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
    float currentConversion = voltageConversion/resistorValue;
    return currentConversion - offsetCurrent + referenceTemperature ;
}
