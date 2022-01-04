#ifndef SAMPLEPROJECT_PERIPHERAL_H
#define SAMPLEPROJECT_PERIPHERAL_H

#include "stm32l4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void peripheral_run(I2C_HandleTypeDef *i2c);

#ifdef __cplusplus
}
#endif

#endif //SAMPLEPROJECT_PERIPHERAL_H
