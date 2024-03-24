#include "MCP9808.hpp"

void MCP9808::writeRegister(etl::span<uint8_t> data) {

    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, data.data(), data.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
    }
}

uint16_t MCP9808::readRegister(Register address) {
    etl::array<uint8_t, NUM_OF_BYTES_TO_TRANSFER::TRANSFER_2BYTES> buffer = {};

    auto addr = static_cast<std::underlying_type_t<Register>>(address);
    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, &addr, 1)) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
    }

    if (MCP9808_TWIHS_Read(I2C_BUS_ADDRESS, buffer.data(), buffer.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        return ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    } else {
        return 0;
    }
}

void MCP9808::setRegister(Register address, Mask mask, uint16_t setting) {
    auto previous = readRegister(address);

    uint16_t newSetting = (static_cast<uint16_t>(mask) & previous) | setting;

    if (address == Register::REG_RESOLUTION) {  // 1 byte register
        etl::array<uint8_t, NUM_OF_BYTES_TO_TRANSFER::TRANSFER_2BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>(newSetting & 0x00FF)};
        writeRegister(etl::span<uint8_t>(data));
    }
    else {  // 2 bytes register
        etl::array<uint8_t, NUM_OF_BYTES_TO_TRANSFER::TRANSFER_3BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>((newSetting >> 8) & 0x00FF),
                          static_cast<uint8_t>(newSetting & 0x00FF)};
        writeRegister(etl::span<uint8_t>(data));
    }
}

void MCP9808::setHysteresisTemperature(MCP9808::HysteresisTemperatureOptions option) {
    setRegister(Register::REG_CONFIG, Mask::THYST_MASK, option);
}

void MCP9808::setLowPowerMode(MCP9808::LowPowerMode setting) {
    setRegister(Register::REG_CONFIG, Mask::SHDN_MASK, setting);
}

void MCP9808::setCriticalTemperatureLock(MCP9808::CriticalTemperatureRegisterLock setting) {
    setRegister(Register::REG_CONFIG, Mask::TCRIT_LOCK_MASK, setting);
}

void MCP9808::setTemperatureWindowLock(MCP9808::TemperatureWindowLock setting) {
    setRegister(Register::REG_CONFIG, Mask::WINLOCK_MASK, setting);
}

void MCP9808::clearInterrupts() {
    setRegister(Register::REG_CONFIG, Mask::IRQ_CLEAR_MASK, IRQ_CLEAR);
}

void MCP9808::setAlertStatus(MCP9808::AlertStatus setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_STATUS_MASK, setting);
}

void MCP9808::setAlertControl(MCP9808::AlertControl setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_CONTROL_MASK, setting);
}

void MCP9808::setAlertSelection(MCP9808::AlertSelection setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_SELECT_MASK, setting);
}

void MCP9808::setAlertPolarity(MCP9808::AlertPolarity setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_POLARITY_MASK, setting);
}

void MCP9808::setAlertMode(MCP9808::AlertMode setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_MODE_MASK, setting);
}

void MCP9808::setResolution(MCP9808::MeasurementResolution setting) {
    setRegister(Register::REG_RESOLUTION, Mask::RES_MASK, setting << 8);
}

float MCP9808::getTemperature() {
    auto data = readRegister(Register::REG_TEMP);

    uint8_t upperByte = (data >> 8) & 0x1F;
    const uint8_t LowerByte = data & 0xFF;

    if ((upperByte & 0x10) != 0) {
        upperByte &= 0x0F;
        return -(256 - (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f));
    } else {
        return (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f);
    }
}

bool MCP9808::isDeviceConnected() {
    return readRegister(Register::REG_MFGID) == MANUFACTURER_ID;
}

void MCP9808::setUpperTemperatureLimit(float temp) {
    auto data = getData(temp);
    setRegister(Register::REG_TUPPER, Mask::TUPPER_TLOWER_TCRIT_MASK, data);
}

void MCP9808::setLowerTemperatureLimit(float temp) {
    auto data = getData(temp);
    setRegister(Register::REG_TLOWER, Mask::TUPPER_TLOWER_TCRIT_MASK, data);
}

void MCP9808::setCriticalTemperatureLimit(float temp) {
    auto data = getData(temp);
    setRegister(Register::REG_TCRIT, Mask::TUPPER_TLOWER_TCRIT_MASK, data);
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

uint16_t MCP9808::getData(float floatToConvert) {
    float intPart;

    float fractPart = std::modf(floatToConvert, &intPart);
    auto data = static_cast<uint16_t>(std::abs(intPart));
    data = (data << 4) & 0x0FFC;
    data =  floatToConvert < 0.f ? data | 0x1000u : data;  // set the sign bit
    auto fract = static_cast<uint16_t>(std::abs(fractPart * 100.0f));
    data = (data | ((fract / 50) << 3)) & 0x0008u;
    fract %= 50;
    data = (data | ((fract / 25) << 2)) & 0x0006u;

    return data;
}
