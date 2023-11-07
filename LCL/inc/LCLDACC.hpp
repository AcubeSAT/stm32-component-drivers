#pragma once
#include "LCL.hpp"
#define DAC_COUNT_INCREMENT     (124U)  // Equivalent to 0.1V
#define DAC_COUNT_MAX           (4095U)

class LCLDACC:public LCL{
private:
    const DACC_CHANNEL_NUM dacChannel = DACC_CHANNEL_0;
    uint16_t dac_count=0x800;


public:
    LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin);
    void enableLCL() override;
    void disableLCL() override;
    void increment();
};
