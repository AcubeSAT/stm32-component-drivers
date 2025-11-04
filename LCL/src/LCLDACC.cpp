#include "LCLDACC.hpp"
#include "Logger.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin,
                 DACThreshold voltageSetting) : LCL(resetPin, setPin), dacChannel(dacChannel),
                                                voltageSetting(static_cast<std::underlying_type_t<DACThreshold>>(voltageSetting)) {
}

bool LCLDACC::writeDACCDataWithTimeout(uint16_t voltage) {
    DACC_DataWrite(dacChannel, voltage);
    const TickType_t startTime = xTaskGetTickCount();

    while (not DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_ERROR << "LCL DAC channel " << static_cast<int>(dacChannel) << " timed out.";
            DACC_Initialize();
            return false;
        }
    }
    return true;
}

bool LCLDACC::enableLCL() {
    if (writeDACCDataWithTimeout(voltageSetting)) {
        PIO_PinWrite(resetPin, true);
        PIO_PinWrite(setPin, false);
        vTaskDelay(pdMS_TO_TICKS(smallDelay));
        PIO_PinWrite(setPin, true);
        return true;
    }
    LOG_ERROR<< "Failed to enable LCL due to DACC timeout";
    return false;

}

bool LCLDACC::disableLCL() {
    if (writeDACCDataWithTimeout(DACDisableValue)) {
        PIO_PinWrite(resetPin, false);
        PIO_PinWrite(setPin, true);
        return true;
    }
    LOG_ERROR << "Failed to disable LCL due to DACC timeout";
    return false;
}

