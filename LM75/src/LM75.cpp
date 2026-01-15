#include "LM75.h"
#include <Logger.hpp>

LM75Sensor::LM75Sensor() {
};

I2C1_Initialize();
void LM75Sensor::updateTemp() {
    if (!I2Cx_Read(LM45_ADDR, &tempRead, 1)){
        this->lastError = Error::HARDWARE_FAULT;
    }
    if (!I2Cx_Read(LM45_ADDR, &tempRead, 2)){
        this->lastError = Error::NO_DATA_AVAILABLE;
    }
}


void LM75Sensor::logTemp() {
    LOG_INFO << "The temperature is" << temp << "degC";
}

