#pragma once

#include "NANDFlash.hpp"
#include "NandEcc.hpp"
#include <cstddef>
#include <cstdint>
#include <etl/array.h>
#include <etl/delegate.h>
#include <etl/expected.h>
#include <etl/span.h>
#include <etl/vector.h>

/**
 * @brief Error codes for NandBlockDevice operations.
 */
enum class BlockDeviceError : uint8_t {
    NOT_INITIALIZED = 1U,      /*!< initialize() not called or failed */
    INVALID_DIE_COUNT,         /*!< DieConfig span does not hold 1..MaxDieCount entries */
    MISMATCHED_BUS_DELEGATES,  /*!< Only one of lockBus/unlockBus is bound */
    OUT_OF_BOUNDS,             /*!< Die index exceeds the configured geometry */
    INVALID_PARAMETER,         /*!< Block index, offset, size or alignment outside the geometry (page/block operations) */
    DRIVER_ERROR,              /*!< An underlying MT29F management operation (initialize/reset/bad block lookup) failed */
    IO,                        /*!< Transient bus/driver fault with no device state change (pre-command timeout, read failure) */
    BAD_BLOCK,                 /*!< Program/erase failed or timed out after confirm or erase of a known bad block */
    UNCORRECTABLE,             /*!< The page failed ECC correction  */
    INCOMPATIBLE_LAYOUT,       /*!< The page has a valid spare area layout version of another ECC implementation */
};

/**
 * @brief Per die hardware configuration for NandBlockDevice.
 */
struct DieConfig {
    SMC::ChipSelect chipSelect;
    PIO_PIN readyBusyPin;
    PIO_PIN writeProtectPin;
};

/**
 * @brief Type alias for the injected bus lock/unlock delegates.
 */
using BusLockDelegate = etl::delegate<void()>;

/**
 * @brief Filesystem agnostic page/block device over 1..MaxDieCount MT29F NAND dies.
 *
 * @details Presents all dies as ONE flat array of erase blocks: logical block
 *          b maps to die b / BlocksPerDie, physical block b % BlocksPerDie.
 *          (filesystem needs to handle logical placement and bad block avoidance by 
 *          relocation on its own).
 *
 *          Per page on flash layout:
 *          - columns 0-8191: data area
 *          - column 8192: bad block marker, always programmed 0xFF
 *          - columns 8193-8527: 335B BCH parity
 *          - columns 8528-8531: spare area layout version, 4 redundant copies
 *
 * @note Thread Safety:
 *       - Bus locking is injected via lock/unlock delegates in the constructor
 *       - Serializing concurrent filesystem actions is the callers's job
 *
 * @ingroup drivers
 */
class NandBlockDevice {
public:
    static constexpr size_t MaxDieCount = 2U;
    static constexpr uint32_t BlocksPerDie = MT29F::BlocksPerLun;

    /* ============= Geometry ============= */

    static constexpr size_t PageSize = MT29F::DataBytesPerPage;     /*!< Read/program granularity */
    static constexpr size_t BlockSize = static_cast<size_t>(MT29F::PagesPerBlock) * MT29F::DataBytesPerPage; /*!< Physical erase block, spare excluded */

    /* ============= Spare area layout ============= */

    static constexpr size_t MarkerOffset = 0U;  /*!< Bad block marker */
    static constexpr size_t ParityOffset = 1U;  /*!< 335B BCH parity */
    static constexpr size_t VersionOffset = ParityOffset + NandEcc::ParityBytesPerPage; /*!< 4 redundant spare area layout version copies */
    static constexpr size_t SpareUsedBytes = VersionOffset + NandEcc::VersionCopyCount; /*!< 340B, spare bytes transferred with every page read and program */

    static_assert(NandEcc::DataBytesPerPage == MT29F::DataBytesPerPage);
    static_assert(SpareUsedBytes <= MT29F::SpareBytesPerPage);
    static_assert(BlockSize % PageSize == 0U);

