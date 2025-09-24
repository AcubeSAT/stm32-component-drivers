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
    TIMEOUT = 1,
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
    UNSUPPORTED_OPERATION,
    ECC_UNCORRECTABLE,
    ECC_ALGORITHM_ERROR
};

/**
 * @brief Bad block information structure
 */
struct BadBlockInfo {
    uint16_t blockNumber;
    uint8_t lun;
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
    static constexpr uint16_t ECC_BYTES = 340;
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
        READ_STATUS = 0x70,
        ERASE_BLOCK = 0x60,
        ERASE_BLOCK_CONFIRM = 0xD0,
        READ_MODE = 0x00,
        READ_CONFIRM = 0x30,
        PAGE_PROGRAM = 0x80,
        PAGE_PROGRAM_CONFIRM = 0x10,
        CHANGE_WRITE_COLUMN = 0x85,
        CHANGE_READ_COLUMN = 0x05,
        CHANGE_READ_COLUMN_CONFIRM = 0xE0
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

    static constexpr etl::array<uint8_t, 5> EXPECTED_DEVICE_ID = {0x2C, 0x68, 0x00, 0x27, 0xA9}; /*!< Expected device ID for MT29F32G09ABAAA */
    
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
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout period
     */
    etl::expected<void, NANDErrorCode> waitForReady(uint32_t timeoutMs);
    
    /**
     * @brief Read NAND status register
     * 
     * @return Status register value or error code
     */
    uint8_t readStatusRegister();
        
    /**
     * @brief Validate NAND address bounds
     *
     * @param addr Address to validate
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS LUN, block, page, or column out of bounds
     */
    etl::expected<void, NANDErrorCode> validateAddress(const NANDAddress& addr);

    /**
     * @brief Calculate ECC codes for page data
     *
     * @param pageData Page data to calculate ECC for
     * @param[out] eccOutput Buffer to store calculated ECC (335 bytes)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid buffer size (must be 8192 bytes data, 335 bytes ECC)
     * @retval NANDErrorCode::ECC_ALGORITHM_ERROR BCH initialization or encoding failure
     */
    etl::expected<void, NANDErrorCode> calculateECC(etl::span<const uint8_t> pageData,
                                                    etl::span<uint8_t> eccOutput);

    /**
     * @brief Validate and correct page data using ECC codes
     *
     * @param pageData Page data to validate and correct (modified in-place)
     * @param eccCodes ECC codes read from spare area (335 bytes)
     *
     * @return Number of corrected errors, or specific error code
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid buffer size (must be 8192 bytes data, 335 bytes ECC)
     * @retval NANDErrorCode::ECC_ALGORITHM_ERROR BCH initialization or decoding failure
     * @retval NANDErrorCode::ECC_UNCORRECTABLE Too many errors (>4 per codeword or >255 total)
     */
    etl::expected<uint8_t, NANDErrorCode> validateAndCorrectECC(etl::span<uint8_t> pageData,
                                                               etl::span<const uint8_t> eccCodes);


    /* ============= Write protection management functions ============= */
    
    /**
     * @brief Enable writes by deasserting WP# pin
     */
    void enableWrites();
    
    /**
     * @brief Disable writes by asserting WP# pin
     */
    void disableWrites();
    

    /* ========== Bad block management helper functions ================ */
    
    /**
     * @brief Read block marker from spare area
     *
     * @param block Block number to check
     * @param lun LUN number
     *
     * @return Block marker byte (0xFF = good, 0x00 = bad) or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid size or column address
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::READ_FAILED Status register indicates read failure
     */
    etl::expected<uint8_t, NANDErrorCode> readBlockMarker(uint16_t block, uint8_t lun);

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

    
    /* ================ Parameter page parsing functions ================ */
        
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


    /**
     * @brief Validate device parameters match expected geometry
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::HARDWARE_FAILURE Device geometry mismatch or ONFI signature invalid
     * @retval NANDErrorCode::BAD_PARAMETER_PAGE All 3 parameter page copies have invalid CRC
     */
    etl::expected<void, NANDErrorCode> validateDeviceParameters();
    

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
          badBlockCount(0)
          {
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
     * @brief Read data from NAND flash with optional ECC validation
     *
     * @note If you need ECC check for the data then a whole page must be read.
     * 
     * @param addr NAND address to read from
     * @param[out] data Buffer to store read data (size determines bytes to read)
     * @param performECC Whether to read spare area and perform ECC validation (default: false)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Invalid size or column address
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::READ_FAILED Status register indicates read failure
     * @retval NANDErrorCode::ECC_UNCORRECTABLE Too many bit errors to correct
     * @retval NANDErrorCode::ECC_ALGORITHM_ERROR BCH algorithm failure
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> readPage(const NANDAddress& addr, etl::span<uint8_t> data, bool performECC = false);
    
    /**
     * @brief Program (write) data to NAND flash page
     *
     * @note Page must be in erased state before programming. Also a whole page must be written everytime. If you need to write  data that are
     *       lesss than a page, then you need to manually add 0xFF to the rest of the data. The function always expects a buffer that is 
     *       DATA_BYTES_PER_PAGE in length.
     * 
     * @details The function adds ECC data for the data to be written.
     *
     * @param addr NAND address to write to
     * @param data Data to write (max page size)
     *
     * @return Success or specific error code
     * @retval NANDErrorCode::NOT_INITIALIZED Driver not initialized
     * @retval NANDErrorCode::INVALID_PARAMETER Data size not equal to full page or column not zero
     * @retval NANDErrorCode::ADDRESS_OUT_OF_BOUNDS Address validation failed
     * @retval NANDErrorCode::BUSY_ARRAY Device array busy
     * @retval NANDErrorCode::TIMEOUT Device not ready within timeout
     * @retval NANDErrorCode::ECC_ALGORITHM_ERROR BCH algorithm failure
     * @retval NANDErrorCode::PROGRAM_FAILED Status register indicates program failure
     */
    [[nodiscard]] etl::expected<void, NANDErrorCode> programPage(const NANDAddress& addr, etl::span<const uint8_t> data);
     
    /**
     * @brief Erase a block
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
    bool isBlockBad(uint16_t block, uint8_t lun = 0);
    
    /**
     * @brief Mark a block as bad
     * 
     * @param block Block number to mark as bad
     * @param lun LUN number (typically 0)
     * 
     */
    void markBadBlock(uint16_t block, uint8_t lun = 0);
        
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
        case NANDErrorCode::ECC_UNCORRECTABLE: return "ECC uncorrectable errors";
        case NANDErrorCode::ECC_ALGORITHM_ERROR: return "ECC algorithm error";
        default: return "Unknown error";
    }
}
