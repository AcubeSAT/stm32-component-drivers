#pragma once

#include "SMC.hpp"
#include "etl/span.h"
#include "etl/array.h"

/**
 * Error codes for MRAM operations
 */
enum class MRAMError : uint8_t {
    NONE = 0, ///< Operation completed successfully
    TIMEOUT = 1, ///< Operation timed out
    ADDRESS_OUT_OF_BOUNDS = 2, ///< Attempted to access invalid address
    READY = 3, ///< Device is ready for operation
    NOT_READY = 4, ///< Device is not ready for operation
    INVALID_ARGUMENT = 5, ///< Invalid argument provided
    DATA_MISMATCH = 6 ///< Unexpected read value
};

/**
 * Driver for MR4A08BUYS45 MRAM device.
 * This class provides an interface to the MRAM device which has 2,097,152 (2^21) words of 8 bits each.
 * The device can store data from address 0 to 2^21-1.
 * 
 * This class inherits from SMC to utilize the underlying Static Memory Controller interface.
 * 
 * @see https://gr.mouser.com/datasheet/MR4A08B_Datasheet-1511665.pdf
 */
class MRAM final : public SMC {
public:
    /**
     * Initializes the MRAM device.
     * @param chipSelect Chip Select signal used for this device
     */
    explicit MRAM(ChipSelect chipSelect) : SMC(chipSelect) {
    }

    /**
     * Writes a single byte to the specified address.
     * @param dataAddress Address to write to
     * @param data Byte to write
     * @return NONE if successful, error code otherwise
     */
    MRAMError mramWriteByte(uint32_t dataAddress, uint8_t data);

    /**
     * Reads a single byte from the specified address.
     * @param dataAddress Address to read from
     * @param[out] data Buffer to store read byte
     * @return NONE if successful, error code otherwise
     */
    MRAMError mramReadByte(uint32_t dataAddress, uint8_t &data);

    /**
     * Writes multiple bytes starting at the specified address.
     * @param startAddress Starting address for write operation
     * @param data Span containing bytes to write
     * @return NONE if successful, error code otherwise
     */
    MRAMError mramWriteData(uint32_t startAddress, etl::span<const uint8_t> data);

    /**
     * Reads multiple bytes starting at the specified address.
     * @param startAddress Starting address for read operation
     * @param[out] data Span to store read bytes
     * @return NONE if successful, error code otherwise
     */
    MRAMError mramReadData(uint32_t startAddress, etl::span<uint8_t> data);

    /**
     * Handles errors that occur during device operations.
     * @param error The error code to be handled
     */
    void errorHandler(MRAMError error);

    /**
     * Checks if the device is responding correctly.
     * @return READY if device is working correctly, error code otherwise
     */
    MRAMError isMRAMAlive();

private:
    /// Size of the device identification signature in bytes
    static constexpr uint8_t CustomIDSize = 4;

    /// Device identification signature used to verify correct device
    static constexpr uint8_t CustomID[CustomIDSize] = {0xDE, 0xAD, 0xBE, 0xEF};

    /// Maximum valid address (2^21-1)
    static constexpr uint32_t MaxAllowedAddress = 0x1FFFFF;

    /// Address where device ID is stored
    static constexpr uint32_t CustomMRAMIDAddress = MaxAllowedAddress - CustomIDSize;

    /// Maximum address available for user data
    static constexpr uint32_t MaxWriteableAddress = CustomMRAMIDAddress - 1;

    /// Word size in bits
    static constexpr uint8_t WordSizeBits = 8;

    /**
     * Writes the custom identification signature to the device.
     * This should typically only be called once during device initialization.
     */
    void writeID();

    /**
     * Verifies the device identification signature.
     * @param idArray Span containing the ID to verify
     * @return true if ID matches expected value, false otherwise
     * @pre idArray must be of size CustomIDSize
     */
    bool checkID(etl::span<const uint8_t> idArray) const;

    /**
     * Validates if an address range is within bounds
     * @param startAddress Starting address of range
     * @param size Size of range in bytes
     * @return true if range is valid, false otherwise
     */
    bool isAddressRangeValid(uint32_t startAddress, size_t size) const;
};
