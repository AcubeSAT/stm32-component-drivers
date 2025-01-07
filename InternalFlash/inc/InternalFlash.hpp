#pragma once

#include "FreeRTOS.h"
#include "Logger.hpp"
#include "peripheral/pio/plib_pio.h"
#include "plib_efc.h"
#include "task.h"
#include <cstdint>

/**
 * todo (#31): Find out if isAlligned function is needed.
 * todo (#32): Remove FreeRTOS functions.
 * todo (#33): Add WriteFloat wrapper function.
 * todo (#34): Investigate if we need class instacnes.
 * todo (#35): Protect written data.
 * todo (#36): General Memory class.
 * todo (#37): Multi-variable write.
 * todo (#39): Peripheral initialization.
 * todo (#42): Find out why a reference doesn't work in writeQuadWord.
 */

/**
 * Class to interact with the Internal FLASH Memory of the MCU.
 *
 * @brief This class provides functions with a generic implementation based on
 * the EFC peripheral library for easy integration into any code.
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
        NONE,
        TIMEOUT,
        ADDRESS_UNSAFE,
        ADDRESS_NOT_ALIGNED,
        INVALID_COMMAND,
        REGION_LOCKED,
        FLASH_ERROR,
        ECC_ERROR,
        UNDEFINED,
    };

    /**
     * Number of bits in a single byte, used for calculations.
     */
    static constexpr uint8_t NumOfBitsinByte = 8;

    /**
     * Wait period before an EFC transaction is skipped.
     */
    static constexpr TickType_t TimeoutTicks = 1000;

    /**
     * Starting adress of available Flash memory.
     */
    static constexpr uint32_t StartAddress = 0x5F0000;

    /**
     * End limit of available Flash memory.
     */
    static constexpr uint32_t EndAddress = 0x600000;

    /**
     * Number of 32-bit words in a quad word (128 bits).
     */
    static constexpr uint8_t WordsPerQuadWord = 4;

    /**
     * Number of 32-bit words in a flash page.
     */
    static constexpr uint8_t WordsPerPage = 128;

    /**
     * Constructor to create an instance of the class.
     */
    FlashDriver() = default;

    /**
     * Write function for writing 128 bits (QuadWord). Only ‘0’ values can be
     * programmed using Flash technology; ‘1’ is the erased value. In order to
     * program words in a page, the page must first be erased, as documented in
     * Harmony Peripheral Libraries 2.39.3.
     * @param data Array containing the data to be written.
     * @param address FLASH address to be modified.
     * @return Member of the EFCError enum.
     */
    [[nodiscard]] EFCError
    writeQuadWord(etl::array<uint32_t, WordsPerQuadWord> &data,
                  FlashAddress_t address);

    /**
     * Write function for writing a page. Only ‘0’ values can be programmed
     * using Flash technology; ‘1’ is the erased value. In order to program
     * words in a page, the page must first be erased, as documented in Harmony
     * Peripheral Libraries 2.39.4.
     * @param data Array containing the data to be written.
     * @param address FLASH address to be modified.
     * @return Member of the EFCError enum.
     */
    [[nodiscard]] EFCError writePage(etl::array<uint32_t, WordsPerPage> &data,
                                     FlashAddress_t address);

    /**
     * Reads a specified length of bytes from a given address in FLASH memory
     * into the user-provided buffer.
     * @tparam N The number of elements in the user-provided array.
     * @param data A reference to an array to store the read data. The size of
     * the array must be large enough to accommodate the specified length in
     * bytes.
     * @param length The number of bytes to read from the flash memory. Must be
     * less than or equal to the total size of the array in bytes.
     * @param address FLASH address to be read from.
     * @return Member of the EFCError enum indicating success or an error.
     */
    template <size_t N>
    [[nodiscard]] EFCError readFromMemory(etl::array<uint32_t, N> &data,
                                          FlashReadLength_t length,
                                          FlashAddress_t address) {
        if (not isAddressSafe(address)) {
            return EFCError::ADDRESS_UNSAFE;
        }

        EFC_Read(data.data(), length, address);

        if (waitForResponse() == EFCError::TIMEOUT) {
            return EFCError::TIMEOUT;
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
    [[nodiscard]] static bool isAligned(FlashAddress_t address,
                                        uint32_t alignment) {
        return (address % alignment) == 0;
    }

    /**
     * Ensure Flash addressed used is within defined limits.
     * @param address FLASH address to be modified.
     * @return True if the address is within the limits defined.
     */
    [[nodiscard]] static bool isAddressSafe(FlashAddress_t address) {
        return address >= StartAddress && address < EndAddress;
    }

    /**
     * Function to get match internal EFC errors to the EFCError enum.
     * @return the error returned from EFC_ErrorGet as a EFCError.
     */
    [[nodiscard]] static EFCError getEFCError();

    /**
     * This function is used to erase a sector.
     * @param address FLASH address to be Erased.
     * @return member of the EFC_ERROR enum.
     */
    [[nodiscard]] static EFCError eraseSector(FlashAddress_t address);

    /**
     * Function to ensure that no calls to EFC are made while a transaction is
     * happening and to ensure the transaction doesn't get stuck.
     * @return Timeout if the transaction got stuck, None otherwise.
     */
    [[nodiscard]] static EFCError waitForResponse();
};
