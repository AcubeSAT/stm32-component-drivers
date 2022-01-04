#include "I2CDevice.h"

I2CDevice::I2CDevice(I2C_HandleTypeDef* i2c) {
    this->i2c = i2c;
}

template<>
void I2CDevice::writeReg(uint8_t addr, uint8_t data) {
    uint8_t txData[3];  // data to be transmitted
    txData[0] = addr; // register address
    txData[1] = data; //the data

    HAL_I2C_Master_Transmit(i2c, deviceAddress, txData, 2, 100);
}

template<>
void I2CDevice::writeReg(uint8_t addr, uint16_t data) {
    uint8_t txData[3];  // data to be transmitted
    txData[0] = addr;   // register address
    txData[1] = data >> 8; // high 8-bits
    txData[2] = data & 0x00FF; // low 8-bits

    HAL_I2C_Master_Transmit(i2c, deviceAddress, txData, 3, 100);
}

template<>
uint8_t I2CDevice::readReg(uint8_t addr) {
    uint8_t buffer; // a place to hold the incoming data

    HAL_I2C_Master_Transmit(i2c, deviceAddress, &addr, 1, 100); // transmit the target register's address
    HAL_I2C_Master_Receive(i2c, deviceAddress, &buffer, 1, 100); // read the register's contents

    return buffer;
}

template<>
uint16_t I2CDevice::readReg(uint8_t addr) {
    uint8_t buffer[2]; // a place to hold the incoming data

    HAL_I2C_Master_Transmit(i2c, deviceAddress, &addr, 1, 100); // transmit the target register's address
    HAL_I2C_Master_Receive(i2c, deviceAddress, buffer, 2, 100); // read the register's contents

    // combine the 2 octets received (MSB, LSB) into one 16-bit uint
    return (static_cast<uint16_t>(buffer[0] << 8u) | static_cast<uint16_t>(buffer[1]));
}

template<>
void I2CDevice::setReg(uint8_t addr, uint8_t mask, uint8_t setting) {
    uint8_t previous = readReg<uint8_t>(addr); // store the previous value of the register
    uint8_t newSetting = (mask & previous) | setting;
    writeReg(addr, newSetting); // write the new one after changing the desired setting
}

template<>
void I2CDevice::setReg(uint8_t addr, uint16_t mask, uint16_t setting) {
    uint16_t previous = readReg<uint16_t>(addr); // store the previous value of the register
    uint16_t newSetting = (mask & previous) | setting;
    writeReg(addr, newSetting); // write the new one after changing the desired setting
}