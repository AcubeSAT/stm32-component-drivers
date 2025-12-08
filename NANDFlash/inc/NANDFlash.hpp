#pragma once

#include "SMC.hpp"
#include "definitions.h"
#include <etl/expected.h>
#include <etl/span.h>
#include <etl/array.h>
#include <etl/delegate.h>

/**
 * @brief Error codes for NAND flash operations
 */
enum class NANDErrorCode : uint8_t {
    TIMEOUT = 1U,           /*!< Operation timed out waiting for device ready */
    ADDRESS_OUT_OF_BOUNDS,  /*!< LUN, block, page, or column address exceeds device limits */
    DEVICE_BUSY,            /*!< Device busy (RDY=0 or ARDY=0) */
    PROGRAM_FAILED,         /*!< Program operation failed (FAIL or FAILC bit set in status) */
    ERASE_FAILED,           /*!< Erase operation failed (FAIL or FAILC bit set in status) */
    WRITE_PROTECTED,        /*!< Device is write-protected (WP# asserted or status WP bit clear) */
    INVALID_PARAMETER,      /*!< Invalid parameter (size exceeds page, invalid block marker value) */
    NOT_INITIALIZED,        /*!< Driver not initialized */
    HARDWARE_FAILURE,       /*!< Hardware failure (ID mismatch, geometry mismatch, bad block table full) */
    BAD_PARAMETER_PAGE,     /*!< All ONFI parameter page copies have invalid CRC */
};

/**
 * @brief Type alias for yield delegate
 *
 * Callable that yields to OS scheduler for given milliseconds.
 * Used for long operations where other tasks should run and not starvate.
 *
 * @note Bind to wrapper calling vTaskDelay(pdMS_TO_TICKS(ms))
 */
