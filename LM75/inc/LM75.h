#pragma once
#include <cstdint>
#include <etl/span.h>

class LM75Sensor{
    etl::array<uint8_t, 4> buf;
    constexpr static uint8_t LM45_ADDR = 0x48 << 1;
    constexpr static uint8_t  LM45_REG = 0x00;
    uint16_t tempRead;
    float temp;
    Error lastError;

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



