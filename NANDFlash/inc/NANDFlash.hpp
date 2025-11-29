#pragma once

#include "SMC.hpp"
#include "definitions.h"
#include <etl/expected.h>
#include <etl/span.h>
#include <etl/array.h>

/**
 * @brief Error codes for NAND flash operations
 */
enum class NANDErrorCode : uint8_t {
    TIMEOUT = 1U,
    ADDRESS_OUT_OF_BOUNDS,
    BUSY_IO,
    BUSY_ARRAY,
    PROGRAM_FAILED,
    ERASE_FAILED,
    READ_FAILED,
    NOT_READY,
    WRITE_PROTECTED,
    BAD_BLOCK,
    INVALID_PARAMETER,
    NOT_INITIALIZED,
    HARDWARE_FAILURE,
    BAD_PARAMETER_PAGE,
};

/**
 * @brief Driver for MT29F64G08AFAAAWP NAND Flash
 *
 * @note Thread Safety:
 *       - Driver requires external synchronization
 *       - Caller must hold external mutex during all operations
 *
 *       Bad Block Management:
 *       - Driver maintains bad block table (factory + runtime discovered)
 *       - Query via isBlockBad() before operations
 *       - Driver does NOT enforce checks on read/program/erase (caller responsibility)
 *
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 * @see https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 * @see https://onfi.org/files/onfi_2_0_gold.pdf
 */
class MT29F : public SMC {
public:
    /* ============== MT29F64G08AFAAAWP device constants =============== */
    static constexpr uint32_t DataBytesPerPage = 8192U;
    static constexpr uint16_t SpareBytesPerPage = 448U;
    static constexpr uint16_t TotalBytesPerPage = DataBytesPerPage + SpareBytesPerPage;
    static constexpr uint8_t PagesPerBlock = 128U;
    static constexpr uint16_t BlocksPerLun = 4096U;
    static constexpr uint8_t LunsPerCe = 1U;

    /**
     * @brief NAND address structure
     */
    struct NANDAddress {
        uint32_t lun;
        uint32_t block;
        uint32_t page;
        uint32_t column;

        constexpr explicit NANDAddress(uint32_t lun = 0, uint32_t block = 0, uint32_t page = 0, uint32_t column = 0)
            : lun {lun}
            , block {block}
            , page {page}
            , column {column} {}
    };

    /**
     * @brief Constructor for MT29F NAND flash driver
     *
     * @param chipSelect SMC chip select for NAND flash
     * @param readyBusyPin GPIO pin for R/B# signal monitoring
     * @param writeProtectPin GPIO pin for write protect control
     */
    MT29F(ChipSelect chipSelect, PIO_PIN readyBusyPin, PIO_PIN writeProtectPin)
        : SMC { chipSelect }
        , NandReadyBusyPin { readyBusyPin }
        , NandWriteProtect { writeProtectPin } {
        selectNandConfiguration(chipSelect);
    }

    // Explicitly non-copyable and non-movable
    MT29F(const MT29F&) = delete;
    MT29F& operator=(const MT29F&) = delete;
    MT29F(MT29F&&) = delete;
    MT29F& operator=(MT29F&&) = delete;

    ~MT29F() = default;

    /* ========= Driver Initialization and Basic Info ========= */

    /**
     * @brief Initialize the NAND flash driver and verify device
     *
     * @details This function performs basic device initialization including:
     * - Device reset and ID verification
     * - ONFI compliance checking
     * - Parameter page validation against expected geometry
     * - Factory bad block scanning
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::SUCCESS Initialization completed successfully
     * @retval NANDErrorCode::HARDWARE_FAILURE Wrong device ID, ONFI failure, or geometry mismatch
     * @retval NANDErrorCode::TIMEOUT Device not responding
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> initialize();

    /**
     * @brief Reset the NAND flash device
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not responding within timeout period
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> reset();


    /* ================== Data Operations ================== */

