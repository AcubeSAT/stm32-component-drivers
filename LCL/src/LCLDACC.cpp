#include "LCLDACC.hpp"
#include "Logger.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin) : LCL(resetPin, setPin),
                                                                                  dacChannel(dacChannel) {
}

void LCLDACC::enableLCL() {
    DACC_DataWrite(dacChannel, volts);
    const TickType_t startTime = xTaskGetTickCount();

    while (!DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_DEBUG << " DACC_IsReady timed out.";
            DACC_Initialize();
            break;
        }
    }
    PIO_PinWrite(resetPin, true);
    PIO_PinWrite(setPin, false);
    vTaskDelay(pdMS_TO_TICKS(smallDelay));
    PIO_PinWrite(setPin, true);
}

void LCLDACC::disableLCL() {
    TickType_t startTime = xTaskGetTickCount();
    DACC_DataWrite(dacChannel, zeroVolts);

    while (!DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_DEBUG << " DACC_IsReady timed out.";
            DACC_Initialize();
            break;
        }
    }
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}
