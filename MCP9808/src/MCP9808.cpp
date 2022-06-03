#include "../inc/MCP9808.hpp"


void MCP9808::setHystTemp(uint16_t temp) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_THYST_MASK, temp);
}

void MCP9808::setLowPwrMode(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_SHDN_MASK, setting);
}

void MCP9808::setCritTempLock(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_TCRIT_LOCK_MASK, setting);
}

void MCP9808::setTempWinLock(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_WINLOCK_MASK, setting);
}

void MCP9808::clearInterrupts() {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_IRQ_CLEAR_MASK, MCP9808_CONFIG_IRQ_CLEAR);
}

void MCP9808::setAlertStatus(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_STATUS_MASK, setting);
}

void MCP9808::setAlertControl(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_CONTROL_MASK, setting);
}

void MCP9808::setAlertSelection(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_SELECT_MASK, setting);

}

void MCP9808::setAlertPolarity(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_POLARITY_MASK, setting);

}

void MCP9808::setAlertMode(uint16_t setting) {
    setReg(MCP9808_REG_CONFIG, MCP9808_CONFIG_ALERT_MODE_MASK, setting);
}

void MCP9808::setResolution(uint16_t setting) {
    setReg(MCP9808_REG_RESOLUTION, MCP9808_RES_MASK, setting << 8);
    // since the bits we're interested in are located in the less significant byte, and
    // the I2C protocol reads MSB-first, while the register fits only 8 bits,
    // we shift by 8 to bring them to the MSB and thus store them.

}

void MCP9808::getTemp(float &result) {
    uint8_t buffer[2];
    readReg(MCP9808_REG_TEMP, buffer);

    buffer[0] &= 0X1F;
    if ((buffer[0] & 0x10) == 0x10) {
        buffer[0] &= 0x0F;
        result = 256 - (16 * buffer[0] + buffer[1] / 16);
    } else {
        result = ((16 * static_cast<float>(buffer[0]) + static_cast<float>(buffer[1]) / 16));
    }
}