    /**
     * @brief Read data from NAND flash page
     *
     * @param address NAND address to read from
     * @param[out] data Buffer to store read data (size determines bytes to read)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid size or column address
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::READ_FAILED Status register indicates read failure
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readPage(const NANDAddress& address, etl::span<uint8_t> data);

    /**
     * @brief Program (write) data to NAND flash page
     *
     * @note 1) Page must be in erased state before programming.
     *       2) The block marker is located at column address 8192 (BlockMarkerOffset).
     *          If the caller's data includes this address, the byte at that position
     *          MUST be 0xFF.
     *
     * @param address NAND address to write to
     * @param data Data to write (max page size)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Data size exceeds available space, or
     *                                          invalid block marker value at column 8192
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::PROGRAM_FAILED Status register indicates program failure
     *
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> programPage(const NANDAddress& address, etl::span<const uint8_t> data);

    /**
     * @brief Erase a block
     *
     * @note If erase fails, the block is automatically marked as bad in the runtime table.
     *
     * @param block Block number to erase
     * @param lun LUN number (typically 0)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Block or LUN out of bounds
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::ERASE_FAILED Status register indicates erase failure (block marked as bad)
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> eraseBlock(uint16_t block, uint8_t lun = 0);


    /* ==================== Bad Block Management ==================== */

    /**
     * @brief Check if a block is marked as bad
     *
     * @param block Block number to check
     * @param lun LUN number (typically 0)
     *
     * @return true if block is bad, false if good
     */
    [[nodiscard]] bool isBlockBad(uint16_t block, uint8_t lun = 0) const;

private:
    /* ============= ONFI Protocol Definitions ============= */

    /**
     * @brief NAND Commands
     */
    enum class Commands : uint8_t {
        RESET = 0xFFU,
        READID = 0x90U,
        READ_PARAM_PAGE = 0xECU,
        READ_UNIQ_ID = 0xEDU,
        READ_STATUS = 0x70U,
        ERASE_BLOCK = 0x60U,
        ERASE_BLOCK_CONFIRM = 0xD0U,
        READ_MODE = 0x00U,
        READ_CONFIRM = 0x30U,
        PAGE_PROGRAM = 0x80U,
        PAGE_PROGRAM_CONFIRM = 0x10U,
        CHANGE_WRITE_COLUMN = 0x85U,
        CHANGE_READ_COLUMN = 0x05U,
        CHANGE_READ_COLUMN_CONFIRM = 0xE0U,
    };

    /**
     * @brief Address parameters for Read ID command
     */
    enum class ReadIDAddress : uint8_t {
        MANUFACTURER_ID = 0x00U,
        ONFI_SIGNATURE = 0x20U,
    };

    static constexpr uint8_t StatusFail = 0x01U;    /*!< Program/Erase operation failed */

    static constexpr uint8_t StatusFailc = 0x02U;   /*!< Command failed */

    static constexpr uint8_t StatusArdy = 0x20U;    /*!< Array ready */

    static constexpr uint8_t StatusRdy = 0x40U;     /*!< Device ready */

    static constexpr uint8_t StatusWp = 0x80U;      /*!< Write protected (1 = not protected, 0 = protected) */


    /* ============= Device Specifications ============= */

    static constexpr etl::array<uint8_t, 5> ExpectedDeviceId = { 0x2CU, 0x68U, 0x00U, 0x27U, 0xA9U }; /*!< Expected device ID for MT29F64G08AFAAAWP */

    static constexpr uint32_t GpioSettleTimeNs = 100U; /*!< WP# GPIO settling time */

    /*
        Calculated based on the datasheet.
    */
    static constexpr uint32_t TwhrNs = 120U;   /*!< tWHR: Command/address to data read */
    static constexpr uint32_t TadlNs = 200U;   /*!< tADL: Address to data input */
    static constexpr uint32_t TrhwNs = 200U;   /*!< tRHW/tRHZ: Read to write turnaround */
    static constexpr uint32_t TrrNs = 40U;     /*!< tRR: R/B# ready to first read access */
    static constexpr uint32_t TwbNs = 200U;    /*!< tWB: Command to busy transition */

    /*
        Calculated based on the ONFI table of the datasheet. For safety reasons they are ~5 times what the datasheet says.
        Also for practical reasons (easier calculations) the read timeout was put to 1ms.
    */
    static constexpr uint32_t TimeoutReadMs = 1U;       /*!< Timeout for READ operation (35us max from datasheet) */
    static constexpr uint32_t TimeoutProgramMs = 3U;    /*!< Timeout for RPROGRAM operation (560us max from datasheet) */
    static constexpr uint32_t TimeoutEraseMs = 35U;     /*!< Timeout for ERASE operation (7ms max from datasheet) */
    static constexpr uint32_t TimeoutResetMs = 5U;      /*!< Timeout for RESET operation (1ms max from datasheet) */

    /**
     * @brief Type alias for 5-cycle NAND addressing
     *
     * @note Represents the 5 address cycles required for NAND operations:
     *       [CA1, CA2, RA1, RA2, RA3]
     */
    using AddressCycles = etl::array<uint8_t, 5>;

