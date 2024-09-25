/*******************************************************************************

  Filename:		nand_MT29F_lld.h
  Description:  Library routines for the Software Drivers for NAND flash

  � 2012 Micron Technology, Inc. All Rights Reserved

 You have a license to reproduce, display, perform, produce derivative works of,
 and distribute (in original or modified form) the Program, provided that you
 explicitly agree to the following disclaimer:

   THIS PROGRAM IS PROVIDED "AS IT IS" WITHOUT WARRANTY OF ANY KIND, EITHER
   EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTY
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE
   PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
   REPAIR OR CORRECTION.

********************************************************************************

   Version History.

   Ver.				No		Date     	Comments

   First release	1.0		07/2012
   Second release	1.1		03/2013		Added OTP functions

*******************************************************************************/

#include "common.h"
#include "nand_hw_if.h"

/******************************************************************************
 * 								USER SETTINGS AREA
 *****************************************************************************/

/**
 * Platform endianness (LITTLE_ENDIAN, BIG_ENDIAN)
 */
#define LITTLE_ENDIAN
// #define BIG_ENDIAN

/**
 * Timeout support (require time.h include file)
 */
#define TIMEOUT_SUPPORT

#ifdef TIMEOUT_SUPPORT
/** timeout in seconds */
#define TIME_OUT_SECOND			2

/**
 * if timeout support is enabled, this macro constant expression
 * CLOCKS_PER_SEC specifies the relation between a clock tick
 * and a second (clock ticks per second)
 */
#define CLOCKS_PER_SEC 			1000
#endif

/******************************************************************************
 * 							END OF USER SETTINGS AREA
 * 					(Please do not modify the following lines)
 *****************************************************************************/

/*
 * NAND commands and constants set
 */

#define MICRON_NAND_DEFAULT

/**
 * Flash width data type
 */

typedef bus_t flash_width;

/*
 * Data type and structure
 */

/** NAND address */
typedef struct nand_address_t {

    /* LUN */
    MT_uint32 lun;

    /* block address */
    MT_uint32 block;

    /* page address */
    MT_uint32 page;

    /* column address */
    MT_uint32 column;

} nand_addr_t;

/** Parameter Page Data Structure */
typedef struct parameter_page_t {
    /** Parameter page signature (ONFI) */
    char signature[5];

    /** Revision number */
    MT_uint16 rev_num;

    /** Features supported */
    MT_uint16 feature;

    /** Optional commands supported */
    MT_uint16 command;

    /** Device manufacturer */
    char manufacturer[13];

    /** Device part number */
    char model[21];

    /** Manufacturer ID (Micron = 2Ch) */
    MT_uint8 jedec_id;

    /** Date code */
    MT_uint16 date_code;

    /** Number of data bytes per page */
    MT_uint32 data_bytes_per_page;

    /** Number of spare bytes per page */
    MT_uint16 spare_bytes_per_page;

    /** Number of data bytes per partial page */
    MT_uint32 data_bytes_per_partial_page;

    /** Number of spare bytes per partial page */
    MT_uint16 spare_bytes_per_partial_page;

    /** Number of pages per block */
    MT_uint32 pages_per_block;

    /** Number of blocks per unit */
    MT_uint32 blocks_per_lun;

    /** Number of logical units (LUN) per chip enable */
    MT_uint8 luns_per_ce;

    /** Number of address cycles */
    MT_uint8 num_addr_cycles;

    /** Number of bits per cell (1 = SLC; >1= MLC) */
    MT_uint8 bit_per_cell;

    /** Bad blocks maximum per unit */
    MT_uint16 max_bad_blocks_per_lun;

    /** Block endurance */
    MT_uint16 block_endurance;

    /** Guaranteed valid blocks at beginning of target */
    MT_uint8 guarenteed_valid_blocks;

    /** Block endurance for guaranteed valid blocks */
    MT_uint16 block_endurance_guarenteed_valid;

    /** Number of programs per page */
    MT_uint8 num_programs_per_page;

    /** Partial programming attributes */
    MT_uint8 partial_prog_attr;

    /** Number of bits ECC bits */
    MT_uint8 num_ECC_bits_correctable;

    /** Number of interleaved address bits */
    MT_uint8 num_interleaved_addr_bits;

    /** Interleaved operation attributes */
    MT_uint8 interleaved_op_attr;

} param_page_t;

/*
 * List of features supported
 */
