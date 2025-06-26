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
    SUCCESS = 0,
    TIMEOUT,
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
    UNSUPPORTED_OPERATION
};

/**
 * @brief NAND flash device information from parameter page
 */
struct NANDDeviceInfo {
    char manufacturer[13];
    char model[21];
    uint8_t jedecId;
    uint16_t revisionNumber;
    uint32_t dataBytesPerPage;
    uint16_t spareBytesPerPage;
    uint32_t pagesPerBlock;
    uint32_t blocksPerLUN;
    uint8_t lunsPerCE;
    uint16_t maxBadBlocksPerLUN;
    uint16_t blockEndurance;
    uint8_t eccBits;
    bool isONFICompliant;
};

/**
 * @brief Bad block information structure
 */
struct BadBlockInfo {
    uint16_t blockNumber;
    uint8_t lun;
    bool isFactoryBad;
    uint32_t errorCount;
};

/**
 * @brief Driver for MT29F32G09ABAAA NAND Flash
 * 
 * 
 * @ingroup drivers
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/NAND-Flash-Interface-with-EBI-on-Cortex-M-Based-MCUs-DS90003184A.pdf
 * @see https://gr.mouser.com/datasheet/2/671/micron_technology_micts05995-1-1759202.pdf
 * @see https://onfi.org/files/onfi_2_0_gold.pdf
 */
class MT29F : public SMC {
public:
    /* ============== MT29F32G09ABAAA device constants =============== */
    static constexpr uint32_t DATA_BYTES_PER_PAGE = 8192;
    static constexpr uint16_t SPARE_BYTES_PER_PAGE = 448;
    static constexpr uint16_t TOTAL_BYTES_PER_PAGE = DATA_BYTES_PER_PAGE + SPARE_BYTES_PER_PAGE;
    static constexpr uint8_t PAGES_PER_BLOCK = 128;
    static constexpr uint16_t BLOCKS_PER_LUN = 4096;
    static constexpr uint8_t LUNS_PER_CE = 1;
    static constexpr uint32_t BLOCK_SIZE_BYTES = PAGES_PER_BLOCK * TOTAL_BYTES_PER_PAGE;
    static constexpr uint64_t MAX_ADDRESS = static_cast<uint64_t>(BLOCKS_PER_LUN) * BLOCK_SIZE_BYTES - 1;

    /**
     * @brief NAND address structure for 5-cycle addressing
     */
    struct NANDAddress {
        uint32_t lun;
        uint32_t block; 
        uint32_t page;
        uint32_t column;
        
        constexpr NANDAddress(uint32_t lun = 0, uint32_t block = 0, uint32_t page = 0, uint32_t column = 0)
            : lun(lun), block(block), page(page), column(column) {}
    };

private:
    /* =========== Hardware interface addresses and pins =========== */
 
    const uint32_t triggerNANDALEAddress = moduleBaseAddress | 0x200000; /*!< SMC address for triggering ALE (Address Latch Enable) */
    
    const uint32_t triggerNANDCLEAddress = moduleBaseAddress | 0x400000; /*!< SMC address for triggering CLE (Command Latch Enable) */

    const PIO_PIN nandReadyBusyPin; /*!< GPIO pin for monitoring R/B# (Ready/Busy) signal */
    
    const PIO_PIN nandWriteProtect; /*!< GPIO pin for controlling WP# (Write Protect) signal */

    inline static constexpr uint8_t enableNandConfiguration = 1; /*!< SMC NAND configuration enable flag */


    /* ================= Driver state variables ================== */
  
    bool isInitialized = false; /*!< Driver initialization status */
    
    NANDDeviceInfo deviceInfo{}; /*!< Device information from parameter page */

    
    /* ================= Bad block management  ================== */
    
    static constexpr size_t MAX_BAD_BLOCKS = 256;  /*!< Maximum number of bad blocks to track */
    
