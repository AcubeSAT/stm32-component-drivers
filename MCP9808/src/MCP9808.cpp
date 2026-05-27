#include "MCP9808.hpp"
#include "HAL_I2C.hpp"

MCP9808::Error MCP9808::write(etl::span<uint8_t> data) {
    return convertI2cError(HAL_I2C::writeRegister<PeripheralNumber>(I2CBaseAddress, data));
}

MCP9808::Error MCP9808::read(etl::span<uint8_t> i2cData) {
    return convertI2cError(HAL_I2C::readRegister<PeripheralNumber>(I2CBaseAddress, i2cData));
};

MCP9808::Error MCP9808::writeReadRaw(Register address, etl::span<uint8_t> i2cData) {
    etl::array<uint8_t, 1> data{static_cast<uint8_t>(address)};
    return convertI2cError(HAL_I2C::writeReadRegister<PeripheralNumber>(I2CBaseAddress, data, i2cData));
}

MCP9808::Error MCP9808::writeRead(Register address, etl::array<uint8_t, 2> i2cData) {
    if (auto error = writeRead(address, i2cData); error != Error::NONE) {
        return error;
    }

    if (address == Register::REG_RESOLUTION) {
        i2cData[1] = 0;
    }
    return Error::NONE;
}


MCP9808::Error MCP9808::setRegister(Register address, Mask mask, uint16_t setting) {
    etl::array<uint8_t, 2> data;
    const auto WriteReadError = writeRead(address, data);
    if (WriteReadError != Error::NONE)
        return WriteReadError;
    const uint16_t Previous = (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
    const uint16_t NewSetting = (static_cast<uint16_t>(mask) & Previous) | setting;

    if (address == Register::REG_RESOLUTION) {
        etl::array<uint8_t, 2> data = {static_cast<uint8_t>(address),
                                                                           static_cast<uint8_t>(NewSetting & 0x00FF)};
        if (auto error = write(etl::span<uint8_t>(data)); error != Error::NONE)
            return error;
    } else {
        etl::array<uint8_t, 3> data = {static_cast<uint8_t>(address),
                                                                           static_cast<uint8_t>((NewSetting >> 8) &
                                                                                                0x00FF),
                                                                           static_cast<uint8_t>(NewSetting & 0x00FF)};
        if (auto error = write(etl::span<uint8_t>(data)); error != Error::NONE)
            return error;
    }

    return Error::NONE;
}

MCP9808::Error MCP9808::setHysteresisTemperature(MCP9808::HysteresisTemperatureOptions option) {
    return setRegister(Register::REG_CONFIG, Mask::THYST_MASK,
                       static_cast<std::underlying_type_t<HysteresisTemperatureOptions>>(option));
}

MCP9808::Error MCP9808::setLowPowerMode(MCP9808::LowPowerMode setting) {
    return setRegister(Register::REG_CONFIG, Mask::SHDN_MASK,
                       static_cast<std::underlying_type_t<LowPowerMode>>(setting));
}

MCP9808::Error MCP9808::setCriticalTemperatureLock(MCP9808::CriticalTemperatureRegisterLock setting) {
    return setRegister(Register::REG_CONFIG, Mask::TCRIT_LOCK_MASK,
                       static_cast<std::underlying_type_t<CriticalTemperatureRegisterLock>>(setting));

}

MCP9808::Error MCP9808::setTemperatureWindowLock(MCP9808::TemperatureWindowLock setting) {
    return setRegister(Register::REG_CONFIG, Mask::WINLOCK_MASK, static_cast<std::underlying_type_t<TemperatureWindowLock>>(setting));
}

MCP9808::Error MCP9808::clearInterrupts() {
    return setRegister(Register::REG_CONFIG, Mask::IRQ_CLEAR_MASK, IrqClear);
}

MCP9808::Error MCP9808::setAlertStatus(MCP9808::AlertStatus setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_STATUS_MASK, static_cast<std::underlying_type_t<AlertStatus>>(setting));
}

