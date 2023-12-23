#include "LCLDACC.hpp"
#include "Logger.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin, DACVolts dacVolts) : LCL(resetPin,
                                                                                                         setPin),
                                                                                                     dacChannel(
                                                                                                             dacChannel),
                                                                                                     voltageSetting(
                                                                                                             static_cast<uint16_t>(dacVolts)) {
}

etl::expected<bool, bool> LCLDACC::writeDACCDataWithTimeout(uint16_t voltage) {
    DACC_DataWrite(dacChannel, voltage);
    const TickType_t startTime = xTaskGetTickCount();
    bool dacTimedOut = false;

    while (!DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_ERROR << " LCL DAC timed out.";
            DACC_Initialize();
            dacTimedOut = true;
            return etl::unexpected(dacTimedOut);
        }
    }
    return dacTimedOut;
}

void LCLDACC::enableLCL() {
    if (!writeDACCDataWithTimeout(voltageSetting)) {
        PIO_PinWrite(resetPin, true);
        PIO_PinWrite(setPin, false);
        vTaskDelay(pdMS_TO_TICKS(smallDelay));
        PIO_PinWrite(setPin, true);
    }
}

void LCLDACC::disableLCL() {
    if (!writeDACCDataWithTimeout(DACDisableValue)) {
        PIO_PinWrite(resetPin, false);
        PIO_PinWrite(setPin, true);
    }
}