    etl::array<BadBlockInfo, MAX_BAD_BLOCKS> badBlockTable; /*!< Table of known bad blocks */
    
    size_t badBlockCount = 0; /*!< Current number of bad blocks in table */
    
    static constexpr uint16_t BAD_BLOCK_MARKER_OFFSET = 8192; /*!< Offset to bad block marker in spare area */
    
    static constexpr uint8_t GOOD_BLOCK_MARKER = 0xFF; /*!< Marker value for good blocks */
    
    static constexpr uint8_t BAD_BLOCK_MARKER = 0x00; /*!< Marker value for bad blocks */


    /* ================== ONFI standard ==================== */

    /** 
     * @brief NAND Commands 
     */
    enum class Commands : uint8_t {
        RESET = 0xFF,                        
        READID = 0x90,                      
        READ_PARAM_PAGE = 0xEC,              
        READ_UNIQ_ID = 0xED,                 
        SET_FEATURE = 0xEF,                  
        GET_FEATURE = 0xEE,                  
        READ_STATUS = 0x70,                  
        READ_STATUS_ENHANCED = 0x78,         
        ERASE_BLOCK = 0x60,                  
        ERASE_BLOCK_CONFIRM = 0xD0,          
        READ_MODE = 0x00,                    
        READ_CONFIRM = 0x30,                 
        READ_NEXT_SEQ_PAGE = 0x31,           
        PAGE_PROGRAM = 0x80,                 
        PAGE_PROGRAM_CONFIRM = 0x10,         
        READ_INTERNAL_DATA_MOVE = 0x35,      
        PROGRAM_INTERNAL_DATA_MOVE = 0x85,   
        LOCK = 0x2A,                         
        BLOCK_UNLOCK_LOW = 0x23,             
        BLOCK_UNLOCK_HIGH = 0x24,            
        BLOCK_LOCK_READ_STATUS = 0x7A       
    };
    
    /** 
     * @brief Address parameters for Read ID command 
     */
    enum class ReadIDAddress : uint8_t {
        MANUFACTURER_ID = 0x00,  
        ONFI_SIGNATURE = 0x20    
    };

    static constexpr uint8_t STATUS_FAIL = 0x01; /*!< Program/Erase operation failed */
    
    static constexpr uint8_t STATUS_FAILC = 0x02; /*!< Command failed */
    
    static constexpr uint8_t STATUS_ARDY = 0x20; /*!< Array ready */
    
    static constexpr uint8_t STATUS_RDY = 0x40; /*!< Device ready */
    
    static constexpr uint8_t STATUS_WP = 0x80; /*!< Write protected (1 = not protected, 0 = protected) */

    
    /* ============= Device identification constants ============ */

    static constexpr etl::array<uint8_t, 5> EXPECTED_DEVICE_ID = {0x2C, 0x68, 0x04, 0x4A, 0xA9}; /*!< Expected device ID for MT29F32G09ABAAA */
    
    /** 
     * @brief Structure for 5-cycle NAND addressing 
     */
    struct AddressCycles {
        uint8_t cycle[5];  ///< Address cycles: [CA1, CA2, RA1, RA2, RA3]
    };

    /* Calculated based on 1000Hz tick rate and the datasheet. For safety reasons they have a 10x margin. */ 
    static constexpr uint32_t TIMEOUT_READ_MS = 1;        /*!< Timeout constant for READ operation (60μs max + margin) */
    static constexpr uint32_t TIMEOUT_PROGRAM_MS = 6;     /*!< Timeout constant for RPROGRAM operation (600μs max + margin) */ 
    static constexpr uint32_t TIMEOUT_ERASE_MS = 60;      /*!< Timeout constant for ERASE operation (6ms max + margin) */
    static constexpr uint32_t TIMEOUT_RESET_MS = 10;      /*!< Timeout constant for RESET operation (1ms max + margin) */


    /* ======== Low-level hardware interface functions (SMC) ======== */

