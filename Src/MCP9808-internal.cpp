#include "MCP9808.hpp"

void MCP9808::writeReg(uint8_t addr, uint16_t &data) {
    uint8_t txData[3];  // data to be transmitted
    txData[0] = addr;   // register address
    txData[1] = data >> 8; // high 8-bits
    txData[2] = data & 0x00FF; // low 8-bits

    HAL_I2C_Master_Transmit(i2c, MCP9808_I2C_USER_ADDR, txData, 3, 100);
}

void MCP9808::readReg(uint8_t addr, uint16_t &result) {
    uint8_t buffer[2]; // a place to hold the incoming data

    HAL_I2C_Master_Transmit(i2c, MCP9808_I2C_USER_ADDR, &addr, 1, 100); // transmit the target register's address
    HAL_I2C_Master_Receive(i2c, MCP9808_I2C_USER_ADDR, buffer, 2, 100); // read the register's contents

    // combine the 2 octets received (MSB, LSB) into one 16-bit uint)
    result = ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
}

MCP9808::MCP9808(I2C_HandleTypeDef* i2c) {
    this->i2c = i2c;
}
