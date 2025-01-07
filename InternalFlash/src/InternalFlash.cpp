#include "InternalFlash.hpp"

[[nodiscard]] FlashDriver::EFCError FlashDriver::getEFCError() {
        switch (EFC_ErrorGet()) {
            case EFC_ERROR_NONE:
                return EFCError::NONE;
            case EFC_CMD_ERROR:
                return EFCError::INVALID_COMMAND;
            case EFC_LOCK_ERROR:
                return EFCError::REGION_LOCKED;
            case EFC_FLERR_ERROR:
                return EFCError::FLASH_ERROR;
            case EFC_ECC_ERROR:
                return EFCError::ECC_ERROR;
        }
        return EFCError::UNDEFINED;
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::eraseSector(FlashAddress_t address) {
    if (not isAddressSafe(address)) {
        return EFCError::ADDRESS_UNSAFE;
    }

    EFC_SectorErase(address);

    if (waitForResponse() == EFCError::TIMEOUT) {
        return EFCError::TIMEOUT;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::waitForResponse() {
        auto start = xTaskGetTickCount();
        while (EFC_IsBusy()) {
            if (xTaskGetTickCount() - start > TimeoutTicks) {
                LOG_ERROR << "EFC transaction failed";
                EFC_Initialize();
                return EFCError::TIMEOUT;
            }
            taskYIELD();
        }
        return EFCError::NONE;
}


[[nodiscard]] FlashDriver::EFCError FlashDriver::writeQuadWord(etl::array<uint32_t, WordsPerQuadWord>& data, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::ADDRESS_UNSAFE;
    }

    const auto EraseResult = eraseSector(address);
    if(EraseResult != EFCError::NONE) {
        return EraseResult;
    }

    EFC_QuadWordWrite(data.data(), address);

    if(waitForResponse() == EFCError::TIMEOUT) {
        return EFCError::TIMEOUT;
    }

    return getEFCError();
}

[[nodiscard]] FlashDriver::EFCError FlashDriver::writePage(etl::array<uint32_t, WordsPerPage>& data, FlashAddress_t address) {
    if(not isAddressSafe(address)) {
        return EFCError::ADDRESS_UNSAFE;
    }

    const auto EraseResult = eraseSector(address);
    if(EraseResult != EFCError::NONE) {
        return EraseResult;
    }

    EFC_PageWrite(data.data(), address);

    if(waitForResponse() == EFCError::TIMEOUT) {
        return EFCError::TIMEOUT;
    }

    return getEFCError();
}

