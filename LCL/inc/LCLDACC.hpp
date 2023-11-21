#pragma once
#include "LCL.hpp"


class LCLDACC:public LCL{
private:
    const DACC_CHANNEL_NUM dacChannel = DACC_CHANNEL_0;

public:
    LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin);
    void enableLCL() override;
    void disableLCL() override;
};
