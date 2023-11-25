#include "LCL.hpp"
#include "LCLDACC.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin) : LCL(resetPin, setPin),
                                                                                  dacChannel(dacChannel) {
    disableLCL();
}

void LCLDACC::enableLCL() {
    DACC_DataWrite(dacChannel, 2048); // Set initial arbitrary data value (1.618 V)

    while (!DACC_IsReady(dacChannel)) {
        // Wait until DACC is ready
    }
    DACC_DataWrite(dacChannel, 2048); // Set data value again after ensuring DACC is ready
    PIO_PinWrite(resetPin, true);
    PIO_PinWrite(setPin, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    PIO_PinWrite(setPin, true);
}

void LCLDACC::disableLCL() {
    // Disable the DACC channel (dacChannel)
    DACC_DataWrite(dacChannel, 0x00);
    //can't use DACC_IsReady(dacChannel) because this runs before the DACC_Initialization in main.
    // Drive resetPin and setPin as needed for DAC operation
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}
