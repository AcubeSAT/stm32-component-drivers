#include "InternalFlash.hpp"

[[nodiscard]] FlashDriver::EFCError FlashDriver::writeQuadWord(uint32_t data, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    const auto erase = eraseSector(address);
    if(erase != EFCError::None) {
        return erase;
    }

    EFC_QuadWordWrite(&data, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::writePage(uint32_t data, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    const auto erase = eraseSector(address);
    if(erase != EFCError::None) {
        return erase;
    }

    EFC_PageWrite(&data, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::readFromMemory(uint32_t& data, FlashReadLength_t length, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_Read(&data, length, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}
