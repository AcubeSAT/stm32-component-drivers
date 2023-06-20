
#ifndef OBC_SOFTWARE_AD590_DRIVER_HPP
#define OBC_SOFTWARE_AD590_DRIVER_HPP

#include "src/config/default/peripheral/afec/plib_afec0.h"

class AD590 {
public:
    float getTemperature();
};
#endif //OBC_SOFTWARE_AD590_DRIVER_HPP