    /**
     * @brief Send data byte to NAND flash
     * 
     * @param data Data byte to send
     */
    inline void sendData(uint8_t data) {
        smcWriteByte(moduleBaseAddress, data);
    }

    /**
     * @brief Send address byte to NAND flash (triggers ALE)
     * 
     * @param address Address byte to send
     */
    inline void sendAddress(uint8_t address) {
        smcWriteByte(triggerNANDALEAddress, address);
    }

    /**
     * @brief Send command to NAND flash (triggers CLE)
     * 
     * @param command NAND command to send
     */
    inline void sendCommand(Commands command) {
        smcWriteByte(triggerNANDCLEAddress, static_cast<uint8_t>(command));
    }

    /**
     * @brief Read data byte from NAND flash
     * 
     * @return Data byte read from device
     */
    inline uint8_t readData() {
        return smcReadByte(moduleBaseAddress);
    }

    /* ========= Internal helper functions for NAND operations ========= */
    
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
     * @param addr NAND address structure
     * @param[out] cycles Generated address cycles
     */
    void buildAddressCycles(const NANDAddress& addr, AddressCycles& cycles);
    
    /**
     * @brief Wait for NAND device to become ready
     * 
     * @param timeoutMs Timeout in milliseconds
     * 
     * @return Success or timeout error
     */
    etl::expected<void, NANDErrorCode> waitForReady(uint32_t timeoutMs);
    
    /**
     * @brief Read NAND status register
     * 
     * @return Status register value or error code
     */
    etl::expected<uint8_t, NANDErrorCode> readStatusRegister();
    
    /**
     * @brief Check operation status for errors
     * 
     * @return Success or operation-specific error code
     */
    etl::expected<void, NANDErrorCode> checkOperationStatus();
    
    /**
     * @brief Validate NAND address bounds
     * 
     * @param addr Address to validate
     * 
     * @return Success or address bounds error
     */
    etl::expected<void, NANDErrorCode> validateAddress(const NANDAddress& addr);

    
    /* ============= Write protection management functions ============= */
    
    /**
     * @brief Enable writes by deasserting WP# pin
     */
    void enableWrites();
    
    /**
     * @brief Disable writes by asserting WP# pin
     */
    void disableWrites();
    
    /**
     * @brief Check if device is write protected
     * 
     * @return true if write protected (hardware or status register)
     */
    bool isWriteProtected();

    
    /* ========== Bad block management helper functions ================ */
    
    /**
     * @brief Check if a block is marked as bad
     * 
     * @param block Block number to check
     * @param lun LUN number
     * 
     * @return true if block is bad, false if good
     */
    etl::expected<bool, NANDErrorCode> checkBlockBad(uint16_t block, uint8_t lun);
    
    /**
     * @brief Mark a block as bad in bad block table and spare area
     * 
     * @param block Block number to mark
     * @param lun LUN number
     * @param isFactoryBad true if factory bad block, false if runtime failure
     * 
     * @return Success or error code
     */
    etl::expected<void, NANDErrorCode> markBlockBad(uint16_t block, uint8_t lun, bool isFactoryBad = false);
    
    /**
     * @brief Scan all blocks for factory bad block markers
     * 
     * @note Continues scanning even if some blocks can't be read
     * 
     * @details Reads the first byte of spare area from first page of each block.
     * Factory bad blocks are marked with 0x00, good blocks with 0xFF.
     * Populates the bad block table with found factory bad blocks.
     * 
     * @return Success or error code
     */
    etl::expected<void, NANDErrorCode> scanFactoryBadBlocks();
    
    /**
     * @brief Read bad block marker from spare area
     * 
     * @param block Block number to check
     * @param lun LUN number
     * 
     * @return Bad block marker byte (0xFF = good, 0x00 = bad)
     */
    etl::expected<uint8_t, NANDErrorCode> readBadBlockMarker(uint16_t block, uint8_t lun);

    
    /* ================ Parameter page parsing functions ================ */
    
