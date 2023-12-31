#include "LCLDACC.hpp"
#include "Logger.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin,
                 DACThreshold voltageSetting) : LCL(resetPin, setPin), dacChannel(dacChannel),
                                                voltageSetting(static_cast<std::underlying_type_t<DACThreshold>>(voltageSetting)) {
}

etl::expected<bool, bool> LCLDACC::writeDACCDataWithTimeout(uint16_t voltage) {
    DACC_DataWrite(dacChannel, voltage);
    const TickType_t startTime = xTaskGetTickCount();
    bool dacTimedOut = false;

    while (not DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_ERROR << "LCL DAC channel " << static_cast<int>(dacChannel) << " timed out.";
            DACC_Initialize();
            dacTimedOut = true;
            return etl::unexpected(dacTimedOut);
        }
    }
    return dacTimedOut;
}

void LCLDACC::enableLCL() {
    if (not writeDACCDataWithTimeout(voltageSetting)) {
        PIO_PinWrite(resetPin, true);
        PIO_PinWrite(setPin, false);
        vTaskDelay(pdMS_TO_TICKS(smallDelay));
        PIO_PinWrite(setPin, true);
    }
}

void LCLDACC::disableLCL() {
    if (not writeDACCDataWithTimeout(DACDisableValue)) {
        PIO_PinWrite(resetPin, false);
        PIO_PinWrite(setPin, true);
    }
}
