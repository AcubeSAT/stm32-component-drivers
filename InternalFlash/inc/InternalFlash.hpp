#pragma once

#include <cstdint>
#include "peripheral/pio/plib_pio.h"
#include "plib_efc.h"
#include "FreeRTOS.h"
#include "Logger.hpp"
#include "task.h"

// todo (#31): Find out if isAlligned function is needed.
// todo (#32): Remove FreeRTOS functions.
// todo (#33): Add WriteFloat wrapper function.
// todo (#34): Investigate if we need class instacnes.
// todo (#35): Protect written data.
// todo (#36): General Memory class.
// todo (#37): Multi-variable write.

/**
 * Class to interact with the Internal FLASH Memory of the MCU.
 *
 * @brief This class provides functions with a generic implementation based on the EFC peripheral library for easy integration into any code.
 */
class FlashDriver {
public:
    using FlashAddress_t = uint32_t;
    using FlashReadLength_t = uint32_t;

    /**
     * @enum EFCError
     * @brief enum to represent various EFC errors
     */
    enum class EFCError : uint8_t {
        None,
        Timeout,
        AddressUnsafe,
        AdressNotAligned,
        InvalidCommand,
        RegionLocked,
        FlashError,
        ECCError,
        Undefined
    };

    /**
     * Number of bits in a single byte, used for calculations.
     */
    static constexpr uint8_t numOfBitsinByte = 8;

    /**
     * Wait period before an EFC transaction is skipped.
     */
    static constexpr uint16_t timeoutTicks = 1000;

    /**
     * Size of a flash page in Bytes.
     */
    static constexpr uint32_t flashPageSize = 512;

    /**
     * Size of 4 words (128 bits) in Bytes.
     */
    static constexpr uint32_t quadWordSize = 16;

    /**
     * Starting adress of available Flash memory.
     */
    static constexpr uint32_t startAddress = 0x5F0000;

    /**
     * End limit of available Flash memory.
     */
    static constexpr uint32_t endAddress = 0x600000;

    /**
     * Constructor to create an instance of the class.
     */
    FlashDriver() = default;

    /**
    * Templated write function for writing 128 bits (QuadWord). Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased, as documented in Harmony Peripheral Libraries 2.39.3.
     * @param data Array containing the data to be written.
     * @param address FLASH address to be modified.
     * @return member of the EFCError enum.
     */
    template <typename T, size_t N>
    [[nodiscard]] static EFCError writeQuadWord( etl::array<T, N> data, FlashAddress_t address) {
        static_assert((sizeof(T) * N) <= quadWordSize * numOfBitsinByte, "Data size exceeds 128 bits.");

        if (not isAddressSafe(address)) {
            return EFCError::AddressUnsafe;
        }

        const auto eraseResult = eraseSector(address);
        if (eraseResult != EFCError::None) {
            return eraseResult;
        }

        EFC_QuadWordWrite(static_cast<uint32_t *>(const_cast<T *>(data.data())), address);


        if (waitForResponse() == EFCError::Timeout) {
            return EFCError::Timeout;
        }

        return getEFCError();
    }

    /**
    * Templated write function for writing a page. Only ‘0’ values can be programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased, as documented in Harmony Peripheral Libraries 2.39.3.
     * @param data Array containing the data to be written.
     * @param address FLASH address to be modified.
     * @return member of the EFCError enum.
     */
    template <typename T, size_t N>
    [[nodiscard]] static EFCError writePage( etl::array<T, N> data, FlashAddress_t address) {
        static_assert((sizeof(T) * N) == flashPageSize * numOfBitsinByte, "Data size must match the page size.");

        if (not isAddressSafe(address)) {
            return EFCError::AddressUnsafe;
        }

        const auto eraseResult = eraseSector(address);
        if (eraseResult != EFCError::None) {
            return eraseResult;
        }

        EFC_PageWrite(static_cast<uint32_t *>(const_cast<T *>(data.data())), address);

        if (waitForResponse() == EFCError::Timeout) {
            return EFCError::Timeout;
        }

        return getEFCError();
    }

    /**
     * Reads a specified length of bytes from a given address in FLASH memory into
     * the user-provided buffer.
     * @tparam T The type of data stored in the array (e.g., uint8_t or uint32_t).
     * @tparam N The number of elements in the array.
     * @param data A reference to an array of type T and size N to store the read data.
     * @param length The number of bytes to read from the flash memory.
     * @param address FLASH address to be read from.
     * @return A member of the EFCError enum indicating the result.
     */
    template <typename T, size_t N>
    [[nodiscard]] static EFCError readFromMemory(etl::array<T, N>& data,const FlashReadLength_t length, FlashAddress_t address) {
        if (not isAddressSafe(address)) {
            return EFCError::AddressUnsafe;
        }

        EFC_Read(static_cast<uint32_t*>(data.data()), length, address);

        if (waitForResponse() == EFCError::Timeout) {
            return EFCError::Timeout;
        }

        return getEFCError();
}

private:
    /**
     * Ensure Flash address used is correctly alligned.
     * @param address FLASH address to be modified.
     * @param alignment the allignment that should be kept.
     * @return True if the address is correctly aligned.
     */
    static bool isAligned(FlashAddress_t address, uint32_t alignment) {
        return (address % alignment) == 0;
    }

    /**
     * Ensure Flash addressed used is within defined limits.
     * @param address FLASH address to be modified.
     * @return True if the address is within the limits defined.
     */
    static bool isAddressSafe(FlashAddress_t address) {
        return address >= startAddress && address < endAddress;
    }

    /**
     * Function to get match internal EFC errors to the EFCError enum.
     * @return the error returned from EFC_ErrorGet as a EFCError.
     */
    static EFCError getEFCError() {
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
    [[nodiscard]] static EFCError eraseSector(FlashAddress_t address) {
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
    static EFCError waitForResponse() {
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
};