MCP9808::Error MCP9808::setAlertControl(MCP9808::AlertControl setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_CONTROL_MASK, static_cast<std::underlying_type_t<AlertControl>>(setting));
}

MCP9808::Error MCP9808::setAlertSelection(MCP9808::AlertSelection setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_SELECT_MASK, static_cast<std::underlying_type_t<AlertSelection>>(setting));
}

MCP9808::Error MCP9808::setAlertPolarity(MCP9808::AlertPolarity setting) {
    return  setRegister(Register::REG_CONFIG, Mask::ALERT_POLARITY_MASK, static_cast<std::underlying_type_t<AlertPolarity>>(setting));
}

MCP9808::Error MCP9808::setAlertMode(MCP9808::AlertMode setting) {
    return setRegister(Register::REG_CONFIG, Mask::ALERT_MODE_MASK, static_cast<std::underlying_type_t<AlertMode>>(setting));
}

MCP9808::Error MCP9808::setResolution(MCP9808::MeasurementResolution setting) {
    return setRegister(Register::REG_RESOLUTION, Mask::RES_MASK, static_cast<std::underlying_type_t<MeasurementResolution>>(setting) /*<< 8*/);
}

etl::expected<float, MCP9808::Error> MCP9808::getTemperature() {
    return getTemperature(Register::REG_TEMP);
}

etl::expected<float, MCP9808::Error> MCP9808::getTemperature(Register reg) {
    etl::array<uint8_t, 2> data = {};
    if (auto error = writeRead(reg, data); error != Error::NONE)
        return etl::unexpected(error);

    uint8_t UpperByte = data[0] & TempUpperByteMask;
    const uint8_t LowerByte = data[1];

    if ((UpperByte & TempSignBitMask) != 0) {
        UpperByte &= TempValueMask;
        const float IntPart = static_cast<float>(UpperByte) * TempConvFactor;
        const float FracPart = static_cast<float>(LowerByte) / TempConvFactor;
        const float Magnitude = IntPart + FracPart;
        return -(TempNegativeOffset - Magnitude);
    }

    const float IntPart = static_cast<float>(UpperByte) * TempConvFactor;
    const float FracPart = static_cast<float>(LowerByte) / TempConvFactor;
    return IntPart + FracPart;
}

etl::expected<float, MCP9808::Error> MCP9808::getCriticalTemperatureLimit() {
    return getTemperature(Register::REG_TCRIT);
}

etl::expected<float, MCP9808::Error> MCP9808::getUpperTemperatureLimit() {
    return getTemperature(Register::REG_TUPPER);
}

etl::expected<float, MCP9808::Error> MCP9808::getLowerTemperatureLimit() {
    return getTemperature(Register::REG_TLOWER);
}

MCP9808::Error MCP9808::isDeviceConnected() {
    etl::array<uint8_t, 2> data = {};
    if (auto error = writeRead(Register::REG_MFGID, data); error != Error::NONE)
        return error;

    const uint16_t ReadValue = (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);

    if (ReadValue == ManufacturerID)
        return Error::NONE;
    else
        return Error::ID_READ_WAS_WRONG;
}

MCP9808::Error MCP9808::setUpperTemperatureLimit(float temp) {
    return setRegister(Register::REG_TUPPER, Mask::CLEAR_MASK, floatToCustomFormat(temp));
}

MCP9808::Error MCP9808::setLowerTemperatureLimit(float temp) {
    return setRegister(Register::REG_TLOWER, Mask::CLEAR_MASK, floatToCustomFormat(temp));
}

MCP9808::Error MCP9808::setCriticalTemperatureLimit(float temp) {
    return setRegister(Register::REG_TCRIT, Mask::CLEAR_MASK, floatToCustomFormat(temp));
}

MCP9808::Error MCP9808::enableLowPowerMode() {
    return setLowPowerMode(LowPowerMode::LOWPWR_ENABLE);
}

MCP9808::Error MCP9808::disableLowPowerMode() {
    return setLowPowerMode(LowPowerMode::LOWPWR_DISABLE);
}

MCP9808::Error MCP9808::enableCriticalTemperatureLock() {
    return setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_ENABLE);
}

