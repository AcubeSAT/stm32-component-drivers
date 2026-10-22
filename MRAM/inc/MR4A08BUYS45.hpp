#pragma once

#include "SMC.hpp"
#include "xdmac/plib_xdmac.h"
#include <cstdint>
#include <cstddef>

enum class MRAMError : uint8_t {
    NONE = 0,
    TIMEOUT = 1,
    ADDRESS_OUT_OF_BOUNDS = 2,
    READY = 3,
    NOT_READY = 4,
    INVALID_ARGUMENT = 5,
    DATA_MISMATCH = 6,
    DMA_IN_PROGRESS = 7,
    DMA_TRANSFER_ERROR = 8,
};

/**
 * Hard-coded MRAM memory map, per ASAT_DDJF_OBSW Section 4.2.4 ("OBC MRAM"), Figure 9.
 * Region sizes sum to 2048 KiB, matching the MR4A08BUYS45's full capacity.
 */
namespace MRAMLayout {
    constexpr uint32_t ParameterPoolAddress      = 0x000000;
    constexpr size_t    ParameterPoolSize         = 32 * 1024;

    constexpr uint32_t ServiceConfigStateAddress = 0x008000;
    constexpr size_t    ServiceConfigStateSize    = 32 * 1024;

    constexpr uint32_t CriticalVariablesAddress  = 0x010000;
    constexpr size_t    CriticalVariablesSize     = 8 * 1024;

    constexpr uint32_t PatchUncompressedAddress  = 0x012000;
    constexpr size_t    PatchUncompressedSize     = 1016 * 1024;

    constexpr uint32_t PatchCompressedAddress    = 0x110000;
    constexpr size_t    PatchCompressedSize       = 960 * 1024;

    constexpr uint32_t TotalBytes =
        ParameterPoolSize + ServiceConfigStateSize + CriticalVariablesSize +
        PatchUncompressedSize + PatchCompressedSize;

    static_assert(TotalBytes == 2u * 1024u * 1024u,
                  "MRAM layout regions must sum to the device's full 2 MiB capacity");
    static_assert(PatchCompressedAddress + PatchCompressedSize - 1 == 0x1FFFFF,
                  "Layout must end exactly at the device's top address");
}

/**
 * Driver for MR4A08BUYS45 MRAM device (2,097,152 bytes, x8).
 *
 * Access to MRAM data is done by the caller
 * directly, via pointers/references into this device's mapped address range
 * (or, preferably, via linker-defined variables placed in MRAM) — not
 * through this class. This class owns only:
 *   - this chip select's SMC timing configuration,
 *   - this device's address bounds,
 *   - an async bulk copy (RAM -> MRAM) via DMA
 *
 * Errors are reported directly via MRAMError return values
 */
class MRAM final : public SMC {
public:
    explicit MRAM(ChipSelect chipSelect)
        : SMC(chipSelect), chipSelectNumber(static_cast<uint8_t>(chipSelect)) {
        configureSmcTiming();
        configureDmaChannel();
    }

    /**
     * Starts an asynchronous copy from a RAM buffer into MRAM. Returns
     * immediately; does not block the CPU while the transfer runs.
     * @param dest  Destination address, must fall within this device's
     *              mapped, writeable range.
     * @param src   Source buffer in RAM.
     * @param count Number of bytes to copy.
     * @return NONE if the transfer was started, an error otherwise.
     */
    MRAMError startCopyFromRAM(void *dest, const void *src, size_t count);

    /**
     * Non-blocking poll for the status of the last transfer started with
     * startCopyFromRAM().
     * @return DMA_IN_PROGRESS while running, NONE on success,
     *         DMA_TRANSFER_ERROR if the channel reported a bus/overflow
     *         error.
     */
    MRAMError checkDMACompletion();

    /**
     * Checks if the device is responding correctly. Performs a small,
     * direct (non-DMA) read/write at a fixed internal address
     * @return READY if device is working correctly, error code otherwise.
     */
    MRAMError isMRAMAlive();

private:
    /// This instance's chip select number (0-3)
    const uint8_t chipSelectNumber;

    /// XDMAC channel reserved for RAM -> MRAM bulk transfers. Channel 2 is
    /// configured for memory-to-memory transfers in plib_xdmac.c's
    /// XDMAC_Initialize(); channels 0-1 remain reserved for UART.
    static constexpr XDMAC_CHANNEL DmaChannel = XDMAC_CHANNEL_2;

    static constexpr uint8_t CustomIDSize = 4;
    static constexpr uint8_t CustomID[CustomIDSize] = {0xDE, 0xAD, 0xBE, 0xEF};
    static constexpr uint32_t MaxAllowedAddress = 0x1FFFFF;
    static constexpr uint32_t CustomMRAMIDAddress = MaxAllowedAddress - CustomIDSize;
    static constexpr uint32_t MaxWriteableAddress = CustomMRAMIDAddress - 1;

    /// Validates that [startAddress, startAddress+size) lies within this
    /// device's writeable range. Runs before every DMA transfer is started.
    bool isAddressRangeValid(uint32_t startAddress, size_t size) const;

    /// Programs SMC_SETUP/PULSE/CYCLE/MODE for this chip select from the
    /// MR4A08BUYS45's AC timing table
    void configureSmcTiming();

    /// Configures XDMAC channel 2 for memory-to-memory transfers, and
    /// registers this instance's completion callback.
    void configureDmaChannel();

    /// Set by the DMA completion callback; read by checkDMACompletion().
    static volatile XDMAC_TRANSFER_EVENT lastTransferStatus;
    static volatile bool transferPending;
};