    /**
     * @brief Construct the block device over the given dies.
     *
     * @param dieConfigs Per die hardware configuration.
     * @param yieldMs Yield delegate forwarded to each MT29F.
     * @param lockBus Delegate acquiring the shared memory bus lock.
     * @param unlockBus Delegate releasing the shared memory bus lock.
     */
    NandBlockDevice(etl::span<const DieConfig> dieConfigs, YieldDelegate yieldMs,
                    BusLockDelegate lockBus, BusLockDelegate unlockBus);

    NandBlockDevice(const NandBlockDevice&) = delete;
    NandBlockDevice& operator=(const NandBlockDevice&) = delete;
    NandBlockDevice(NandBlockDevice&&) = delete;
    NandBlockDevice& operator=(NandBlockDevice&&) = delete;

    ~NandBlockDevice() = default;

    /**
     * @brief Initialize every die (reset + factory bad block scan).
     *
     * @return Success (empty expected) or specific error code
     * @retval BlockDeviceError::INVALID_DIE_COUNT DieConfig span not 1..MaxDieCount
     * @retval BlockDeviceError::MISMATCHED_BUS_DELEGATES Only one bus delegate is bound
     * @retval BlockDeviceError::DRIVER_ERROR A die failed to initialize
     */
    [[nodiscard]] etl::expected<void, BlockDeviceError> initialize();

    /**
     * @brief Reset every die (MT29F RESET command).
     *
     * @return Success (empty expected) or specific error code
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::DRIVER_ERROR A die failed to reset
     */
    [[nodiscard]] etl::expected<void, BlockDeviceError> reset();

    /**
     * @brief Number of logical erase blocks.
     * 
     * @note 0 if the DieConfig span size was invalid.
     */
    [[nodiscard]] uint32_t blockCount() const {
        return static_cast<uint32_t>(dies.size()) * BlocksPerDie;
    }

    /**
     * @brief Read whole pages from a block.
     *
     * @param block Logical block < blockCount()
     * @param offset Byte offset within the block
     * @param[out] buffer Destination buffer for the data
     *
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::INVALID_PARAMETER Bounds/alignment violated
     * @retval BlockDeviceError::IO Driver read failure
     * @retval BlockDeviceError::UNCORRECTABLE A page failed ECC correction
     * @retval BlockDeviceError::INCOMPATIBLE_LAYOUT A page carries a valid spare area layout version 
     *         of another ECC version
     */
    [[nodiscard]] etl::expected<void, BlockDeviceError> readPages(uint32_t block, size_t offset, etl::span<uint8_t> buffer);

    /**
     * @brief Program whole pages of an erased block.
     *
     * @param block Logical block < blockCount()
     * @param offset Byte offset within the block
     * @param data Data to program
     *
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::INVALID_PARAMETER Bounds/alignment violated
     * @retval BlockDeviceError::BAD_BLOCK A page failed to program or timed
     *         out after confirm
     * @retval BlockDeviceError::IO Other driver failure
     */
    [[nodiscard]] etl::expected<void, BlockDeviceError> programPages(uint32_t block, size_t offset, etl::span<const uint8_t> data);

    /**
     * @brief Erase a block.
     *
     * @param block Logical block < blockCount()
     *
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::INVALID_PARAMETER Block out of range
     * @retval BlockDeviceError::BAD_BLOCK Block is known as bad or the erase
     *         failed / timed out after confirm
     * @retval BlockDeviceError::IO Other driver failure
     */
    [[nodiscard]] etl::expected<void, BlockDeviceError> eraseBlock(uint32_t block);

    /**
     * @brief Whether a block of the flat address space is marked bad (factory or runtime).
     *
     * @param block Flat block index
     *
     * @return true if the block is marked bad, or specific error code
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::INVALID_PARAMETER Block out of range
     * @retval BlockDeviceError::DRIVER_ERROR The lookup itself failed
     */
    [[nodiscard]] etl::expected<bool, BlockDeviceError> isBlockBad(uint32_t block) const;

    /**
     * @brief Number of blocks currently marked bad on one die (factory + runtime).
     *
     * @param dieIndex Die to query
     *
     * @return Bad block count or specific error code
     * @retval BlockDeviceError::NOT_INITIALIZED initialize() has not succeeded
     * @retval BlockDeviceError::OUT_OF_BOUNDS dieIndex exceeds configured dies
     */
    [[nodiscard]] etl::expected<uint32_t, BlockDeviceError> badBlockCount(size_t dieIndex) const;