#ifdef MICRON_NAND_DEFAULT
#define SUPPORTED_EXTERNAL_VPP								0x1000
#define SUPPORTED_VOLUME_ADDRESSING							0x0800
#define SUPPORTED_NV_DDR2									0x0400
#define SUPPORTED_EZ_NAND									0x0200
#define SUPPORTED_PROGRAM_PAGE_REGISTER_CLEAR_ENHANCEMENT	0x0100
#define SUPPORTED_EXTENDED_PARAMETER_PAGE					0x0080
#define SUPPORTED_MULTIPLANE_READ_OPERATIONS				0x0040
#define SUPPORTED_NV_DDR									0x0020
#define SUPPORTED_ODD_TO_EVEN_PAGE_COPYBACK					0x0010
#define SUPPORTED_MULTIPLANE_PROGRAM_AND_ERASE_OPERATIONS	0x0008
#define SUPPORTED_NON_SEQUENTIAL_PAGE_PROGRAMMING			0x0004
#define SUPPORTED_MULTIPLE_LUN_OPERATIONS					0x0002
#define SUPPORTED_16_BIT_DATA_BUS_WIDTH						0x0001
#endif

/*
 * List of optional commands supported
 */
#ifdef MICRON_NAND_DEFAULT
#define OPTIONAL_CMD_ODT_CONFIGURE							0x0800
#define OPTIONAL_CMD_VOLUME_SELECT							0x0400
#define OPTIONAL_CMD_RESET_LUN								0x0200
#define OPTIONAL_CMD_SMALL_DATA_MOVE						0x0100
#define OPTIONAL_CMD_CHANGE_ROW_ADDRESS						0x0080
#define OPTIONAL_CMD_CHANGE_READ_COLUMN_ENHANCED			0x0040
#define OPTIONAL_CMD_READ_UNIQUE_ID							0x0020
#define OPTIONAL_CMD_COPYBACK								0x0010
#define OPTIONAL_CMD_READ_STATUS_ENHANCED					0x0008
#define OPTIONAL_CMD_GET_FEATURES_AND_SET_FEATURES			0x0004
#define OPTIONAL_CMD_READ_CACHE_COMMANDS					0x0002
#define OPTIONAL_CMD_PAGE_CACHE_PROGRAM_COMMAND				0x0001
#endif

/*
 * NAND Command set
 */
#ifdef MICRON_NAND_DEFAULT
#define CMD_RESET							0xFF
#define CMD_READID							0x90
#define CMD_READ_PARAM_PAGE					0xEC
#define	CMD_READ_UNIQ_ID					0xED
#define CMD_SET_FEATURE						0xEF
#define CMD_GET_FEATURE						0xEE
#define CMD_READ_STATUS						0x70
#define CMD_READ_STATUS_ENHANCED			0x78
#define CMD_ERASE_BLOCK						0x60
#define CMD_ERASE_BLOCK_CONFIRM				0xD0
#define CMD_READ_MODE						0x00
#define CMD_READ_CONFIRM					0x30
#define CMD_PAGE_PROGRAM					0x80
#define CMD_PAGE_PROGRAM_CONFIRM			0x10
#define CMD_READ_INTERNAL_DATA_MOVE			0x35
#define CMD_PROGRAM_INTERNAL_DATA_MOVE		0x85
#define CMD_LOCK							0x2A
#define CMD_BLOCK_UNLOCK_LOW				0x23
#define CMD_BLOCK_UNLOCK_HIGH				0x24
#define CMD_BLOCK_LOCK_READ_STATUS			0x7A
#endif

/*
 * Significant addresses
 */
#ifdef MICRON_NAND_DEFAULT
#define ADDR_READ_ID						0x00
#define ADDR_READ_ID_ONFI					0x20
#define ADDR_READ_UNIQ_ID					0x00
#define ADDR_PARAM_PAGE						0x00
#define ADDR_READ_UNIQ_ID					0x00
#define ADDR_FEATURE_TIMING					0x01
#define ADDR_FEATURE_OUTPUT_STRENGTH		0x80
#define ADDR_FEATURE_RB_STRENGTH			0x81
#define ADDR_FEATURE_ARRAY_OPMODE			0x90
#endif

/**
 * Significant constants
 */
#ifdef MICRON_NAND_DEFAULT
#define NUM_OF_READID_BYTES					5
#define NUM_OF_READIDONFI_BYTES				4
#define NUM_OF_READUNIQUEID_BYTES			16
#define NUM_OF_PPAGE_BYTES					129
#define NUM_OF_ADDR_CYCLE					5
#define ONFI_SIGNATURE_LENGHT				4
#define NUM_OF_UNIQUEID_BYTES				32
#endif

/*
 * Feature constants
 */
