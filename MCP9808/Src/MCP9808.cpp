#include <math.h>
#include "MCP9808.hpp"

MCP9808::MCP9808(I2C_HandleTypeDef *i2c) : I2CDevice(i2c) {
    deviceAddress = MCP9808_I2C_BUS_ADDR;
}

void MCP9808::setHystTemp(uint16_t temp) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_THYST_MASK, temp);
}

void MCP9808::setLowPwrMode(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_SHDN_MASK, setting);
}

void MCP9808::setCritTempLock(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_TCRIT_LOCK_MASK, setting);
}

void MCP9808::setTempWinLock(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_WINLOCK_MASK, setting);
}

void MCP9808::clearInterrupts() {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_IRQ_CLEAR_MASK, MCP9808_CONFIG_IRQ_CLEAR);
}

void MCP9808::setAlertStatus(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_STATUS_MASK, setting);
}

void MCP9808::setAlertControl(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_CONTROL_MASK, setting);
}

void MCP9808::setAlertSelection(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_SELECT_MASK, setting);

}

void MCP9808::setAlertPolarity(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_POLARITY_MASK, setting);

}

void MCP9808::setAlertMode(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_MODE_MASK, setting);
}

void MCP9808::setResolution(uint16_t setting) {
    setReg<uint16_t>(MCP9808_REG_RESOLUTION, MCP9808_RES_MASK, setting << 8);
    // since the bits we're interested in are located in the less significant byte, and
    // the I2C protocol reads MSB-first, while the register fits only 8 bits,
    // we shift by 8 to bring them to the MSB and thus store them.

}

float MCP9808::getTemp() {
    uint16_t data = readReg<uint16_t>(MCP9808_REG_TEMP);

    float result = 0;

    // keep bit 12 only (the sign bit) and if it's 1, we start counting from -2^12
    // TODO: find a way to test negative temperatures
    if ((data & 0x100000)) {
       result += 1 / (2 << 12);
    }

    // start summing the other bits
    for (uint8_t i = 0; i < 12; i++) {
        if (data & 0x01) { // we need the LSB only in every iteration
            result += ldexpf(2, i-5);  // add the appropriate power of 2 to the result
        }
        data >>= 1;  // then toss out the bit
    }

    return result;
}
