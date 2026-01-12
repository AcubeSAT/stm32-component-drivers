

#ifndef OBC_SOFTWARE_LM75_H
#define OBC_SOFTWARE_LM75_H
#include <cstdint>

#endif //OBC_SOFTWARE_LM75_H

class LM75Sensor{
    uint8_t data;
    uint16_t temp;
    public:
    LM75Sensor();
    enum class Error {
    /**
     * The sensor could not read any data at this time
     */
    NO_DATA_AVAILABLE,
    /**
     * Sensor catastrophic failure, reset required
     */
    HARDWARE_FAULT,
};
    void updateTemp();
    void logTemp();

};



