#include "LCL.hpp"
#include "LCLDACC.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin):LCL(resetPin, setPin), dacChannel(dacChannel) {
    disableLCL();
}

void LCLDACC::enableLCL() {
    DACC_DataWrite (dacChannel, 0xff);//value
    if (DACC_IsReady(dacChannel))
    {
        DACC_DataWrite (dacChannel, 0xff);//value
    }
    else{
        //error handling code here
        DACC_Initialize();
    }
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

void LCLDACC::disableLCL() {
    // Disable the DACC channel (dacChannel)
    DACC_DataWrite (dacChannel, 0);
    if (DACC_IsReady(dacChannel))
    {
        DACC_DataWrite (dacChannel, 0);
    }
    else{
        DACC_Initialize();
    }

    // Drive resetPin and setPin as needed for DAC operation
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}