    /**
     * @brief Address cycle indices for 5-cycle NAND addressing
     */
    enum AddressCycle : uint8_t {
        CA1,    /*!< Column address byte 1 [CA1]: Column[7:0] */
        CA2,    /*!< Column address byte 2 [CA2]: Column[15:8] */
        RA1,    /*!< Row address byte 1 [RA1]: Page[6:0] | Block[0] */
        RA2,    /*!< Row address byte 2 [RA2]: Block[8:1] */
        RA3,    /*!< Row address byte 3 [RA3]: LUN[0] | Block[11:9] */
    };


    /* ============= Bad Block Management ============= */

    /**
     * @brief Internal bad block information structure
     */
    struct BadBlockInfo {
        uint16_t blockNumber;
        uint8_t lun;
    };

    size_t badBlockCount = 0;   /*!< Current number of bad blocks in table */

    static constexpr size_t MaxBadBlocks = 512U;                /*!< Maximum number of bad blocks to track */

    static constexpr uint16_t BlockMarkerOffset = 8192U;        /*!< Offset to bad block marker in spare area */

    static constexpr uint8_t GoodBlockMarker = 0xFFU;           /*!< Marker value for good blocks */

    static constexpr uint8_t BadBlockMarker = 0x00U;            /*!< Marker value for bad blocks */

    etl::array<BadBlockInfo, MaxBadBlocks> badBlockTable = {};  /*!< Table of known bad blocks */


    /**
     * @brief Read block marker from spare area
     * 
     * @details Based on the datasheet the bad block marker is guaranteed to be stored 
     *          in the first page of each block in byte 0 of the spare area
     *
     * @param block Block number to check
     * @param lun LUN number (default 0)
     *
     * @return Block marker byte (0xFF = good, 0x00 = bad) or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid size or column address
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::READ_FAILED Status register indicates read failure
     */
    etl::expected<uint8_t, NANDErrorCode> readBlockMarker(uint16_t block, uint8_t lun = 0);

    /**
     * @brief Scan all blocks in a LUN for factory bad block markers
     *
     * @param lun LUN number to scan (default 0)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS LUN out of bounds
     * @retval NANDErrorCode::HARDWARE_FAILURE Too many bad blocks (>256)
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized (from readBlockMarker)
     * @retval NANDErrorCode::TIMEOUT Device not ready (from readBlockMarker)
     * @retval NANDErrorCode::READ_FAILED Read operation failed (from readBlockMarker)
     */
    etl::expected<void, NANDErrorCode> scanFactoryBadBlocks(uint8_t lun = 0);

    /**
     * @brief Mark a block as bad
     *
     * @param block Block number to mark as bad
     * @param lun LUN number (typically 0)
     */
    void markBadBlock(uint16_t block, uint8_t lun = 0);


    /* ============= Write Protection ============= */

    /**
     * @brief RAII guard for write-enable state
     */
    class WriteEnableGuard {
    private:
        MT29F& nand;
    public:
        explicit WriteEnableGuard(MT29F& n) : nand(n) {
            nand.enableWrites();
        }

        ~WriteEnableGuard() {
            nand.disableWrites();
        }

        // Non-copyable, non-movable
        WriteEnableGuard(const WriteEnableGuard&) = delete;
        WriteEnableGuard& operator=(const WriteEnableGuard&) = delete;
        WriteEnableGuard(WriteEnableGuard&&) = delete;
        WriteEnableGuard& operator=(WriteEnableGuard&&) = delete;
    };

    /**
     * @brief Enable writes by deasserting WP# pin
     */
    void enableWrites() const;

    /**
     * @brief Disable writes by asserting WP# pin
     */
    void disableWrites() const;


    /* ============= Hardware Interface ============= */

    const uint32_t TriggerNANDALEAddress = moduleBaseAddress | 0x200000U; /*!< SMC address for triggering ALE (Address Latch Enable) */

    const uint32_t TriggerNANDCLEAddress = moduleBaseAddress | 0x400000U; /*!< SMC address for triggering CLE (Command Latch Enable) */

    const PIO_PIN NandReadyBusyPin; /*!< GPIO pin for monitoring R/B# (Ready/Busy) signal */

    const PIO_PIN NandWriteProtect; /*!< GPIO pin for controlling WP# (Write Protect) signal */

    /**
     * @brief Send data byte to NAND flash
     *
     * @param data Data byte to send
     */
    void sendData(uint8_t data) {
        smcWriteByte(moduleBaseAddress, data);
    }

