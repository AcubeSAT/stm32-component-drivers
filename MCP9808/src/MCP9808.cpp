#include "MCP9808.hpp"

etl::expected<void, MCP9808::Error> MCP9808::writeRegister(etl::span<uint8_t> data) {

    if (MCP9808_TWIHS_Write(I2C_BUS_ADDRESS, data.data(), data.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        if (error != static_cast<std::underlying_type_t<Error>>(Error::ERROR_NONE))
            return etl::unexpected(static_cast<Error>(error));

        return etl::unexpected(Error::ERROR_NONE);
    }

    return etl::unexpected(Error::WRITE_REQUEST_FAILED);
}

etl::expected<uint16_t, MCP9808::Error> MCP9808::readRegister(Register address) {
    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> buffer {0};

    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_1BYTE> addr {static_cast<std::underlying_type_t<Register>>(address)};
    const auto WriteError = writeRegister(addr);
    if (WriteError.error() != Error::ERROR_NONE)
        return etl::unexpected(WriteError.error());

    if (MCP9808_TWIHS_Read(I2C_BUS_ADDRESS, buffer.data(), buffer.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        if (error != static_cast<std::underlying_type_t<Error>>(Error::ERROR_NONE))
            return etl::unexpected(static_cast<Error>(error));

        return ((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
    }

    return etl::unexpected(Error::READ_REQUEST_FAILED);
}

etl::expected<void, MCP9808::Error> MCP9808::setRegister(Register address, Mask mask, uint16_t setting) {
    const auto Previous = readRegister(address);
    if (!Previous.has_value())
        return etl::unexpected(Previous.error());

    const uint16_t NewSetting = (static_cast<uint16_t>(mask) & Previous) | setting;

    if (address == Register::REG_RESOLUTION) {  // 1 byte register
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>(NewSetting & 0x00FF)};
        const auto WriteError = writeRegister(etl::span<uint8_t>(data));
        if (WriteError.error() != Error::ERROR_NONE)
            return etl::unexpected(WriteError.error());
    }
    else {  // 2 bytes register
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_3BYTES> data = {static_cast<uint8_t>(address),
                          static_cast<uint8_t>((NewSetting >> 8) & 0x00FF),
                          static_cast<uint8_t>(NewSetting & 0x00FF)};
        const auto WriteError = writeRegister(etl::span<uint8_t>(data));
        if (WriteError.error() != Error::ERROR_NONE)
            return etl::unexpected(WriteError.error());
    }

    return {};
}

etl::expected<void, MCP9808::Error> MCP9808::setHysteresisTemperature(MCP9808::HysteresisTemperatureOptions option) {
    return setRegister(Register::REG_CONFIG, Mask::THYST_MASK, option);
}

etl::expected<void, MCP9808::Error> MCP9808::setLowPowerMode(MCP9808::LowPowerMode setting) {
    return setRegister(Register::REG_CONFIG, Mask::SHDN_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setCriticalTemperatureLock(MCP9808::CriticalTemperatureRegisterLock setting) {
    return setRegister(Register::REG_CONFIG, Mask::TCRIT_LOCK_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setTemperatureWindowLock(MCP9808::TemperatureWindowLock setting) {
    return setRegister(Register::REG_CONFIG, Mask::WINLOCK_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::clearInterrupts() {
    return setRegister(Register::REG_CONFIG, Mask::IRQ_CLEAR_MASK, IRQ_CLEAR);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertStatus(MCP9808::AlertStatus setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_STATUS_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertControl(MCP9808::AlertControl setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_CONTROL_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertSelection(MCP9808::AlertSelection setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_SELECT_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertPolarity(MCP9808::AlertPolarity setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_POLARITY_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertMode(MCP9808::AlertMode setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_MODE_MASK, setting);
}

etl::expected<void, MCP9808::Error> MCP9808::setResolution(MCP9808::MeasurementResolution setting) {
    return setRegister(Register::REG_RESOLUTION, Mask::RES_MASK, setting << 8);
}

etl::expected<float, MCP9808::Error> MCP9808::getTemperature() {
    const auto Data = readRegister(Register::REG_TEMP);
    if (!Data.has_value())
        return etl::unexpected(Data.error());

    uint8_t upperByte = (Data >> 8) & 0x1F;
    const uint8_t LowerByte = Data & 0xFF;

    if ((upperByte & 0x10) != 0) {
        upperByte &= 0x0F;
        return -(256 - (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f));
    }

    return (static_cast<float>(upperByte) * 16.0f + static_cast<float>(LowerByte) / 16.0f);
}

etl::expected<bool, MCP9808::Error> MCP9808::isDeviceConnected() {
    const auto ReadValue = readRegister(Register::REG_MFGID);
    if (ReadValue.has_value())
        return ReadValue.value();
    return etl::unexpected(ReadValue.error());
}

etl::expected<void, MCP9808::Error> MCP9808::setUpperTemperatureLimit(float temp) {
    return setRegister(Register::REG_TUPPER, Mask::TUPPER_TLOWER_TCRIT_MASK, floatConversion(temp));
}

etl::expected<void, MCP9808::Error> MCP9808::setLowerTemperatureLimit(float temp) {
    return setRegister(Register::REG_TLOWER, Mask::TUPPER_TLOWER_TCRIT_MASK, floatConversion(temp));
}

etl::expected<void, MCP9808::Error> MCP9808::setCriticalTemperatureLimit(float temp) {
    return setRegister(Register::REG_TCRIT, Mask::TUPPER_TLOWER_TCRIT_MASK, floatConversion(temp));
}

etl::expected<void, MCP9808::Error> MCP9808::enableLowPowerMode() {
    return setLowPowerMode(LowPowerMode::LOWPWR_ENABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::disableLowPowerMode() {
    return setLowPowerMode(LowPowerMode::LOWPWR_DISABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::enableCriticalTemperatureLock() {
    return setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_ENABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::disableCriticalTemperatureLock() {
    return setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_DISABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::enableTemperatureWindowLock() {
    return setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_ENABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::disableTemperatureWindowLock() {
    return setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_DISABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::enableAlertStatus() {
    return setAlertStatus(AlertStatus::ALERT_ENABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::disableAlertStatus() {
    return setAlertStatus(AlertStatus::ALERT_DISABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::enableAlertControl() {
    return setAlertControl(AlertControl::ALERT_CONTROL_ENABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::disableAlertControl() {
    return setAlertControl(AlertControl::ALERT_CONTROL_DISABLE);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertSelectionOnCriticalTemperature() {
    return setAlertSelection(AlertSelection::ALERT_SELECT_CRITONLY);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertSelectionOnAll() {
    return setAlertSelection(AlertSelection::ALERT_SELECT_ALL);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertPolarityActiveHigh() {
    return setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_HIGH);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertPolarityActiveLow() {
    return setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_LOW);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertModeComparator() {
    return setAlertMode(AlertMode::ALERT_MODE_COMPARATOR);
}

etl::expected<void, MCP9808::Error> MCP9808::setAlertModeInterrupt() {
    return setAlertMode(AlertMode::ALERT_MODE_IRQ);
}

uint16_t MCP9808::floatConversion(float floatToConvert) {
    float intPart;

    const float FractPart = std::modf(floatToConvert, &intPart);
    auto data = static_cast<uint16_t>(std::abs(intPart));
    data = (data << 4) & 0x0FFC;
    data =  floatToConvert < 0.f ? data | 0x1000u : data;  // set the sign bit
    auto fract = static_cast<uint16_t>(std::abs(FractPart * 100.0f));
    data = (data | ((fract / 50) << 3)) & 0x0008u;
    fract %= 50;
    data = (data | ((fract / 25) << 2)) & 0x0006u;

    return data;
}