    /**
     * @brief Parse ONFI parameter page data
     * 
     * @param paramPage 256-byte parameter page data
     * 
     * @return Parsed device information or error code
     */
    etl::expected<NANDDeviceInfo, NANDErrorCode> parseParameterPage(const etl::span<const uint8_t, 256>& paramPage);
    
    /**
     * @brief Validate parameter page CRC-16 checksum
     * 
     * @details Implements ONFI CRC-16 algorithm with polynomial 0x8005.
     * Initial value is 0x4F4E ("ON" in ASCII).
     * Validates first 254 bytes against stored CRC in bytes 254-255.
     * 
     * @param paramPage 256-byte parameter page data
     * 
     * @return true if CRC matches, false if invalid
     */
    bool validateParameterPageCRC(const etl::array<uint8_t, 256>& paramPage);
    

public:
    /**
     * @brief Constructor for MT29F NAND flash driver
     * 
     * @param chipSelect SMC chip select for NAND flash
     * @param readyBusyPin GPIO pin for R/B# signal monitoring
     * @param writeProtectPin GPIO pin for write protect control
     */
    MT29F(ChipSelect chipSelect, PIO_PIN readyBusyPin, PIO_PIN writeProtectPin) 
        : SMC(chipSelect),
          nandReadyBusyPin(readyBusyPin),
          nandWriteProtect(writeProtectPin),
          badBlockTable{},
          badBlockCount(0) {
        selectNandConfiguration(chipSelect);
    }
    
    /* Disable copy constructor and copy assignment operator */
    MT29F(const MT29F&) = delete;
    MT29F& operator=(const MT29F&) = delete;
    
    /**
     * @brief Configure SMC for NAND flash operation
     * 
     * @param chipSelect SMC chip select to configure
     */
    inline void selectNandConfiguration(ChipSelect chipSelect) {
        switch (chipSelect) {
            case NCS0:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS0(enableNandConfiguration);
                break;
            case NCS1:
                MATRIX_REGS->CCFG_SMCNFCS &= 0xF;
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS1(enableNandConfiguration);
                break;
            case NCS2:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS2(enableNandConfiguration);
                break;
            case NCS3:
                MATRIX_REGS->CCFG_SMCNFCS |= CCFG_SMCNFCS_SMC_NFCS3(enableNandConfiguration);
                break;
            default:
                break;
        }
    }

    /* ========= Driver Initialization and Basic Info ========= */
    
    /**
     * @brief Initialize the NAND flash driver and verify device
     * 
     * @details This function performs complete device initialization including:
     * - Device reset and ID verification
     * - ONFI compliance checking
     * - Parameter page reading and validation
     * - Factory bad block scanning
     * - Write protection initialization
     * 
     * @note Must be called before any other operations
     * 
     * @return Success or specific error code
     * @retval NANDErrorCode::SUCCESS Initialization completed successfully
     * @retval NANDErrorCode::HARDWARE_FAILURE Wrong device ID or ONFI failure
     * @retval NANDErrorCode::TIMEOUT Device not responding
     * @retval NANDErrorCode::BAD_PARAMETER_PAGE Invalid parameter page
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> initialize();
    
    /**
     * @brief Reset the NAND flash device
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> reset();
    
    /**
     * @brief Read device manufacturer and device ID
     * 
     * @param[out] id Buffer to store 5-byte device ID
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readDeviceID(etl::span<uint8_t, 5> id);
    
    /**
     * @brief Read ONFI signature to verify ONFI compliance
     * 
     * @param[out] signature Buffer to store 4-byte ONFI signature
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readONFISignature(etl::span<uint8_t, 4> signature);
    
    /**
     * @brief Read and parse the parameter page
     * 
     * @return Device information structure or error code
     */
    [[nodiscard]] etl::expected<NANDDeviceInfo, NANDErrorCode> readParameterPage();
    
    
    /**
     * @brief Get device information
     * 
     * @return Device info structure (only valid after successful initialization)
     */
    const NANDDeviceInfo& getDeviceInfo() const { return deviceInfo; }


