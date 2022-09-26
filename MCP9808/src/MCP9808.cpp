#include "MCP9808.hpp"

void MCP9808::writeRegister(uint8_t address, uint16_t data) {
    uint8_t txData[] = {
            address,
            static_cast<uint8_t>(data >> 8),
            static_cast<uint8_t>(data & 0x00FF)
    };

    uint8_t ackData = 0;

    if (TWIHS2_Write(I2C_BUS_ADDRESS, &ackData, 1)) {
        waitForResponse();
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Write(I2C_BUS_ADDRESS, txData, 1)) {
        waitForResponse();
        error = TWIHS2_ErrorGet();
    }
}

uint16_t MCP9808::readRegister(uint8_t address) {
    uint8_t buffer[2];
    uint8_t ackData = 0;

    if (TWIHS2_Write(I2C_BUS_ADDRESS, &ackData, 1)) {
        waitForResponse();
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Write(I2C_BUS_ADDRESS, &address, 1)) {
        waitForResponse();
        error = TWIHS2_ErrorGet();
    }

    if (TWIHS2_Read(I2C_BUS_ADDRESS, buffer, 2)) {
        waitForResponse();
        error = TWIHS2_ErrorGet();
        return ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    } else {
        return 0;
    }
}

void MCP9808::setRegister(uint8_t address, Mask mask, uint16_t setting) {
    uint16_t previous = readRegister(address);

    uint16_t newSetting = (mask & previous) | setting;
    writeRegister(address, newSetting);
}

void MCP9808::setHysteresisTemperature(MCP9808::HysteresisTemperatureOptions option) {
    setRegister(REG_CONFIG, THYST_MASK, option);
}

void MCP9808::setLowPowerMode(MCP9808::LowPowerMode setting) {
    setRegister(REG_CONFIG, SHDN_MASK, setting);
}

void MCP9808::setCriticalTemperatureLock(MCP9808::CriticalTemperatureRegisterLock setting) {
    setRegister(REG_CONFIG, TCRIT_LOCK_MASK, setting);
}

void MCP9808::setTemperatureWindowLock(MCP9808::TemperatureWindowLock setting) {
    setRegister(REG_CONFIG, WINLOCK_MASK, setting);
}

void MCP9808::clearInterrupts() {
    setRegister(REG_CONFIG, IRQ_CLEAR_MASK, IRQ_CLEAR);
}

void MCP9808::setAlertStatus(MCP9808::AlertStatus setting) {
    setRegister(REG_CONFIG, ALERT_STATUS_MASK, setting);
}

void MCP9808::setAlertControl(MCP9808::AlertControl setting) {
    setRegister(REG_CONFIG, ALERT_CONTROL_MASK, setting);
}

void MCP9808::setAlertSelection(MCP9808::AlertSelection setting) {
    setRegister(REG_CONFIG, ALERT_SELECT_MASK, setting);
}

void MCP9808::setAlertPolarity(MCP9808::AlertPolarity setting) {
    setRegister(REG_CONFIG, ALERT_POLARITY_MASK, setting);
}

void MCP9808::setAlertMode(MCP9808::AlertMode setting) {
    setRegister(REG_CONFIG, ALERT_MODE_MASK, setting);
}

void MCP9808::setResolution(MCP9808::MeasurementResolution setting) {
    setRegister(REG_RESOLUTION, RES_MASK, setting << 8);
}

float MCP9808::getTemperature() {
    uint16_t data;
    float result = 0;

    data = readRegister(REG_TEMP);

    uint8_t upperByte = (data >> 8) & 0x1F;
    uint8_t lowerByte = data & 0xFF;

    if ((upperByte & 0x10) != 0) {
        upperByte &= 0x0F;
        result = 256 - upperByte * 16.0f + lowerByte / 16.0f;
    } else {
        result = upperByte * 16.0f + lowerByte / 16.0f;
    }

    return result;
}

bool MCP9808::isDeviceConnected() {
    return readRegister(REG_MFGID) == MANUFACTURER_ID;
}
