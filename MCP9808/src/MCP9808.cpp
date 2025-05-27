#include "MCP9808.hpp"

MCP9808::Error MCP9808::writeRegister(etl::span<uint8_t> data) {

    if (MCP9808_TWIHS_Write(I2CBusAddress, data.data(), data.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        if (error != static_cast<std::underlying_type_t<Error>>(Error::ERROR_NONE))
            return static_cast<Error>(error);

        return Error::ERROR_NONE;
    }

    return Error::WRITE_REQUEST_FAILED;
}

etl::expected<uint16_t, MCP9808::Error> MCP9808::readRegister(Register address) {
    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> buffer{0};

    etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_1BYTE> addr{
            static_cast<std::underlying_type_t<Register>>(address)};
    const auto WriteError = writeRegister(addr);
    if (WriteError.error() != Error::ERROR_NONE)
        return etl::unexpected(WriteError.error());

    if (MCP9808_TWIHS_Read(I2CBusAddress, buffer.data(), buffer.size())) {
        waitForResponse();
        error = MCP9808_TWIHS_ErrorGet();
        if (error != static_cast<std::underlying_type_t<Error>>(Error::ERROR_NONE))
            return etl::unexpected(static_cast<Error>(error));

        return address == Register::REG_RESOLUTION ? static_cast<uint16_t>(buffer[0]) & 0x00FF : (
                (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));;
    }

    return etl::unexpected(Error::READ_REQUEST_FAILED);
}

MCP9808::Error MCP9808::setRegister(Register address, Mask mask, uint16_t setting) {
    const auto Previous = readRegister(address);
    if (!Previous.has_value())
        return Previous.error();

    const uint16_t NewSetting = (static_cast<uint16_t>(mask) & Previous) | setting;

    if (address == Register::REG_RESOLUTION) {
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_2BYTES> data = {static_cast<uint8_t>(address),
                                                                           static_cast<uint8_t>(NewSetting & 0x00FF)};
        const auto WriteError = writeRegister(etl::span<uint8_t>(data));
        if (WriteError != Error::ERROR_NONE)
            return WriteError;
    } else {
        etl::array<uint8_t, NumOfBytesToTransfer::TRANSFER_3BYTES> data = {static_cast<uint8_t>(address),
                                                                           static_cast<uint8_t>((NewSetting >> 8) &
                                                                                                0x00FF),
                                                                           static_cast<uint8_t>(NewSetting & 0x00FF)};
        const auto WriteError = writeRegister(etl::span<uint8_t>(data));
        if (WriteError != Error::ERROR_NONE)
            return WriteError;
    }

    return {};
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
    return setRegister(Register::REG_CONFIG, Mask::ALERT_STATUS_MASK, static_cast<std::underlying_type_t<AlertStatus>>(setting));;
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
    const auto Data = readRegister(reg);
    if (!Data.has_value())
        return etl::unexpected(Data.error());

    uint8_t upperByte = (Data.value() >> ByteShift) & TempUpperByteMask;
    const uint8_t LowerByte = Data.value() & TempLowerByteMask;

    if ((upperByte & TempSignBitMask) != 0) {
        upperByte &= TempValueMask;
        return -(TempNegativeOffset - (static_cast<float>(upperByte) * TempConvFactor + static_cast<float>(LowerByte) / TempConvFactor));
    }

    return (static_cast<float>(upperByte) * TempConvFactor + static_cast<float>(LowerByte) / TempConvFactor);
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
    const auto ReadValue = readRegister(Register::REG_MFGID);
    if (ReadValue.has_value())
        if(ReadValue.value() == ManufacturerID)
            return Error::ERROR_NONE;
        else if(ReadValue.value() == FalseData)
            return Error::ID_READ_FAILED;
        else
            return Error::ID_READ_WAS_WRONG;
    else
        return ReadValue.error();
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
    float intPart = 0.0;

    const float FractPart = std::modf(value, &intPart);
    auto data = static_cast<uint16_t>(std::abs(intPart));
    data = (data << FloatConvShift1) & FloatConvIntMask;
    auto fract = static_cast<uint16_t>(std::abs(FractPart * FloatConvFractBase));
    data = (data | ((fract / FloatConvFractDiv1) << FloatConvShift2)) &
           static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);
    fract %= FloatConvFractDiv1;
    data = (data | ((fract / FloatConvFractDiv2) << FloatConvShift3)) &
           static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);

    return (value > 0.f ? data : ~data + 1) &
           static_cast<std::underlying_type_t<Mask>>(Mask::TUPPER_TLOWER_TCRIT_MASK);
}