using YieldDelegate = etl::delegate<void(uint32_t)>;

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
 *       - Caller must mark the runtime bad blocks via markBadBlock()
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
    static constexpr uint32_t TotalBytesPerPage = DataBytesPerPage + SpareBytesPerPage;
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
            : lun{lun}
            , block{block}
            , page{page}
            , column{column} {}
    };

    /**
     * @brief Constructor for MT29F NAND flash driver
     *
     * @param chipSelect SMC chip select for NAND flash
     * @param readyBusyPin GPIO pin for R/B# signal monitoring
     * @param writeProtectPin GPIO pin for write protect control
     * @param yieldMs Delegate for yielding to OS during long operations
     */
    MT29F(ChipSelect chipSelect, PIO_PIN readyBusyPin, PIO_PIN writeProtectPin, YieldDelegate yieldMs)
        : SMC{chipSelect}
        , NandReadyBusyPin{readyBusyPin}
        , NandWriteProtect{writeProtectPin}
        , yieldMilliseconds{yieldMs} {
        enableNandFlashMode(chipSelect);
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
     *          - Device reset and ID verification
     *          - ONFI compliance checking
     *          - Parameter page validation against expected geometry
     *          - Factory bad block scanning
     *          - Enables the write protection if the user has provided a WP# pin
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::HARDWARE_FAILURE Wrong device ID, ONFI failure, or geometry mismatch
     * @retval NANDErrorCode::TIMEOUT Device not responding
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> initialize();

    /**
     * @brief Reset the NAND flash device
     *
     * @return Success (empty expected) or specific error code
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
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid size or column address
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::DEVICE_BUSY Device busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readPage(const NANDAddress& address, etl::span<uint8_t> data);

    /**
     * @brief Program (write) data to NAND flash page
     *
     * @warning 1) Page must be in erased state before programming
     *          2) The block marker is located at column address 8192 (BlockMarkerOffset)
     *             If the caller's data includes this address, the byte at that position
     *             MUST be 0xFF.
     *
     * @param address NAND address to write to
     * @param data Data to write (max page size)
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Data size exceeds available space, or invalid block marker value at column 8192
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::DEVICE_BUSY Device busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::PROGRAM_FAILED Status register indicates program failure
     *
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> programPage(const NANDAddress& address, etl::span<const uint8_t> data);

    /**
     * @brief Erase a block
     *
     * @param block Block number to erase
     * @param lun LUN number (typically 0)
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Block or LUN out of bounds
     * @retval NANDErrorCode::DEVICE_BUSY Device busy
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
     * @retval true if block is bad
     * @retval false if good
     */
    [[nodiscard]] bool isBlockBad(uint16_t block, uint8_t lun = 0) const;

    /**
     * @brief Mark a block as bad in the runtime table
     *
     * @note Call this when program/erase operations fail to track bad blocks.
     * 
     * @warning The driver does not auto-mark blocks. The caller is responsible for
     *          bad block policy decisions.
     *
     * @param block Block number to mark as bad
     * @param lun LUN number (typically 0)
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::HARDWARE_FAILURE Bad block table is full
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> markBadBlock(uint16_t block, uint8_t lun = 0);


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
    };

    /**
     * @brief Address parameters for Read ID command
     */
    enum class ReadIDAddress : uint8_t {
        MANUFACTURER_ID = 0x00U,
        ONFI_SIGNATURE = 0x20U,
    };

    static constexpr uint8_t StatusFail = 0x01U;           /*!< Program/Erase operation failed */

    static constexpr uint8_t StatusFailCommand = 0x02U;   /*!< Program/Erase command failed */

    static constexpr uint8_t StatusArrayReady = 0x20U;    /*!< Array ready */

    static constexpr uint8_t StatusReady = 0x40U;         /*!< Device ready */

    static constexpr uint8_t StatusWriteProtect = 0x80U;  /*!< Write protected (1 = not protected, 0 = protected) */


    /* ============= Device Specifications ============= */

    static constexpr etl::array<uint8_t, 5> ExpectedDeviceId = { 0x2CU, 0x68U, 0x00U, 0x27U, 0xA9U }; /*!< Expected device ID for MT29F64G08AFAAAWP */

    static constexpr uint32_t GpioSettleTimeNs = 100U; /*!< WP# GPIO settling time */

    /*
        NAND timing constants (Timing Mode 0).
        Calculated based on the datasheet.
    */
    static constexpr uint32_t TwhrNs = 120U;   /*!< tWHR: Command/address to data read */
    
    static constexpr uint32_t TadlNs = 200U;   /*!< tADL: Address to data input */

    static constexpr uint32_t TrhwNs = 200U;   /*!< tRHW/tRHZ: Read to write turnaround */

    static constexpr uint32_t TrrNs = 40U;     /*!< tRR: R/B# ready to first read access */

    static constexpr uint32_t TwbNs = 200U;    /*!< tWB: Command to busy transition */

    /*
        Calculated based on the ONFI table of the datasheet.
        For safety reasons they are ~5 times what the datasheet says.
    */
    static constexpr uint32_t TimeoutReadUs = 200U;      /*!< Timeout for READ operation (35us max from datasheet) */

    static constexpr uint32_t TimeoutProgramUs = 3000U;  /*!< Timeout for PROGRAM operation (560us max from datasheet) */

    static constexpr uint32_t TimeoutEraseUs = 35000U;   /*!< Timeout for ERASE operation (7ms max from datasheet) */

    static constexpr uint32_t TimeoutResetUs = 5000U;    /*!< Timeout for RESET operation (1ms max from datasheet) */

    /**
     * @brief Type alias for 5-cycle NAND addressing
     *
     * @note Represents the 5 address cycles required for NAND operations:
     *       [COLUMN_ADDRESS_1, COLUMN_ADDRESS_2, ROW_ADDRESS_1, ROW_ADDRESS_2, ROW_ADDRESS_3]
     */
    using AddressCycles = etl::array<uint8_t, 5>;

    /**
     * @brief Address cycle indices for 5-cycle NAND addressing
     */
    enum AddressCycle : uint8_t {
        COLUMN_ADDRESS_1,    /*!< Column address byte 1: Column[7:0] */
        COLUMN_ADDRESS_2,    /*!< Column address byte 2: Column[15:8] */
        ROW_ADDRESS_1,       /*!< Row address byte 1: Page[6:0] | Block[0] */
        ROW_ADDRESS_2,       /*!< Row address byte 2: Block[8:1] */
        ROW_ADDRESS_3,       /*!< Row address byte 3: LUN[0] | Block[11:9] */
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
     * @param block Block number to check
     * @param lun LUN number (default 0)
     *
     * @return Block marker byte (0xFF = good, 0x00 = bad) or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     */
    [[nodiscard]] etl::expected<uint8_t, NANDErrorCode> readBlockMarker(uint16_t block, uint8_t lun = 0);

    /**
     * @brief Scan all blocks in a LUN for factory bad block markers
     *
     * @details Based on the datasheet the bad block marker is guaranteed to be stored
     *          in the first page of each block in byte 0 of the spare area.
     *          Fails immediately on any read error.
     *          Yields to other tasks periodically during the scan.
     *
     * @param lun LUN number to scan (default 0)
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS LUN out of bounds
     * @retval NANDErrorCode::HARDWARE_FAILURE Too many bad blocks (bad block table full)
     * @retval NANDErrorCode::TIMEOUT Block marker read timed out
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> scanFactoryBadBlocks(uint8_t lun = 0);


    /* ============= Write Protection ============= */

    /**
     * @brief RAII guard for write-enable state
     */
    class WriteEnableGuard {
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

    private:
        MT29F& nand;
    };

    /**
     * @brief Enable writes by deasserting WP# pin
     */
    void enableWrites();

    /**
     * @brief Disable writes by asserting WP# pin
     */
    void disableWrites();


    /* ============= Hardware Interface ============= */

    static constexpr uint32_t SmcAleTriggerOffset = 0x200000U;  /*!< Offset of ALE (Address Latch Enable) */
    
    static constexpr uint32_t SmcCleTriggerOffset = 0x400000U;  /*!< Offset of CLE (Command Latch Enable) */

    const uint32_t TriggerNANDALEAddress = moduleBaseAddress | SmcAleTriggerOffset; /*!< SMC address for triggering ALE */
    
    const uint32_t TriggerNANDCLEAddress = moduleBaseAddress | SmcCleTriggerOffset; /*!< SMC address for triggering CLE */

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
     * @return Success (empty expected) or error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> executeReadCommandSequence(const NANDAddress& address);


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
     * @param parameterPage 256-byte parameter page data
     *
     * @retval true if CRC matches
     * @retval false if invalid
     */
    static bool validateParameterPageCRC(etl::span<const uint8_t, 256> parametrPage);

    /**
     * @brief Validate device parameters match expected geometry
     * 
     * @details 1) ONFI requires redundant copies of the parameter page. 
     *             This device has 3 copies. We try each copy until we find a valid one.
     *          2) The values of the parameter page are stored in little-endian
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::HARDWARE_FAILURE Device geometry mismatch or ONFI signature invalid
     * @retval NANDErrorCode::BAD_PARAMETER_PAGE All 3 parameter page copies have invalid CRC
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> validateDeviceParameters();


    /* ============= Address and Status Utilities ============= */

    /**
     * @brief Build 5-cycle address sequence for NAND Device
     *
     * @note Converts NAND address structure to hardware address cycles:
     *       - Cycle 1 (COLUMN_ADDRESS_1): Column[7:0]
     *       - Cycle 2 (COLUMN_ADDRESS_2): Column[15:8] (only bits [5:0] used)
     *       - Cycle 3 (ROW_ADDRESS_1): Page[6:0] | Block[0]
     *       - Cycle 4 (ROW_ADDRESS_2): Block[8:1]
     *       - Cycle 5 (ROW_ADDRESS_3): LUN[0] | Block[11:9]
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
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS LUN, block, page, or column out of bounds
     */
    [[nodiscard]] static etl::expected<void, NANDErrorCode> validateAddress(const NANDAddress& address);

    /**
     * @brief Read NAND status register
     *
     * @return Status register value
     */
    uint8_t readStatusRegister();

    /**
     * @brief Check if device is ready (both I/O interface and flash array)
     *
     * @param status Status register value
     *
     * @retval true if both RDY and ARDY bits are set
     * @retval false otherwise
     */
    [[nodiscard]] static bool isReady(uint8_t status) {
        return (status & (StatusReady | StatusArrayReady)) == (StatusReady | StatusArrayReady);
    }

    /**
     * @brief Check if device is write protected
     *
     * @param status Status register value
     * 
     * @retval true if WP bit is clear (protected)
     * @retval false otherwise
     */
    [[nodiscard]] static bool isWriteProtected(uint8_t status) {
        return (status & StatusWriteProtect) == 0;
    }

    /**
     * @brief Check if last operation failed
     *
     * @note This is only valid for Program/Erase operations
     * 
     * @param status Status register value
     * 
     * @retval true if FAIL or FAILC bits are set
     * @retval false otherwise
     */
    [[nodiscard]] static bool hasOperationFailed(uint8_t status) {
        return (status & (StatusFail | StatusFailCommand)) != 0;
    }

    /**
     * @brief Check if device is ready for operations
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::DEVICE_BUSY Device is busy (RDY=0 or ARDY=0)
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> ensureDeviceReady();

    /**
     * @brief Verify write protection is disabled
     *
     * @return Success (empty expected) or specific error code
     * @retval NANDErrorCode::WRITE_PROTECTED WP# is asserted (status WP bit clear)
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> verifyWriteEnabled();


    /* ============= Wait Policy ============= */

    static constexpr uint32_t BusyWaitThresholdUs = 1000U;   /*!< Threshold for switching from busy-wait to OS yield (1ms) */

    static constexpr uint32_t BusyWaitPollIntervalUs = 5U;   /*!< Polling interval during busy-wait (5µs) */

    static constexpr uint32_t YieldIntervalMs = 1U;          /*!< Yield interval during hybrid wait (1ms) */

    YieldDelegate yieldMilliseconds; /*!< Delegate for yielding to OS during long operations */

    /**
     * @brief Wait for NAND device to become ready
     *
     * @details Uses hybrid busy-wait/yield approach:
     *          - If timeout <= BusyWaitThresholdUs (1ms): pure busy-wait with 5µs polling
     *          - If timeout > BusyWaitThresholdUs: busy-wait first 1ms, then yield 1ms intervals
     *
     * @param timeoutUs Timeout in microseconds
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout period
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> waitForReady(uint32_t timeoutUs);


    /* ============= Timing Utilities ============= */

    /**
     * @brief Low-level cycle-accurate delay
     *
     * @note Implementation uses tight assembly loop executed from RAM
     * 
     * @param cycles Number of CPU cycles to delay
     */
    static void busyWaitCycles(uint32_t cycles);
    
    /**
     * @brief Busy-wait delay for nanosecond-precision timing
     *
     * @note Uses CPU clock (CPU_CLOCK_FREQUENCY) for calibration.
     *       Rounds up to ensure minimum delay is met.
     *       Maximum delay for 300MHz clock is 14000ns.
     * 
     * @param nanoseconds Delay duration in nanoseconds
     */
    static void busyWaitNanoseconds(uint32_t nanoseconds) {
        constexpr uint32_t CpuMhz = CPU_CLOCK_FREQUENCY / 1000000U;
        const uint32_t Cycles = ((nanoseconds * CpuMhz) + 999U) / 1000U;

        busyWaitCycles(Cycles);
    }

    /**
     * @brief Busy-wait delay for microsecond-precision timing
     * 
     * @note Uses CPU clock (CPU_CLOCK_FREQUENCY) for calibration.
     *       Maximum delay for 300MHz clock is 14000ms.
     *
     * @param microseconds Delay duration in microseconds
     */
    static void busyWaitMicroseconds(uint32_t microseconds) {
        constexpr uint32_t CpuMhz = CPU_CLOCK_FREQUENCY / 1000000U;
        const uint32_t Cycles = microseconds * CpuMhz;
        
        busyWaitCycles(Cycles);
    }


    /* ============= Hardware Configuration ============= */

    /**
     * @brief Enable NAND Flash mode for an SMC chip select.
     *
     * Configures the CCFG_SMCNFCS register to assign the specified chip select
     * to NAND Flash mode. This routes NANDOE/NANDWE signals to the chip select
     * and enables interpretation of address bits A21/A22 as CLE/ALE signals,
     * which is required for command and address latch operations.
     *
     * @param chipSelect SMC chip select to configure for NAND Flash mode.
     */
    static void enableNandFlashMode(ChipSelect chipSelect);
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