    /**
     * @brief Send address byte to NAND flash (triggers ALE)
     *
     * @param address Address byte to send
     */
    void sendAddress(uint8_t address) {
        smcWriteByte(TriggerNANDALEAddress, address);
    }

    /**
     * @brief Send command to NAND flash (triggers CLE)
     *
     * @param command NAND command to send
     */
    void sendCommand(Commands command) {
        smcWriteByte(TriggerNANDCLEAddress, static_cast<uint8_t>(command));
    }

    /**
     * @brief Read data byte from NAND flash
     *
     * @return Data byte read from device
     */
    uint8_t readData() {
        return smcReadByte(moduleBaseAddress);
    }


    /* ============= State Management ============= */

    bool isInitialized = false; /*!< Driver initialization status */


    /* ============= Command Sequences ============= */

    /**
     * @brief Execute NAND read command sequence without initialization check
     *
     * @param address NAND address to read from
     *
     * @return Success or error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     */
    etl::expected<void, NANDErrorCode> executeReadCommandSequence(const NANDAddress& address);


    /* ============= Device Identification and Validation ============= */

    /**
     * @brief Read device manufacturer and device ID
     *
     * @param[out] id Buffer to store 5-byte device ID
     */
    void readDeviceID(etl::span<uint8_t, 5> id);

    /**
     * @brief Read ONFI signature to verify ONFI compliance
     *
     * @param[out] signature Buffer to store 4-byte ONFI signature
     */
    void readONFISignature(etl::span<uint8_t, 4> signature);

    /**
     * @brief Validate parameter page CRC-16 checksum
     *
     * @details Implements ONFI CRC-16 algorithm with polynomial 0x8005.
     *          Initial value is 0x4F4E.
     *          Validates first 254 bytes against stored CRC in bytes 254-255.
     *
     * @param paramPage 256-byte parameter page data
     *
     * @return true if CRC matches, false if invalid
     */
    static bool validateParameterPageCRC(const etl::array<uint8_t, 256>& paramPage);

    /**
     * @brief Validate device parameters match expected geometry
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::HARDWARE_FAILURE Device geometry mismatch or ONFI signature invalid
     * @retval NANDErrorCode::BAD_PARAMETER_PAGE All 3 parameter page copies have invalid CRC
     */
    etl::expected<void, NANDErrorCode> validateDeviceParameters();


    /* ============= Address and Status Utilities ============= */

    /**
     * @brief Build 5-cycle address sequence for NAND Device
     *
     * @note Converts NAND address structure to hardware address cycles:
     *       - Cycle 1 (CA1): Column[7:0]
     *       - Cycle 2 (CA2): Column[15:8] (only bits [5:0] used)
     *       - Cycle 3 (RA1): Page[6:0] | Block[0]
     *       - Cycle 4 (RA2): Block[8:1]
     *       - Cycle 5 (RA3): LUN[0] | Block[11:9]
     *
     * @param address NAND address structure
     * @param[out] cycles Generated address cycles
     */
    static void buildAddressCycles(const NANDAddress& address, AddressCycles& cycles);

    /**
     * @brief Validate NAND address bounds
     *
     * @param address Address to validate
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS LUN, block, page, or column out of bounds
     */
    static etl::expected<void, NANDErrorCode> validateAddress(const NANDAddress& address);

    /**
     * @brief Read NAND status register
     *
     * @return Status register value or error code
     */
    uint8_t readStatusRegister();

    /**
     * @brief Wait for NAND device to become ready
     *
     * @param timeoutMs Timeout in milliseconds
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout period
     */
    etl::expected<void, NANDErrorCode> waitForReady(uint32_t timeoutMs);


    /* ============= Timing Utilities ============= */

    /**
     * @brief Busy-wait delay for nanosecond-precision timing
     *
     * @param nanoseconds Delay duration in nanoseconds
     *
     * @note Uses CPU clock (CPU_CLOCK_FREQUENCY in Hz) for calibration.
     */
    static void busyWaitNanoseconds(uint32_t nanoseconds);


    /* ============= Hardware Configuration ============= */

    /**
     * @brief Configure SMC for NAND flash operation
     *
     * @param chipSelect SMC chip select to configure
     */
    static void selectNandConfiguration(ChipSelect chipSelect);
};


/* ==================== Debug ==================== */

/**
 * @brief Convert error code to human-readable string
 *
 * @note Used for logging
 *
 * @param error Error code to convert
 *
 * @return String description of error
 */
const char* toString(NANDErrorCode error);
