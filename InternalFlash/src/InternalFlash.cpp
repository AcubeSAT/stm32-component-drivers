#include "InternalFlash.hpp"

[[nodiscard]] FlashDriver::EFCError FlashDriver::getEFCError() {
    switch (EFC_ErrorGet()) {
        case EFC_ERROR_NONE:
            return EFCError::None;
        case EFC_CMD_ERROR:
            return EFCError::InvalidCommand;
        case EFC_LOCK_ERROR:
            return EFCError::RegionLocked;
        case EFC_FLERR_ERROR:
            return EFCError::FlashError;
        case EFC_ECC_ERROR:
            return EFCError::ECCError;
    }
    return EFCError::Undefined;
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::eraseSector(FlashAddress_t address) {
    if (not isAddressSafe(address)) {
        return EFCError::AddressUnsafe;
    }

    EFC_SectorErase(address);

    if (waitForResponse() == EFCError::Timeout) {
        return EFCError::Timeout;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::waitForResponse() {
    auto start = xTaskGetTickCount();
    while (EFC_IsBusy()) {
        if (xTaskGetTickCount() - start > timeoutTicks) {
            LOG_ERROR << "EFC transaction failed";
            EFC_Initialize();
            return EFCError::Timeout;
        }
        taskYIELD();
    }
    return EFCError::None;
}
