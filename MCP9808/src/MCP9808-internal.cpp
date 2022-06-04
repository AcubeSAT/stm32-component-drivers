#include "MCP9808.hpp"

void MCP9808::writeReg(uint8_t addr, uint16_t &data) {
    uint8_t txData[3];
    txData[0] = addr;
    txData[1] = data >> 8;
    txData[2] = data & 0x00FF;
    uint8_t ackData = 0;
    TWIHS_ERROR error;

    if (TWIHS2_Write(MCP9808_I2C_BUS_ADDR, &ackData, 1)) {
        while (TWIHS2_IsBusy());
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Write(MCP9808_I2C_BUS_ADDR, txData, 1)) {
        while (TWIHS2_IsBusy());
        error = TWIHS2_ErrorGet();
    }
}

void MCP9808::readReg(uint8_t addr, uint16_t &result) {
    uint8_t buffer[2];
    uint8_t ackData = 0;
    TWIHS_ERROR error;

    if (TWIHS2_Write(MCP9808_I2C_BUS_ADDR, &ackData, 1)) {
        while (TWIHS2_IsBusy());
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Write(MCP9808_I2C_BUS_ADDR, &addr, 1)) {
        while (TWIHS2_IsBusy());
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Read(MCP9808_I2C_BUS_ADDR, buffer, 2)) {
        while (TWIHS2_IsBusy());
        error = TWIHS2_ErrorGet();
        result = ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    }
}

void MCP9808::setReg(uint8_t addr, uint16_t mask, uint16_t setting) {
    uint16_t previous = 0;
    readReg(addr, previous);

    uint16_t newSetting = (mask & previous) | setting;
    writeReg(addr, newSetting);
}