    /* ================== Data Operations ================== */
    
    /**
     * @brief Read data from NAND flash
     * 
     * @param addr NAND address to read from
     * @param[out] data Buffer to store read data (max page size)
     * @param length Number of bytes to read
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readPage(const NANDAddress& addr, etl::span<uint8_t> data, uint32_t length);
    
    /**
     * @brief Read spare area from a page
     * 
     * @param addr NAND address (column ignored, set to spare area start)
     * @param[out] data Buffer to store spare data (max spare size)
     * @param length Number of bytes to read from spare area
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readSpare(const NANDAddress& addr, etl::span<uint8_t> data, uint32_t length);
    
    /**
     * @brief Program (write) data to NAND flash page
     * 
     * @note Page must be in erased state before programming
     * 
     * @param addr NAND address to write to
     * @param data Data to write (max page size)
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> programPage(const NANDAddress& addr, etl::span<const uint8_t> data);
    
    /**
     * @brief Program spare area of a page
     * 
     * @param addr NAND address (column ignored, set to spare area start) 
     * @param data Spare data to write (max spare size)
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> programSpare(const NANDAddress& addr, etl::span<const uint8_t> data);
    
    /**
     * @brief Erase a block
     * 
     * @param block Block number to erase
     * @param lun LUN number (typically 0)
     * 
     * @return Success or error code
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
    [[nodiscard]] etl::expected<bool, NANDErrorCode> isBadBlock(uint16_t block, uint8_t lun = 0);
    
    /**
     * @brief Mark a block as bad
     * 
     * @param block Block number to mark as bad
     * @param lun LUN number (typically 0)
     * 
     * @return Success or error code
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> markBadBlock(uint16_t block, uint8_t lun = 0);
    
    /**
     * @brief Get list of all bad blocks
     * 
     * @return Span of bad block information entries
     */
    [[nodiscard]] etl::span<const BadBlockInfo> getBadBlockList() const noexcept {
        return etl::span<const BadBlockInfo>(badBlockTable.data(), badBlockCount);
    }
    
    /**
     * @brief Get count of bad blocks
     * 
     * @return Number of bad blocks found
     */
    [[nodiscard]] size_t getBadBlockCount() const noexcept { return badBlockCount; }
   
};


/**
 * @brief Convert error code to human-readable string
 * 
 * @note Used for logging
 * @param error Error code to convert
 * 
 * @return String description of error
 */
constexpr const char* toString(NANDErrorCode error) {
    switch (error) {
        case NANDErrorCode::SUCCESS: return "Success";
        case NANDErrorCode::TIMEOUT: return "Timeout";
        case NANDErrorCode::ADDRESS_OUT_OF_BOUNDS: return "Address out of bounds";
        case NANDErrorCode::BUSY_IO: return "Device busy - I/O";
        case NANDErrorCode::BUSY_ARRAY: return "Device busy - Array";
        case NANDErrorCode::PROGRAM_FAILED: return "Program operation failed";
        case NANDErrorCode::ERASE_FAILED: return "Erase operation failed";
        case NANDErrorCode::READ_FAILED: return "Read operation failed";
        case NANDErrorCode::NOT_READY: return "Device not ready";
        case NANDErrorCode::WRITE_PROTECTED: return "Device write protected";
        case NANDErrorCode::BAD_BLOCK: return "Bad block detected";
        case NANDErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case NANDErrorCode::NOT_INITIALIZED: return "Driver not initialized";
        case NANDErrorCode::HARDWARE_FAILURE: return "Hardware failure";
        case NANDErrorCode::BAD_PARAMETER_PAGE: return "Bad parameter page";
        case NANDErrorCode::UNSUPPORTED_OPERATION: return "Unsupported operation";
        default: return "Unknown error";
    }
}
