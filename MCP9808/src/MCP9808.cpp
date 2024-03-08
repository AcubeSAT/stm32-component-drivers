#include "MCP9808.hpp"

void MCP9808::writeRegister(uint8_t* data, uint8_t numOfBytes) {

    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, data, numOfBytes)) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
    }
}

uint16_t MCP9808::readRegister(uint8_t address) {
    uint8_t buffer[2];

    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, &address, 1)) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
    }

    if (MCP9808_TWIHS_Read(I2C_BUS_ADDRESS, buffer, 2)) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        return ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    } else {
        return 0;
    }
}

void MCP9808::setRegister(uint8_t address, Mask mask, uint16_t setting) {
    uint16_t previous = readRegister(address);

    uint16_t newSetting = (mask & previous) | setting;

    if (address == REG_RESOLUTION) {  // 1 byte register
        etl::array<uint8_t, MAX_BYTE_NUM> data = {address,
                          static_cast<uint8_t>(newSetting & 0x00FF)};
        writeRegister(data.data(), 2);
    }
    else {  // 2 bytes register
        etl::array<uint8_t, MAX_BYTE_NUM> data = {address,
                          static_cast<uint8_t>((newSetting >> 8) & 0x00FF),
                          static_cast<uint8_t>(newSetting & 0x00FF)};
        writeRegister(data.data(), 3);
    }
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
        result = -(256 - (upperByte * 16.0f + lowerByte / 16.0f));
    } else {
        result = upperByte * 16.0f + lowerByte / 16.0f;
    }

    return result;
}

bool MCP9808::isDeviceConnected() {
    return readRegister(REG_MFGID) == MANUFACTURER_ID;
}

void MCP9808::setUpperTemperatureLimit(float temp) {
    uint16_t data = getData(temp);
    setRegister(REG_TUPPER, TUPPER_TLOWER_TCRIT_MASK, data);
}

void MCP9808::setLowerTemperatureLimit(float temp) {
    uint16_t data = getData(temp);
    setRegister(REG_TLOWER, TUPPER_TLOWER_TCRIT_MASK, data);
}

void MCP9808::setCriticalTemperatureLimit(float temp) {
    uint16_t data = getData(temp);
    setRegister(REG_TCRIT, TUPPER_TLOWER_TCRIT_MASK, data);
}

void MCP9808::enableLowPowerMode() {
    setLowPowerMode(LowPowerMode::LOWPWR_ENABLE);
}

void MCP9808::disableLowPowerMode() {
    setLowPowerMode(LowPowerMode::LOWPWR_DISABLE);
}

void MCP9808::enableCriticalTemperatureLock() {
    setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_ENABLE);
}

void MCP9808::disableCriticalTemperatureLock() {
    setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_DISABLE);
}

void MCP9808::enableTemperatureWindowLock() {
    setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_ENABLE);
}

void MCP9808::disableTemperatureWindowLock() {
    setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_DISABLE);
}

void MCP9808::enableAlertStatus() {
    setAlertStatus(AlertStatus::ALERT_ENABLE);
}

void MCP9808::disableAlertStatus() {
    setAlertStatus(AlertStatus::ALERT_DISABLE);
}

void MCP9808::enableAlertControl() {
    setAlertControl(AlertControl::ALERT_CONTROL_ENABLE);
}

void MCP9808::disableAlertControl() {
    setAlertControl(AlertControl::ALERT_CONTROL_DISABLE);
}

void MCP9808::setAlertSelectionOnCriticalTemperature() {
    setAlertSelection(AlertSelection::ALERT_SELECT_CRITONLY);
}

void MCP9808::setAlertSelectionOnAll() {
    setAlertSelection(AlertSelection::ALERT_SELECT_ALL);
}

void MCP9808::setAlertPolarityActiveHigh() {
    setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_HIGH);
}

void MCP9808::setAlertPolarityActiveLow() {
    setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_LOW);
}

void MCP9808::setAlertModeComparator() {
    setAlertMode(AlertMode::ALERT_MODE_COMPARATOR);
}

void MCP9808::setAlertModeInterrupt() {
    setAlertMode(AlertMode::ALERT_MODE_IRQ);
}

uint16_t MCP9808::getData(float f) {
    float intPart, fractPart;
    uint16_t data, fract;

    if (f > 0.0)
    {
        fractPart = std::modf(f, &intPart);
        data = intPart;
        data = (data << 4) & 0x0FFCu;
        fract = fractPart * 100.0f;
        data = data | ((fract / 50) << 3) & 0x0008u;
        fract %= 50;
        data = data | ((fract / 25) << 2) & 0x0006u;
    }
    else
    {
        fractPart = std::modf(f, &intPart);
        data = -intPart;
        data = (data << 4) & 0x0FFCu;
        data = data | 0x1000u;
        fract = fractPart * -100.0f;
        data = data | ((fract / 50) << 3) & 0x0008u;
        fract %= 50;
        data = data | ((fract / 25) << 2) & 0x0006u;
    }

    return data;
}
