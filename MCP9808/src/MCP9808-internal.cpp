
#include "../inc/MCP9808.hpp"

void MCP9808::writeReg(uint8_t addr, uint16_t &data) {
    uint8_t txData[3];  // data to be transmitted
    txData[0] = addr;   // register address
    txData[1] = data >> 8; // high 8-bits
    txData[2] = data & 0x00FF; // low 8-bits

    TWIHS2_Write(MCP9808_I2C_BUS_ADDR, txData, 3);
}

void MCP9808::readReg(uint8_t addr, uint8_t *buffer) {
    if (TWIHS2_Write(MCP9808_I2C_BUS_ADDR, &addr, 1)) {
        while (TWIHS2_IsBusy());
        TWIHS2_Write(MCP9808_I2C_BUS_ADDR, &addr, 1);
        while (TWIHS2_IsBusy());
    }
    TWIHS2_Read(MCP9808_I2C_BUS_ADDR, buffer, 2);
    while (TWIHS2_IsBusy());
}

void MCP9808::setReg(uint8_t addr, uint16_t mask, uint16_t setting) {
    uint8_t buffer[2];
    readReg(addr, buffer); // store the previous value of the register
    uint16_t previous = ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    uint16_t newSetting = (mask & previous) | setting;
    writeReg(addr, newSetting); // write the new one after changing the desired setting
}


