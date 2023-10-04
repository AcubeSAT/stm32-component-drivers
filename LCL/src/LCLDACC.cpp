#include "LCL.hpp"
#include "LCLDACC.hpp"

LCLDACC::LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin):LCL(resetPin, setPin), dacChannel(dacChannel) {
    disableLCL();
}

void LCLDACC::enableLCL() {
    if (DACC_IsReady(dacChannel))
    {
        DACC_DataWrite (dacChannel, 0xff);//value
    }
    else{
        //error handling code here
    }
    PIO_PinWrite(setPin, false);

    vTaskDelay(pdMS_TO_TICKS(10));

    PIO_PinWrite(setPin, true);
}

void LCLDACC::disableLCL() {
    // Disable the DACC channel (dacChannel)
    if (DACC_IsReady(dacChannel))
    {
        DACC_DataWrite (dacChannel, 0);
    }
    else{
        //error handling code here
    }

    // Drive resetPin and setPin as needed for DAC operation
    PIO_PinWrite(resetPin, false);
    PIO_PinWrite(setPin, true);
}