MCP9808::Error MCP9808::disableCriticalTemperatureLock() {
    return setCriticalTemperatureLock(CriticalTemperatureRegisterLock::TCRIT_LOCK_DISABLE);
}

MCP9808::Error MCP9808::enableTemperatureWindowLock() {
    return setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_ENABLE);
}

MCP9808::Error MCP9808::disableTemperatureWindowLock() {
    return setTemperatureWindowLock(TemperatureWindowLock::WINLOCK_DISABLE);
}

MCP9808::Error MCP9808::enableAlertStatus() {
    return setAlertStatus(AlertStatus::ALERT_ENABLE);
}

MCP9808::Error MCP9808::disableAlertStatus() {
    return setAlertStatus(AlertStatus::ALERT_DISABLE);
}

MCP9808::Error MCP9808::enableAlertControl() {
    return setAlertControl(AlertControl::ALERT_CONTROL_ENABLE);
}

MCP9808::Error MCP9808::disableAlertControl() {
    return setAlertControl(AlertControl::ALERT_CONTROL_DISABLE);
}

MCP9808::Error MCP9808::setAlertSelectionOnCriticalTemperature() {
    return setAlertSelection(AlertSelection::ALERT_SELECT_CRITONLY);
}

MCP9808::Error MCP9808::setAlertSelectionOnAll() {
    return setAlertSelection(AlertSelection::ALERT_SELECT_ALL);
}

MCP9808::Error MCP9808::setAlertPolarityActiveHigh() {
    return setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_HIGH);
}

MCP9808::Error MCP9808::setAlertPolarityActiveLow() {
    return setAlertPolarity(AlertPolarity::ALERT_POLARITY_ACTIVE_LOW);
}

MCP9808::Error MCP9808::setAlertModeComparator() {
    return setAlertMode(AlertMode::ALERT_MODE_COMPARATOR);
}

MCP9808::Error MCP9808::setAlertModeInterrupt() {
    return setAlertMode(AlertMode::ALERT_MODE_IRQ);
}

uint16_t MCP9808::floatToCustomFormat(float value) {
    // Helper to avoid repeating the verbose cast
    constexpr auto TempRegisterMask = static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);

    // Split into integer and fractional parts
    float IntPartFloat = 0.0f;
    const float FractPart = std::modf(value, &IntPartFloat);

    // Encode integer part into bits 11:4 of the register
    const auto IntPartBits = static_cast<uint16_t>(std::abs(IntPartFloat));
    uint16_t Result = (IntPartBits << FloatConvShift1) & FloatConvIntMask;

    // Convert fraction to hundredths for easier bit extraction
    auto FractHundredths = static_cast<uint16_t>(std::abs(FractPart * FloatConvFractBase));

    // Encode 0.50 degree bit (bit 3) and 0.25 degree bit (bit 2)
    Result |= (FractHundredths / FloatConvFractDiv1) << FloatConvShift2;
    FractHundredths %= FloatConvFractDiv1;
    Result |= (FractHundredths / FloatConvFractDiv2) << FloatConvShift3;
    Result &= TempRegisterMask;

    // Apply two's complement for negative temperatures
    const uint16_t SignedResult = (value > 0.f) ? Result : (~Result + 1);
    return SignedResult & TempRegisterMask;
}
MCP9808::Error MCP9808::convertI2cError(HAL_I2C::I2CError error) {
    switch (error) {
    case HAL_I2C::I2CError::NONE:
        return MCP9808::Error::NONE;
        case HAL_I2C::I2CError::BUSY:
        return MCP9808::Error::I2C_BUSY;
    case HAL_I2C::I2CError::TIMEOUT:
        return MCP9808::Error::I2C_TIMEOUT;
    case HAL_I2C::I2CError::INVALID_PARAMS:
        return MCP9808::Error::INVALID_PARAMS;
    case HAL_I2C::I2CError::OPERATION_ERROR:
        return MCP9808::Error::I2C_OPERATION_ERROR;
    default:
        return MCP9808::Error::UNKNOWN_ERROR;
    }
}