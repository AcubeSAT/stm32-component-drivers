#include "InternalFlash.hpp"

[[nodiscard]] FlashDriver::EFCError FlashDriver::QuadWordWrite(FlashData* data, FlashAddress address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_QuadWordWrite(data, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::PageWrite(FlashData* data, FlashAddress address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_PageWrite(data, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::Read(FlashData* data, FlashReadLength length, FlashAddress address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_Read(data, length, address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::SectorErase(FlashAddress address) {
    if(not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_SectorErase(address);

    if(waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}