#ifdef MICRON_NAND_DEFAULT
/* Timing mode */
#define FEATURE_TIMING_MODE0				0x00
#define FEATURE_TIMING_MODE1				0x01
#define FEATURE_TIMING_MODE2				0x02
#define FEATURE_TIMING_MODE3				0x03
#define FEATURE_TIMING_MODE4				0x04
#define FEATURE_TIMING_MODE5				0x05
/* I/O drive strength */
#define FEATURE_OUTPUT_STRENGH_100			0x00
#define FEATURE_OUTPUT_STRENGH_75			0x01
#define FEATURE_OUTPUT_STRENGH_50			0x02
#define FEATURE_OUTPUT_STRENGH_25			0x03
/* R/B# Pull-Down strength */
#define FEATURE_RB_STRENGH_100				0x00
#define FEATURE_RB_STRENGH_75				0x01
#define FEATURE_RB_STRENGH_50				0x02
#define FEATURE_RB_STRENGH_25				0x03
/* Array operation mode */
#define FEATURE_ARRAY_NORMAL				0x00
#define FEATURE_ARRAY_OTP_OPERATION			0x01
#define FEATURE_ARRAY_OTP_PROTECTION		0x03
#define FEATURE_ARRAY_DISABLE_ECC			0x00
#define FEATURE_ARRAY_ENABLE_ECC			0x08
#endif

/*
 * Time constants, in ns (see the datasheet for more details)
 */
#ifdef MICRON_NAND_DEFAULT
#define TIME_WHR							60
#define TIME_WB								100
#define TIME_ADL							70
#endif

/*
 * Driver status constants
 */

/* normal driver state */
#define DRIVER_STATUS_INITIALIZED						0

/* driver is not initialized */
#define DRIVER_STATUS_NOT_INITIALIZED					1

/*
 * Status register definition
 */
#ifdef MICRON_NAND_DEFAULT
#define	STATUS_FAIL							0x00
#define	STATUS_FAILC						0x01
#define	STATUS_ARDY							0x20
#define	STATUS_RDY							0x40
#define	STATUS_WRITE_PROTECTED				0x80
#endif

/*
 * Block lock status register definition
 */
#ifdef MICRON_NAND_DEFAULT
#define BLOCK_LOCKED_TIGHT					0x01
#define BLOCK_LOCKED						0x02
#define BLOCK_UNLOCKED_DEV_LOCKED			0x05
#define BLOCK_UNLOCKED_DEV_NOT_LOCKED		0x06
#endif

/*
 * Function return constants
 */
#define NAND_SUCCESS							0x00
#define	NAND_GENERIC_FAIL						0x10
#define NAND_BAD_PARAMETER_PAGE					0x20
#define	NAND_INVALID_NAND_ADDRESS				0x30
#define NAND_INVALID_LENGHT						0x31
#define NAND_ERASE_FAILED						0x40
#define NAND_ERASE_FAILED_WRITE_PROTECT			0x41
#define NAND_PROGRAM_FAILED						0x50
#define NAND_PROGRAM_FAILED_WRITE_PROTECT		0x51
#define NAND_READ_FAILED						0x60
#define NAND_UNSUPPORTED						0xFD
#ifdef TIMEOUT_SUPPORT
#define NAND_TIMEOUT						0xFE
#endif

#define NAND_UNIMPLEMENTED						0xFF

/******************************************************************************
 *									List of APIs
 *****************************************************************************/

MT_uint8 Init_Driver(void);

/* reset operations */
MT_uint8 NAND_Reset(void);

/* identification operations */
MT_uint8 NAND_Read_ID(flash_width *buf);
MT_uint8 NAND_Read_ID_ONFI(flash_width *buf);
MT_uint8 NAND_Read_Param_Page(param_page_t *ppage);

/* feature operations */
MT_uint8 NAND_Get_Feature(flash_width feature_address, flash_width *subfeature);
MT_uint8 NAND_Set_Feature(flash_width feature_address, flash_width subfeature);

/* status operations */
flash_width NAND_Read_Status();
flash_width NAND_Read_Status_Enhanced(nand_addr_t addr);

/* read operations */
MT_uint8 NAND_Page_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght);
MT_uint8 NAND_Spare_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght);

/* erase operations */
MT_uint8 NAND_Block_Erase(nand_addr_t addr);

/* program operations */
MT_uint8 NAND_Page_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght);
MT_uint8 NAND_Spare_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght);

/* internal data move operations */
MT_uint8 NAND_Copy_Back(nand_addr_t src_addr, nand_addr_t dest_addr);

/* block lock operations */
MT_uint8 NAND_Lock(void);
MT_uint8 NAND_Unlock(nand_addr_t start_block, nand_addr_t end_block);
MT_uint8 NAND_Read_Lock_Status(nand_addr_t block_addr);

/******************************************************************************
 *							List of unimplemented APIs
 *****************************************************************************/

/* identification operations */
MT_uint8 NAND_Read_Unique_Id(void);

/* cached operations */
MT_uint8 NAND_Cache_Read(void);
MT_uint8 NAND_Cache_Program(void);

/* multi-plane operations */
MT_uint8 NAND_Multiplane_Copy_Back(void);
MT_uint8 NAND_Multiplane_Page_Program(void);
MT_uint8 NAND_Multiplane_Block_Erase(void);

/* block lock operations */
MT_uint8 NAND_Lock_Down(void);
MT_uint8 NAND_Unlock_Down(void);
