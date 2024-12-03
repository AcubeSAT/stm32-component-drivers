#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "plib_efc.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"

/**
 * Class to interact with the Internal FLASH Memory of the MCU.
 *
 * @brief This class provides functions with a generic implementation based on the EFC peripheral library for easy integration into any code.
 */
class FlashDriver {
public:
    using FlashAddress_t = uint32_t;
    using FlashData_t = uint32_t;
    using FlashReadLength_t = uint32_t;

    /**
     * @enum EFCError
     * @brief enum to represent various EFC errors
     */
    enum class EFCError : uint8_t {
        None,
        Timeout,
        AddressUnsafe,
        AdressNotAlligned,
        InvalidCommand,
        RegionLocked,
        FlashError,
        ECCError,
        Undefined
    };

    /**
     * Wait period before an EFC transaction is skipped.
     */
    static constexpr uint16_t TimeoutTicks = 1000;

    /**
     * Size of a flash page in Bytes.
     */
    static constexpr uint32_t FlashPageSize = 512;

    /**
     * Size of 4 words (128 bits) in Bytes.
     */
    static constexpr uint32_t QuadWordSize = 16;

    /**
     * Starting adress of available Flash memory.
     */
    static constexpr uint32_t StartAddress = 0x5F0000;

    /**
     * End limit of available Flash memory.
     */
    static constexpr uint32_t EndAddress = 0x600000;

    /**
     * Constructor to create an instance of the class.
     */
    FlashDriver() = default;

    /**
     * Write function that writes 128 bits. Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased.
     * @param data pointer to 128-bit data buffer.
     * @param address FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    static EFCError writeQuadWord(FlashData_t* data, FlashAddress_t address);

    /**
     * Write function that writes an entire page of 512 bytes. Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased.
     * @param data pointer to data buffer of size equivalent to page size.
     * @param address FLASH address to be modified.
     * @return member of the EFC_ERROR enum.
     */
    static EFCError writePage(FlashData_t* data, FlashAddress_t address);

    /**
     * Reads length number of bytes from a given address in FLASH memory into
     * the user buffer.
     * @param data pointer to user data buffer.
     * @param length number of bytes to read.
     * @param address FLASH address to be read from.
     * @return member of the EFC_ERROR enum.
     */
    static EFCError readFromMemory(FlashData_t* data, FlashReadLength_t length, FlashAddress_t address);

private:
    /**
     * Ensure Flash address used is correctly alligned.
     * @param address FLASH address to be modified.
     * @param alignment the allignment that should be kept.
     * @return True if the address is correctly aligned.
     */
    static inline bool isAligned(FlashAddress_t address, uint32_t alignment) {
        return (address % alignment) == 0;
    }

    /**
     * Ensure Flash addressed used is within defined limits.
     * @param address FLASH address to be modified.
     * @return True if the address is within the limits defined.
     */
    static inline bool isAddressSafe(FlashAddress_t address) {
        return address >= StartAddress && address < EndAddress;
    }

    /**
     * Function to get match internal EFC errors to the EFCError enum.
     * @return the error returned from EFC_ErrorGet as a EFCError.
     */
    static inline EFCError getEFCError() {
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

    /**
     * This function is used to erase a sector.
     * @param address FLASH address to be Erased.
     * @return member of the EFC_ERROR enum.
     */
    [[nodiscard]] static inline EFCError eraseSector(FlashAddress_t address) {
        if(not isAddressSafe(address)) {
            return EFCError::AddressUnsafe;
        }

        EFC_SectorErase(address);

        if(waitForResponse() == EFCError::Timeout) {
            return EFCError::Timeout;
        }

        return getEFCError();
    }

    /**
     * Function to ensure that no calls to EFC are made while a transaction is happening and to ensure the transaction doesn't get stuck.
     * @return Timeout is the transaction got stuck, None otherwise.
     */
    static inline EFCError waitForResponse() {
        auto start = xTaskGetTickCount();
        while (EFC_IsBusy()) {
            if (xTaskGetTickCount() - start > TimeoutTicks) {
                LOG_ERROR << "EFC transaction failed";
                EFC_Initialize();
             return EFCError::Timeout;
            }
            taskYIELD();
        }
        return EFCError::None;
    }
};
