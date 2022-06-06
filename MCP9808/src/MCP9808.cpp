#include "MCP9808.hpp"


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
}

void MCP9808::getTemp(float &result) {
    uint16_t data;

    readReg(MCP9808_REG_TEMP, data);

    uint8_t upperByte = (data >> 8) & 0x1F;
    uint8_t lowerByte = (data & 0xFF);
    if ((upperByte & 0x10) == 0x10) {
        upperByte &= 0x0F;
        result = 256 - ((16 * static_cast<float>(upperByte) + static_cast<float>(lowerByte) / 16));
    } else {
        result = ((16 * static_cast<float>(upperByte) + static_cast<float>(lowerByte) / 16));
    }
}
