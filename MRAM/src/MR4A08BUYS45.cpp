#include "MR4A08BUYS45.hpp"

bool MRAM::isAddressRangeValid(uint32_t startAddress, size_t size) const {
    if (startAddress > MaxWriteableAddress || 
        size > MaxWriteableAddress ||
        (startAddress + size - 1) > MaxWriteableAddress) {
        return false;
    }

    uint32_t endAddress = moduleBaseAddress | (startAddress + size - 1);
    return endAddress <= moduleEndAddress;
}

MRAMError MRAM::mramWriteByte(uint32_t dataAddress, uint8_t data) {
    if (!isAddressRangeValid(dataAddress, 1)) {
        errorHandler(MRAMError::ADDRESS_OUT_OF_BOUNDS);
        return MRAMError::ADDRESS_OUT_OF_BOUNDS;
    }

    uint32_t writeAddress = moduleBaseAddress | dataAddress;
    smcWriteByte(writeAddress, data);
    return MRAMError::NONE;
}

MRAMError MRAM::mramReadByte(uint32_t dataAddress, uint8_t &data) {
    if (!isAddressRangeValid(dataAddress, 1)) {
        errorHandler(MRAMError::ADDRESS_OUT_OF_BOUNDS);
        return MRAMError::ADDRESS_OUT_OF_BOUNDS;
    }

    uint32_t readAddress = moduleBaseAddress | dataAddress;
    data = smcReadByte(readAddress);
    return MRAMError::NONE;
}

MRAMError MRAM::mramWriteData(uint32_t startAddress, etl::span<const uint8_t> data) {
    if (data.empty()) {
        return MRAMError::INVALID_ARGUMENT;
    }

    if (!isAddressRangeValid(startAddress, data.size())) {
        errorHandler(MRAMError::ADDRESS_OUT_OF_BOUNDS);
        return MRAMError::ADDRESS_OUT_OF_BOUNDS;
    }

    for (size_t i = 0; i < data.size(); i++) {
        MRAMError error = mramWriteByte(startAddress + i, data[i]);
        if (error != MRAMError::NONE) {
            return error;
        }
    }
    return MRAMError::NONE;
}

MRAMError MRAM::mramReadData(uint32_t startAddress, etl::span<uint8_t> data) {
    if (data.empty()) {
        return MRAMError::INVALID_ARGUMENT;
    }

    if (!isAddressRangeValid(startAddress, data.size())) {
        errorHandler(MRAMError::ADDRESS_OUT_OF_BOUNDS);
        return MRAMError::ADDRESS_OUT_OF_BOUNDS;
    }

    for (size_t i = 0; i < data.size(); i++) {
        MRAMError error = mramReadByte(startAddress + i, data[i]);
        if (error != MRAMError::NONE) {
            return error;
        }
    }
    return MRAMError::NONE;
}

void MRAM::writeID(void) {
    for (size_t i = 0; i < CustomIDSize; i++) {
        uint32_t address = moduleBaseAddress | (CustomMRAMIDAddress + i);
        smcWriteByte(address, CustomID[i]);
    }
}
bool MRAM::checkID(etl::span<const uint8_t> idArray) {
    if (idArray.size() != CustomIDSize) {
        return false;
    }

    for (size_t i = 0; i < CustomIDSize; i++) {
        if (idArray[i] != CustomID[i]) {
            return false;
        }
    }
    return true;
}

MRAMError MRAM::isMRAMAlive() {
    etl::array<uint8_t, CustomIDSize> readId{};
    etl::span<uint8_t> readIDspan(readId.data(), readId.size());

    MRAMError error = mramReadData(CustomMRAMIDAddress, readIDspan);
    if (error != MRAMError::NONE) {
        return error;
    }

    const etl::span<const uint8_t> constReadIDSpan(readId.data(), readId.size());
    if (checkID(constReadIDSpan)) {
        return MRAMError::READY;
    }

    // Try writing the ID since it might be the first boot
    writeID();
    
    error = mramReadData(CustomMRAMIDAddress, readIDspan);
    if (error != MRAMError::NONE) {
        return error;
    }

    if (checkID(constReadIDSpan)) {
        return MRAMError::READY;
    }

    return MRAMError::NOT_READY;
}
void MRAM::errorHandler(MRAMError error) {
    switch (error) {
        case MRAMError::TIMEOUT:
            break;
            
        case MRAMError::ADDRESS_OUT_OF_BOUNDS:
            break;
            
        case MRAMError::NOT_READY:
            break;
            
        case MRAMError::INVALID_ARGUMENT:
            break;
            
        default:
            break;
    }
}