    /**
     * @brief Number of page reads so far that failed with INCOMPATIBLE_LAYOUT.
     *
     * @details It never resets after initialize(). It is read before mounting.
     *          If the mount fails because the pages it read were unusable 
     *          AND the counter incremented, the flash holds a volume written 
     *          by another ECC firmware.
     */
    [[nodiscard]] uint32_t incompatibleLayoutCount() const {
        return incompatibleLayoutReads;
    }

    /**
     * @brief Spare area layout version copies of the most recently read page.
     */
    [[nodiscard]] etl::span<const uint8_t, NandEcc::VersionCopyCount> lastReadVersion() const {
        return etl::span<const uint8_t, NandEcc::VersionCopyCount>{readSpare.data() + VersionOffset,
                                                                    NandEcc::VersionCopyCount};
    }

private:
    /**
     * @brief RAII guard invoking the injected bus lock/unlock delegates.
     */
    class BusGuard {
    public:
        explicit BusGuard(const NandBlockDevice& device) : device{device} {
            device.lockBus.call_if();
        }

        ~BusGuard() {
            device.unlockBus.call_if();
        }

        BusGuard(const BusGuard&) = delete;
        BusGuard& operator=(const BusGuard&) = delete;
        BusGuard(BusGuard&&) = delete;
        BusGuard& operator=(BusGuard&&) = delete;

    private:
        const NandBlockDevice& device;
    };

    /**
     * @brief Validate block/offset/size against the geometry.
     */
    [[nodiscard]] bool isValidPageRange(uint32_t block, size_t offset, size_t size) const {
        return (block < blockCount())
               and (offset % PageSize == 0U)
               and (size % PageSize == 0U)
               and (offset < BlockSize)
               and (size <= BlockSize - offset);
    }

    /**
     * @brief The die that holds a block of the flat address space.
     */
    [[nodiscard]] MT29F& die(uint32_t block) {
        return dies[block / BlocksPerDie];
    }

    [[nodiscard]] const MT29F& die(uint32_t block) const {
        return dies[block / BlocksPerDie];
    }

    /**
     * @brief The block index within its die.
     */
    [[nodiscard]] static uint16_t localBlock(uint32_t block) {
        return static_cast<uint16_t>(block % BlocksPerDie);
    }

    /**
     * @brief The MT29F address of one page of a page aligned range.
     *
     * @param block Flat block index 
     * @param offset Byte offset of the range within the block as a multiple of PageSize
     * @param pageIndex Index of the page within the range
     */
    [[nodiscard]] static MT29F::NANDAddress pageAddress(uint32_t block, size_t offset, size_t pageIndex) {
        return MT29F::NANDAddress{0U, localBlock(block), static_cast<uint32_t>((offset / PageSize) + pageIndex), 0U};
    }

    /**
     * @brief The PageSize bytes of a page aligned buffer that belong to one page.
     *
     * @tparam Byte uint8_t for a read buffer, const uint8_t for a program buffer
     */
    template <typename Byte>
    [[nodiscard]] static etl::span<Byte, PageSize> pageSpan(etl::span<Byte> buffer, size_t pageIndex) {
        return etl::span<Byte, PageSize>{buffer.data() + (pageIndex * PageSize), PageSize};
    }

    /* ============= State ============= */

    etl::vector<MT29F, MaxDieCount> dies;               /*!< One driver per DieConfig */
    bool hasMismatchedBusDelegates;                     /*!< Only one of lockBus/unlockBus was bound at construction */
    bool isInitialized = false;
    uint32_t incompatibleLayoutReads = 0U;              /*!< Page reads that failed with INCOMPATIBLE_LAYOUT */

    BusLockDelegate lockBus;   /*!< Acquires the shared bus lock */
    BusLockDelegate unlockBus; /*!< Releases the shared bus lock */

    /* ============= Spare buffers ============= */

    etl::array<uint8_t, SpareUsedBytes> readSpare = {};  /*!< Buffer for the spare area during a read operation */
    etl::array<uint8_t, SpareUsedBytes> progSpare = {};  /*!< Buffer for the spare area during a program operation */
};
