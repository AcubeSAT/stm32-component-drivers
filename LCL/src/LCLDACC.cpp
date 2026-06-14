#include "LCLDACC.hpp"
#include "Logger.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin, uint16_t voltageSetting) : 
                 LCL(resetPin, setPin), dacChannel(dacChannel), voltageSetting(voltageSetting) {
}

etl::expected<void, LCLError> LCLDACC::writeDACCDataWithTimeout(uint16_t voltage) {
    DACC_DataWrite(dacChannel, voltage);
    const TickType_t startTime = xTaskGetTickCount();

    while (not DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
        const TickType_t currentTime = xTaskGetTickCount();
        if ((currentTime - startTime) >= maxDelay) {
            LOG_ERROR << "LCL DAC channel " << static_cast<int>(dacChannel) << " timed out.";
            DACC_Initialize();
            return etl::unexpected<LCLError>(LCLError::TIMEOUT);
        }
    }
    return {};
}

etl::expected<void, LCLError> LCLDACC::enableLCL() {
    auto status = writeDACCDataWithTimeout(voltageSetting);
    if (status) {
        PIO_PinWrite(resetPin, true);
        PIO_PinWrite(setPin, false);
        vTaskDelay(pdMS_TO_TICKS(smallDelay));
        PIO_PinWrite(setPin, true);
        lclStatus = true;
        return {};
    } else {
        LOG_ERROR << "Failed to enable LCL due to DACC timeout";
        return etl::unexpected<LCLError>(status.error());
    }
}

etl::expected<void, LCLError> LCLDACC::disableLCL() {
    auto status = writeDACCDataWithTimeout(DACDisableValue);
    if (status) {
        PIO_PinWrite(resetPin, false);
        PIO_PinWrite(setPin, true);
        lclStatus = false;
        return {};
    } else {
        LOG_ERROR << "Failed to disable LCL due to DACC timeout";
        return etl::unexpected<LCLError>(status.error());
    }
}

etl::expected<void, LCLError> LCLDACC::changeCurrentThreshold(uint16_t voltage) {
    auto status = writeDACCDataWithTimeout(voltage);
    if (status) {
        voltageSetting = voltage;
        return {};
    } else {
        LOG_ERROR << "Failed to change current threshold due to DACC timeout";
        return etl::unexpected<LCLError>(status.error());
    }
}