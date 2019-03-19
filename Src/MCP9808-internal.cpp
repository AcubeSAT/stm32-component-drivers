#include "MCP9808.hpp"

void MCP9808::writeReg(uint8_t addr, uint16_t &data) {
    uint8_t txData[2] = {addr, data};
    HAL_I2C_Master_Transmit(i2c, MCP9808_I2C_USER_ADDR, txData, 2, 100);
}

void MCP9808::readReg(uint8_t addr, uint16_t &result) {
    uint8_t buffer[2];
    HAL_I2C_Master_Transmit(i2c, MCP9808_I2C_USER_ADDR, &addr, 1, 100);
    HAL_I2C_Master_Receive(i2c, MCP9808_I2C_USER_ADDR, buffer, 2, 100);

    result = ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
}

MCP9808::MCP9808(I2C_HandleTypeDef* i2c) {
    this->i2c = i2c;
}

