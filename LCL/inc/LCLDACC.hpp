#pragma once
#include "LCL.hpp"


class LCLDACC:public LCL{
private:
    /**
   * The DACC channel used for setting the voltage of the LCL.
   */
    const DACC_CHANNEL_NUM dacChannel = DACC_CHANNEL_0;

public:
    LCLDACC(DACC_CHANNEL_NUM dacChannel, PIO_PIN resetPin, PIO_PIN setPin);
    /**
    * Enable to LCL to monitor and protect the protected IC from over current.
    * Sequence to enable the LCL is as follows:
            * - Wirte to DACC_channel the desired voltage using the following formula: V = Vref * ( value / resolution )
            * - where :
            *           Vref = 3.3V (exact measurement is required)
            *           resolution = 12 bit == 4096
    * - Pull the Reset Pin High to allow SR Latch state of the TLC555 to be driven but the internal comparators
    * instead of being forced Low.
    * - Pull the Set Pin Low to force the SR Latch state of the TLC555 to logic High.
    * - After a brief delay that is necessary for the TLC555 to detect the pulse, pull the Set Pin High making the SR
            * Latch retain its state. If the it was High, the IC is powered on, if a current surge is detected, the SR Latch
            * will be driven Low, cutting the power supply towards the protected IC and retain its state after the surge.
    * @note If a surge is detected and the power is cut, the MCU will have to call the @fn enableLCL to provide the IC
    * with power again.
    */
    void enableLCL() override;

    /**
    * Disable the LCL, cutting the supply voltage to the IC. To achieve this, the Reset Pin is driven Low to force the
    * SR Latch state to Low, cutting the power towards the IC and as an extra step, the PWM signal is closed, setting
    * the current threshold to a small value, typically much smaller than the consumption of the protected IC.
    */
    void disableLCL() override;
};
