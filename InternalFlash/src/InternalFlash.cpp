#include "InternalFlash.hpp"

[[nodiscard]] FlashDriver::EFCError FlashDriver::writeQuadWord(uint32_t data, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    auto Erase = eraseSector(address);
    if(Erase != EFCError::None) {
        return Erase;
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

    auto Erase = eraseSector(address);
    if(Erase != EFCError::None) {
        return Erase;
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
