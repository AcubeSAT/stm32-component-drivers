#ifndef ATSAM_COMPONENT_DRIVERS_LCL_HPP
#define ATSAM_COMPONENT_DRIVERS_LCL_HPP

/**
 * @class A Latchup Current Limiter driver providing all the functionality for these protection
 * circuits.
 */

class LCL {
private:

public:
    void openLCL();
    void shutDownLCL();
    void calculateVoltageThreshold();
    void calculateCurrentUpperBound();
};

#endif //ATSAM_COMPONENT_DRIVERS_LCL_HPP
