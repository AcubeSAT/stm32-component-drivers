#ifndef OBC_SOFTWARE_AD590_DRIVER_HPP
#define OBC_SOFTWARE_AD590_DRIVER_HPP

#include "src/config/default/peripheral/afec/plib_afec0.h"

class AD590 {
private:
    /**
     * Gets the analog temperature from the AD590 temperature sensor and converts it to digital.
     * @return The temperature as a float
     */
    float getTemperature();
};
#endif //OBC_SOFTWARE_AD590_DRIVER_HPP
