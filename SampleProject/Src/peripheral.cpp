#include "peripheral.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include <cstdio>
#include <cstring>
#include "MCP9808.hpp"

extern "C" {
    void peripheral_run(I2C_HandleTypeDef * i2c) {
        uint8_t string[50] = "Welcome to peripheral run\r\n";
        HAL_UART_Transmit(&huart2,string,strlen(reinterpret_cast<char*>(string)),1000);

    }
}