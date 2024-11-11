#include "MCP9808.hpp"

void MCP9808::writeRegister(etl::span<uint8_t> data) {

    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, data.data(), data.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
    }
}

uint16_t MCP9808::readRegister(Register address) {
    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> buffer {0};

    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_1BYTE> addr {static_cast<std::underlying_type_t<Register>>(address)};
    writeRegister(addr);

    if (MCP9808_TWIHS_Read(I2C_BUS_ADDRESS, buffer.data(), buffer.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        return address == Register::REG_RESOLUTION ? static_cast<uint16_t>(buffer[0]) & 0x00FF : ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    }

    return 0;
}

void MCP9808::setRegister(Register address, Mask mask, uint16_t setting) {
    const auto Previous = readRegister(address);

    const uint16_t NewSetting = (static_cast<uint16_t>(mask) & Previous) | setting;

    if (address == Register::REG_RESOLUTION) {  // 1 byte register
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>(NewSetting & 0x00FF)};
        writeRegister(etl::span<uint8_t>(data));
    }
    else {  // 2 bytes register
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_3BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>((NewSetting >> 8) & 0x00FF),
                          static_cast<uint8_t>(NewSetting & 0x00FF)};
        writeRegister(etl::span<uint8_t>(data));
    }
}

void MCP9808::setHysteresisTemperature(HysteresisTemperatureOptions option) {
    setRegister(Register::REG_CONFIG, Mask::THYST_MASK, static_cast<std::underlying_type_t<HysteresisTemperatureOptions>>(option));
}

void MCP9808::setLowPowerMode(LowPowerMode setting) {
    setRegister(Register::REG_CONFIG, Mask::SHDN_MASK, static_cast<std::underlying_type_t<LowPowerMode>>(setting));
}

void MCP9808::setCriticalTemperatureLock(CriticalTemperatureRegisterLock setting) {
    setRegister(Register::REG_CONFIG, Mask::TCRIT_LOCK_MASK, static_cast<std::underlying_type_t<CriticalTemperatureRegisterLock>>(setting));
}

void MCP9808::setTemperatureWindowLock(TemperatureWindowLock setting) {
    setRegister(Register::REG_CONFIG, Mask::WINLOCK_MASK, static_cast<std::underlying_type_t<TemperatureWindowLock>>(setting));
}

void MCP9808::clearInterrupts() {
    setRegister(Register::REG_CONFIG, Mask::IRQ_CLEAR_MASK, IRQ_CLEAR);
}

void MCP9808::setAlertStatus(AlertStatus setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_STATUS_MASK, static_cast<std::underlying_type_t<AlertStatus>>(setting));
}

void MCP9808::setAlertControl(AlertControl setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_CONTROL_MASK, static_cast<std::underlying_type_t<AlertControl>>(setting));
}

void MCP9808::setAlertSelection(AlertSelection setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_SELECT_MASK, static_cast<std::underlying_type_t<AlertSelection>>(setting));
}

void MCP9808::setAlertPolarity(AlertPolarity setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_POLARITY_MASK, static_cast<std::underlying_type_t<AlertPolarity>>(setting));
}

void MCP9808::setAlertMode(AlertMode setting) {
    setRegister(Register::REG_CONFIG, Mask::ALERT_MODE_MASK, static_cast<std::underlying_type_t<AlertMode>>(setting));
}

void MCP9808::setResolution(MeasurementResolution setting) {
    setRegister(Register::REG_RESOLUTION, Mask::RES_MASK, static_cast<std::underlying_type_t<MeasurementResolution>>(setting) /*<< 8*/);
}

float MCP9808::getTemperature() {
    return getTemperature(Register::REG_TEMP);
}

float MCP9808::getCriticalTemperatureLimit() {
    return getTemperature(Register::REG_TCRIT);
}

float MCP9808::getUpperTemperatureLimit() {
    return getTemperature(Register::REG_TUPPER);
}

float MCP9808::getLowerTemperatureLimit() {
    return getTemperature(Register::REG_TLOWER);
}

bool MCP9808::isDeviceConnected() {
    return readRegister(Register::REG_MFGID) == MANUFACTURER_ID;
}

void MCP9808::setUpperTemperatureLimit(float temp) {
    setRegister(Register::REG_TUPPER, Mask::CLEAR_MASK, floatToCustomFormat(temp));
}

void MCP9808::setLowerTemperatureLimit(float temp) {
    setRegister(Register::REG_TLOWER, Mask::CLEAR_MASK, floatToCustomFormat(temp));
}

void MCP9808::setCriticalTemperatureLimit(float temp) {
    setRegister(Register::REG_TCRIT, Mask::CLEAR_MASK, floatToCustomFormat(temp));
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

void MCP9808::enableTemperatureWindowLock() {
    setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_ENABLE);
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

uint16_t MCP9808::floatToCustomFormat(float value) {
    float intPart = 0.0;

    const float FractPart = std::modf(value, &intPart);
    auto data = static_cast<uint16_t>(std::abs(intPart));
    data = (data << 4) & 0x0FFC;
    auto fract = static_cast<uint16_t>(std::abs(FractPart * 100.0f));
    data = (data | ((fract / 50) << 3)) & static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);  // adjust bit 3
    fract %= 50;
    data = (data | ((fract / 25) << 2)) & static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);  // adjust bit 2

    return (value > 0.f ? data : ~data + 1) & static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);  // 2's complement representation
}

float MCP9808::getTemperature(Register reg)
{
    const auto Data = readRegister(reg);

    uint8_t upperByte = (Data >> 8) & 0x1F;
    const uint8_t LowerByte = Data & (reg == Register::REG_TEMP ? 0xFF : 0xFC);

    if ((upperByte & 0x10) != 0) {  // negative temperature
        upperByte &= 0x0F;
        return -(256 - (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f));
    }

    return (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f);
}
