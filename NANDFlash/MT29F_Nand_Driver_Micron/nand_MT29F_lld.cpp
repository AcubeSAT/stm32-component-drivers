/**
  @file
  Filename:		nand_MT29F_lld.cpp
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
#include <string.h>

#ifdef TIMEOUT_SUPPORT
#include <time.h>
#endif

#include "nand_MT29F_lld.hpp"

/* uncomment this line to show the address translation */
/* #define DEBUG_PRINT_ADDRESS */

/* state of the driver */
MT_uint8 driver_status = DRIVER_STATUS_NOT_INITIALIZED;

/* global structure with device info */
struct parameter_page_t device_info;

/******************************************************************************
 * 						Internal functions, not API
 *****************************************************************************/

/* Address comparison constants */
#define ADDR_A_EQ_B	0	/* A = B */
#define ADDR_A_LT_B	1	/* A < B */
#define ADDR_A_GT_B	2	/* A > B */

/* Internal functions */
MT_uint16 __as_uint16(MT_uint8 byte0, MT_uint8 byte1);
MT_uint32 __as_uint32(MT_uint8 byte0, MT_uint8 byte1, MT_uint8 byte2, MT_uint8 byte3);
void __as_string(MT_uint8 *src_ptr, char *dest_ptr, int start, int stop);
MT_uint8 __wait_for_ready();
MT_uint8 __is_valid_addr(nand_addr_t addr);
MT_uint8 __compare_addr(nand_addr_t first_addr, nand_addr_t second_addr);
void __build_cycle_addr(nand_addr_t addr, MT_uint8 *addr_cycle_stream);

MT29F mt29f(SMC::NCS3, MEM_NAND_BUSY_1_PIN, MEM_NAND_WR_ENABLE_PIN);

/******************************************************************************
 *						NAND Low Level Driver Functions
 *****************************************************************************/

/**
    This function initialize the driver and it must be called before
    any other function

    @return Return code
    @retval NAND_DRIVER_STATUS_INITIALIZED
    @retval DRIVER_STATUS_NOT_INITIALIZED

    @StartSteps
  	@Step Flash Reset
  	@Step Read ONFI signature
  	@Step Read the parameter page
  	@EndSteps
 */

MT_uint8 Init_Driver(void) {
    flash_width onfi_signature[ONFI_SIGNATURE_LENGHT];

    /* check if the driver is previously initialized */
    if(DRIVER_STATUS_INITIALIZED == driver_status)
        return DRIVER_STATUS_INITIALIZED;

    /* initialize platform */
    PLATFORM_Init();

    /* reset at startup is mandatory */
    NAND_Reset();

    /* read if this device is ONFI complaint */
    NAND_Read_ID_ONFI(onfi_signature);

    /* verify ONFI signature in the first field of parameter page */
    if(strcmp((const char *)onfi_signature, "ONFI") &&
       (NAND_BAD_PARAMETER_PAGE != NAND_Read_Param_Page(&device_info)))
        driver_status = DRIVER_STATUS_INITIALIZED;

    return driver_status;
}

/**
    The RESET (90h) command must be issued to all CE#s as the first command
    after power-on.

    @return Return code
    @retval NAND_TIMEOUT
    @retval NAND_SUCCESS

    @StartSteps
  	@Step Send reset command
  	@Step Wait for ready
  	@EndSteps
 */

MT_uint8 NAND_Reset(void) {
    MT_uint8 ret;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_RESET);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return ret;
}

/**
    The READ ID (90h) command is used to read identifier codes programmed into the target.
    This command is accepted by the target only when all die (LUNs) on the target are
    idle.

    @param[out] flash_width *buffer: buffer contains device id

    @return Return code
    @retval NAND_SUCCESS

    @StartSteps
  	@Step Send read id command (90h)
  	@Step Send address for read id (00h)
  	@Step Wait tWHR nanoseconds before read
  	@Step Read the device id
  	@EndSteps
 */

MT_uint8 NAND_Read_ID(flash_width *buffer) {
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_READID);
    mt29f.PLATFORM_SendAddr(ADDR_READ_ID);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WHR);

    /* read output */
    for(i=0; i<NUM_OF_READID_BYTES; i++)
        buffer[i] =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;
}

/**
    The READ ID (90h) command is used to read ONFI signature programmed into the target.
    This command is accepted by the target only when all die (LUNs) on the target are
    idle.

    @param[out] flash_width *buffer: buffer contains device id

    @return Return code
    @retval NAND_SUCCESS

    @StartSteps
  	@Step Send read id command (90h)
  	@Step Send address for read id (00h)
  	@Step Wait tWHR nanoseconds before read
  	@Step Read the ONFI signature
  	@EndSteps
 */

MT_uint8 NAND_Read_ID_ONFI(flash_width *buffer) {
    MT_uint32 i;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_READID);
    mt29f.PLATFORM_SendAddr(ADDR_READ_ID_ONFI);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WHR);

    /* read output */
    for(i=0; i<NUM_OF_READIDONFI_BYTES; i++)
        buffer[i] =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;
}

/**
    The READ PARAMETER PAGE (ECh) command is used to read the ONFI parameter page
    programmed into the target. This command is accepted by the target only when all die
    (LUNs) on the target are idle.

    @param[out] flash_width *ppage: a filled structure with ONFI paramter page

    @return Return code
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT

    @StartSteps
  	@Step Send parameter page command (ECh)
  	@Step Send address for parameter page (00h)
  	@Step Wait tWB nanoseconds before read
  	@Step Wait for ready
  	@Step Read the content of the parameter page
  	@Step Parse the parameter page buffer and fill the data structure
  	@EndSteps
 */

MT_uint8 NAND_Read_Param_Page(param_page_t *ppage) {
    MT_uint8 rbuf[NUM_OF_PPAGE_BYTES];
    MT_uint8 ret;
    MT_uint32 i;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_READ_PARAM_PAGE);
    mt29f.PLATFORM_SendAddr(ADDR_PARAM_PAGE);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read output */
    for(i=0; i<NUM_OF_PPAGE_BYTES; i++)
        rbuf[i] =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /*
     * Fill the parameter page data structure in the right way
     */

    /* Parameter page signature (ONFI) */
    __as_string(rbuf, ppage->signature, 0, 3);

    /* check if the buffer contains a valid ONFI parameter page */
    if (strcmp(ppage->signature, "ONFI"))
        return NAND_BAD_PARAMETER_PAGE;

    /* Revision number */
    ppage->rev_num = __as_uint16(rbuf[4], rbuf[5]);

    /* Features supported */
    ppage->feature = __as_uint16(rbuf[6], rbuf[7]);

    /* Optional commands supported */
    ppage->command = __as_uint16(rbuf[8], rbuf[9]);

    /* Device manufacturer */
    __as_string(rbuf, ppage->manufacturer, 32, 43);

    /* Device part number */
    __as_string(rbuf, ppage->model, 44, 63);

    /* Manufacturer ID (Micron = 2Ch) */
    ppage->jedec_id = rbuf[64];

    /* Date code */
    ppage->date_code = __as_uint16(rbuf[65], rbuf[66]);

    /* Number of data bytes per page */
    ppage->data_bytes_per_page = __as_uint32(rbuf[80], rbuf[81], rbuf[82], rbuf[83]);

    /* Number of spare bytes per page */
    ppage->spare_bytes_per_page = __as_uint16(rbuf[84], rbuf[85]);

    /* Number of data bytes per partial page */
    ppage->data_bytes_per_partial_page = __as_uint32(rbuf[86], rbuf[87], rbuf[88], rbuf[89]);

    /* Number of spare bytes per partial page */
    ppage->spare_bytes_per_partial_page = __as_uint16(rbuf[90], rbuf[91]);

    /* Number of pages per block */
    ppage->pages_per_block = __as_uint32(rbuf[92], rbuf[93], rbuf[94], rbuf[95]);

    /* Number of blocks per unit */
    ppage->blocks_per_lun = __as_uint32(rbuf[96], rbuf[97], rbuf[98], rbuf[99]);

    /* Number of logical units (LUN) per chip enable */
    ppage->luns_per_ce = rbuf[100];

    /* Number of address cycles */
    ppage->num_addr_cycles = rbuf[101];

    /* Number of bits per cell (1 = SLC; >1= MLC) */
    ppage->bit_per_cell = rbuf[102];

    /* Bad blocks maximum per unit */
    ppage->max_bad_blocks_per_lun = __as_uint16(rbuf[103], rbuf[104]);

    /* Block endurance */
    ppage->block_endurance = __as_uint16(rbuf[105], rbuf[106]);

    /* Guaranteed valid blocks at beginning of target */
    ppage->guarenteed_valid_blocks = rbuf[107];

    /* Block endurance for guaranteed valid blocks */
    ppage->guarenteed_valid_blocks = __as_uint16(rbuf[108], rbuf[109]);

    /* Number of programs per page */
    ppage->num_programs_per_page = rbuf[110];

    /* Partial programming attributes */
    ppage->partial_prog_attr = rbuf[111];

    /* Number of bits ECC bits */
    ppage->num_ECC_bits_correctable = rbuf[112];

    /* Number of interleaved address bits */
    ppage->num_interleaved_addr_bits = rbuf[113];

    /* Interleaved operation attributes */
    ppage->interleaved_op_attr = rbuf[114];

    return ret;
}

/**
    The SET FEATURES (EFh) command writes the subfeature parameters (P1�P4) to the
    specified feature address to enable or disable target-specific features. This command is
    accepted by the target only when all die (LUNs) on the target are idle.

    @param[in] flash_width feature_address: address of the feature (use ADDDR_FEATURE_XXXX define)
    @param[out] flash_width subfeature: subfeature (use FEATURE_XXXX define)

    @return Return code
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT

    @StartSteps
  	@Step Send set feature command (EFh)
  	@Step Send address of feature
  	@Step Wait tADL nanoseconds
  	@Step Send sub feature value
  	@Step Wait tWB nanoseconds
  	@Step Wait for ready
  	@EndSteps
 */

MT_uint8 NAND_Set_Feature(flash_width feature_address, flash_width subfeature) {
    MT_uint8 ret;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check if this feature/command is supported */
    if ((device_info.command & OPTIONAL_CMD_GET_FEATURES_AND_SET_FEATURES) == 0)
        return NAND_UNSUPPORTED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_SET_FEATURE);
    mt29f.PLATFORM_SendAddr(feature_address);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_ADL);

    /* send sub-feature parameter */
    mt29f.PLATFORM_SendData(subfeature);	/* p0 */
    mt29f.PLATFORM_SendData(0x00);		/* p1 reserved */
    mt29f.PLATFORM_SendData(0x00);		/* p2 reserved */
    mt29f.PLATFORM_SendData(0x00);		/* p3 reserved */

    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return ret;
}

/**
    The GET FEATURES (EEh) command reads the subfeature parameters (P1�P4) from the
    specified feature address. This command is accepted by the target only when all die
    (LUNs) on the target are idle.

    @param[in] flash_width feature_address: address of the feature (use ADDDR_FEATURE_XXXX define)
    @param[out] flash_width: value of the selected feature

    @return Return code
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT

    @StartSteps
  	@Step Send get feature command (EEh)
  	@Step Send address of the feature
  	@Step Wait tWB nanoseconds
  	@Step Wait for ready
  	@Step Read data
  	@EndSteps
 */

MT_uint8 NAND_Get_Feature(flash_width feature_address, flash_width *subfeature) {
    flash_width ret;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check if this feature/command is supported */
    if ((device_info.command & OPTIONAL_CMD_GET_FEATURES_AND_SET_FEATURES) == 0)
        return NAND_UNSUPPORTED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_GET_FEATURE);
    mt29f.PLATFORM_SendAddr(feature_address);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* send sub-feature parameter */
    *subfeature =  mt29f.PLATFORM_ReadData(); /* p0 */
    /*
     * skip p1, p2 and p3 because they are reserved and their value are 00h
     */

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return ret;
}

/**
    The READ STATUS (70h) command returns the status of the last-selected die (LUN) on
    a target. This command is accepted by the last-selected die (LUN) even when it is busy
    (RDY = 0).

    @return Value of status register
    @retval flash_width: value of status register

    @StartSteps
  	@Step Send read status command (70h)
  	@Step Wait tWHR nanoseconds
  	@Step Read data
  	@EndSteps
 */

flash_width NAND_Read_Status() {
    flash_width ret;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_READ_STATUS);

    /* wait */
    mt29f.PLATFORM_Wait(TIME_WHR);

    /* read value */
    ret =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return ret;
}

/**
    The READ STATUS ENHANCED (78h) command returns the status of the addressed die
    (LUN) on a target even when it is busy (RDY = 0). This command is accepted by all die
    (LUNs), even when they are BUSY (RDY = 0).

    @param[in] nand_addr_t addr: any valid address within the LUN to check (column address is ignored)

    @return Value of status register
    @retval flash_width: value of status register

    @StartSteps
  	@Step Send read status command (78h)
  	@Step Wait tWHR nanoseconds
  	@Step Read data
  	@EndSteps
 */

flash_width NAND_Read_Status_Enhanced(nand_addr_t addr) {
    flash_width ret;
    MT_uint8 row_address[5];

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* check if this feature/command is supported */
    if ((device_info.command & OPTIONAL_CMD_READ_STATUS_ENHANCED) == 0)
        return NAND_UNSUPPORTED;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command  */
    mt29f.PLATFORM_SendCmd(CMD_READ_STATUS_ENHANCED);

    /* send row address (3rd, 4th, 5th cycle) */
    mt29f.PLATFORM_SendAddr(row_address[2]);
    mt29f.PLATFORM_SendAddr(row_address[3]);
    mt29f.PLATFORM_SendAddr(row_address[4]);

    /* wait */
    mt29f.PLATFORM_Wait(TIME_WHR);

    /* read value */
    ret =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return ret;
}

/**
    Erase operations are used to clear the contents of a block in the NAND Flash array to
    prepare its pages for program operations.

    @param[in] nand_addr_t addr: any valid address within the block to erase (column address is ignored)

    @return Return code
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT

    @StartSteps
  	@Step Send erase block command (60h)
  	@Step Send row address of the block
  	@Step Send confirm command (D0h)
  	@Step Wait tWB nanoseconds
  	@Step Wait for ready
  	@Step Check status register value
  	@Read data
  	@EndSteps
 */

MT_uint8 NAND_Block_Erase(nand_addr_t addr) {
    MT_uint8 row_address[5];
    MT_uint8 status_reg;
    MT_uint8 ret;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command  */
    mt29f.PLATFORM_SendCmd(CMD_ERASE_BLOCK);

    /* send row address (3rd, 4th, 5th cycle) */
    mt29f.PLATFORM_SendAddr(row_address[2]);
    mt29f.PLATFORM_SendAddr(row_address[3]);
    mt29f.PLATFORM_SendAddr(row_address[4]);

    /* send confirm command */
    mt29f.PLATFORM_SendCmd(CMD_ERASE_BLOCK_CONFIRM);

    /* wait */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout occurs */
    if (NAND_SUCCESS != ret)
        return ret;

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* check if erase is good */
    if(!(status_reg & STATUS_WRITE_PROTECTED))
        return NAND_ERASE_FAILED_WRITE_PROTECT;

    if(status_reg & STATUS_FAIL)
        return NAND_ERASE_FAILED;

    return ret;
}

/**
    The READ PAGE (00h�30h) command copies a page from the NAND Flash array to its
    respective cache register and enables data output. This command is accepted by the die
    (LUN) when it is ready (RDY = 1, ARDY = 1).

    @param[in] nand_addr_t addr: address where to start reading
    @param[in] MT_uint32 lenght: number of byte (or word if x16 device) to read
    @param[out] flash_width *buffer: the buffer contains the data read from the flash

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_READ_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_Page_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 row_address[5];
    MT_uint8 status_reg;
    MT_uint8 ret;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* x16 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        if(lenght > (device_info.data_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.data_bytes_per_page)
        return NAND_INVALID_LENGHT;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command  */
    mt29f.PLATFORM_SendCmd(CMD_READ_MODE);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(row_address[i]);

    /* return to read mode */
    mt29f.PLATFORM_SendCmd(CMD_READ_CONFIRM);

    /* wait */
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read data */
    for(i=0; i<lenght; i++)
        buffer[i] =  mt29f.PLATFORM_ReadData();

    /* read status register on exit */
    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    if(status_reg & STATUS_FAIL)
        return NAND_READ_FAILED;

    return ret;
}

/**
    The PROGRAM PAGE (80h-10h) command enables the host to input data to a cache
    register, and moves the data from the cache register to the specified block and page
    address in the array of the selected die (LUN).

    @param[in] nand_addr_t addr: address where start reading
    @param[in] flash_width *buffer: the buffer contains the data to program into the flash
    @parma[in] MT_uint32 lenght: number of byte (or word) to program

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_Page_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 address[5];
    MT_uint8 status_reg;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* x16 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        if(lenght > (device_info.data_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.data_bytes_per_page)
        return NAND_INVALID_LENGHT;

    __build_cycle_addr(addr, address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(address[i]);

    /* send data */
    for(k=0; k<lenght; k++)
        mt29f.PLATFORM_SendData(buffer[k]);

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM_CONFIRM);

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* check if program is good */
    if(!(status_reg & STATUS_WRITE_PROTECTED))
        return NAND_PROGRAM_FAILED_WRITE_PROTECT;

    if(status_reg & STATUS_FAIL)
        return NAND_PROGRAM_FAILED;

    return NAND_SUCCESS;
}

/**
    The spare read function allows to read the spare area of a page.
    Please read the datasheet for more info about ECC-on-die feature.

    @param[in] nand_addr_t addr: address where to start reading (column is ignored)
    @param[in] MT_uint32 lenght: number of byte (or word) to read
    @parma[out] flash_width *buffer: the buffer contains the data read from the spare area

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_Spare_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 row_address[5];
    MT_uint8 status_reg;
    MT_uint8 ret;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        /* x16 */
        if(lenght > (device_info.spare_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.spare_bytes_per_page)
        return NAND_INVALID_LENGHT;

    /* spare area starts after last main area byte */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) == 0)
        /* x8 bus width */
        addr.column=device_info.data_bytes_per_page;
    else
        /* x16 bus width */
        addr.column=device_info.data_bytes_per_page >> 1;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_READ_MODE);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(row_address[i]);

    /* return to read mode */
    mt29f.PLATFORM_SendCmd(CMD_READ_CONFIRM);

    /* wait */
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read data */
    for(k=0; k<lenght; k++)
        buffer[k] =  mt29f.PLATFORM_ReadData();

    /* read status register on exit */
    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    if(status_reg & STATUS_FAIL)
        return NAND_READ_FAILED;

    return ret;
}

/**
    The spare program function allows to program the spare area of a page.
    Please read the datasheet for more info about ECC-on-die feature.

    @param[in] nand_addr_t addr: address where to start reading
    @param[in] flash_width *buffer: the buffer contains the data to program into the flash
    @param[in] MT_uint32 lenght: number of byte (or word) to program

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send program command (80h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_Spare_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 address[5];
    MT_uint8 status_reg;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        /* x16 */
        if(lenght > (device_info.spare_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.spare_bytes_per_page)
        return NAND_INVALID_LENGHT;

    /* spare area starts after last main area byte */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) == 0)
        /* x8 bus width */
        addr.column=device_info.data_bytes_per_page;
    else
        /* x16 bus width */
        addr.column=device_info.data_bytes_per_page >> 1;

    __build_cycle_addr(addr, address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(address[i]);

    /* send data */
    for(k=0; k<lenght; k++)
        mt29f.PLATFORM_SendData(buffer[k]);

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM_CONFIRM);

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* check if program is good */
    if(!(status_reg & STATUS_WRITE_PROTECTED))
        return NAND_PROGRAM_FAILED_WRITE_PROTECT;

    if(status_reg & STATUS_FAIL)
        return NAND_PROGRAM_FAILED;

    return NAND_SUCCESS;
}

/*
 * NAND_Read_Unique_Id
 */
MT_uint8 NAND_Read_Unique_Id(flash_width *buffer) {

    int i;
    MT_uint8 ret;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command and/or address */
    mt29f.PLATFORM_SendCmd(CMD_READ_UNIQ_ID);
    mt29f.PLATFORM_SendAddr(ADDR_READ_UNIQ_ID);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);

    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read output */
    for(i=0; i<NUM_OF_UNIQUEID_BYTES; i++)
        buffer[i] = (MT_uint8)  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;

}

/**
    The copy back function allow to copy a content of a page in another page
    without any byte is transferred to the host. The destination page must be erased.

    @param[in] nand_addr_t src_addr: address of source page
    @param[in] nand_addr_t dest_addr: source of destination page

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send source address (5 cycles)
  	@Step Send read for internal data move command (35h)
  	@Step Wait for ready
  	@Step Send program for internal data move command (85h)
  	@Step Send destination address (5 cycles)
  	@Step Send confirm command (10h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_Copy_Back(nand_addr_t src_addr, nand_addr_t dest_addr) {
    MT_uint8 src_address_stream[NUM_OF_ADDR_CYCLE];
    MT_uint8 dest_address_stream[NUM_OF_ADDR_CYCLE];
    MT_uint8 status_reg;
    MT_uint8 ret;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return driver_status;

    /* check if this feature/command is supported */
    if ((device_info.command & OPTIONAL_CMD_COPYBACK) == 0)
        return NAND_UNSUPPORTED;

    /* check input parameters */
    if((NAND_SUCCESS != __is_valid_addr(src_addr)) || (NAND_SUCCESS != __is_valid_addr(dest_addr)))
        return NAND_INVALID_NAND_ADDRESS;

    /* check if the source and destination address are the same */
    if(!__compare_addr(src_addr, dest_addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* check if odd to even copy back is supported */
    if ((device_info.command & SUPPORTED_ODD_TO_EVEN_PAGE_COPYBACK) == 0) {
        if(((src_addr.page & 1) != 0) && ((dest_addr.page & 1) == 0))
            return NAND_INVALID_NAND_ADDRESS;
        if(((src_addr.page & 1) == 0) && ((dest_addr.page & 1) != 0))
            return NAND_INVALID_NAND_ADDRESS;
    }

    /* copy back allows to move data from one page to another on the same plane */
    if(((src_addr.block & 1) != 0) && ((dest_addr.block & 1) == 0))
        return NAND_INVALID_NAND_ADDRESS;
    if(((src_addr.block & 1) == 0) && ((dest_addr.block & 1) != 0))
        return NAND_INVALID_NAND_ADDRESS;

    /* copy back is allowed only at the start of a page (ignoring column) */
    src_addr.column = 0;
    dest_addr.column = 0;

    /* build address cycles for source and destination address */
    __build_cycle_addr(src_addr, src_address_stream);
    __build_cycle_addr(dest_addr, dest_address_stream);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_READ_MODE);

    /* send source address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(src_address_stream[i]);

    /* read for internal data move */

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_READ_INTERNAL_DATA_MOVE);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* program for internal data move */

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PROGRAM_INTERNAL_DATA_MOVE);

    /* send destination address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(dest_address_stream[i]);

    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM_CONFIRM);

    /* wait (see datasheet for details) */
    mt29f.PLATFORM_Wait(TIME_WB);
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* check if program is good */
    if(!(status_reg & STATUS_WRITE_PROTECTED))
        return NAND_PROGRAM_FAILED_WRITE_PROTECT;

    if(status_reg & STATUS_FAIL)
        return NAND_PROGRAM_FAILED;

    return ret;
}

/*
 * NAND_Multiplane_Copy_Back
 */
MT_uint8 NAND_Multiplane_Copy_Back(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Cache_Read
 */
MT_uint8 NAND_Cache_Read(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Cache_Program
 */
MT_uint8 NAND_Cache_Program(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Multiplane_Page_Program
 */
MT_uint8 NAND_Multiplane_Page_Program(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Multiplane_Block_Erase
 */
MT_uint8 NAND_Multiplane_Block_Erase(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/**
    The LOCK command locks all of the blocks in the device. Locked blocks are write-protected
    from PROGRAM and ERASE operations.

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_SUCCESS

    @StartSteps
  	@Step Send lock command (2Ah)
  	@EndSteps
 */

MT_uint8 NAND_Lock(void) {

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_LOCK);

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;
}

/**
    By default at power-on, if LOCK is HIGH, all the blocks are locked and protected from
    PROGRAM and ERASE operations. If portions of the device are unlocked using the UNLOCK
    (23h) command, they can be locked again using the LOCK (2Ah) command.

    @param[in] nand_addr_t start_block: address of start block to unlock (column is ignored)
    @param[in] nand_addr_t end_block: address of end block to unlock (column is ignored)

    @return Return code
    @retval DRIVER_NOT_INITIALIZED
    @retval INVALID_NAND_ADDRESS
    @retval NAND_SUCCESS

    @StartSteps
  	@Step Send unlock low command (23h)
  	@Step Send start block address (row only cycles)
  	@Step Send unlock command (24h)
  	@Step Send end block address (row only cycles)
  	@EndSteps
 */

MT_uint8 NAND_Unlock(nand_addr_t start_block, nand_addr_t end_block) {
    MT_uint8 start_address_stream[NUM_OF_ADDR_CYCLE];
    MT_uint8 end_address_stream[NUM_OF_ADDR_CYCLE];

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if((NAND_SUCCESS != __is_valid_addr(start_block)) || (NAND_SUCCESS != __is_valid_addr(end_block)))
        return NAND_INVALID_NAND_ADDRESS;

    /* verify if start_block < end_block */
    if(ADDR_A_GT_B == __compare_addr(start_block, end_block) )
        return NAND_INVALID_NAND_ADDRESS;

    /* build address cycles for start and end block */
    __build_cycle_addr(start_block, start_address_stream);
    __build_cycle_addr(end_block, end_address_stream);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_BLOCK_UNLOCK_LOW);

    /* send row address (3rd, 4th, 5th cycle) */
    mt29f.PLATFORM_SendAddr(start_address_stream[2]);
    mt29f.PLATFORM_SendAddr(start_address_stream[3]);
    mt29f.PLATFORM_SendAddr(start_address_stream[4]);

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_BLOCK_UNLOCK_HIGH);

    /* send row address (3rd, 4th, 5th cycle) */
    mt29f.PLATFORM_SendAddr(end_address_stream[2]);
    mt29f.PLATFORM_SendAddr(end_address_stream[3]);
    mt29f.PLATFORM_SendAddr(end_address_stream[4]);

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;

}

/**
    The BLOCK LOCK READ STATUS (7Ah) command is used to determine the protection
    status of individual blocks.

    @param[in] nand_addr_t block_addr: address of block to check (column is ignored)

    @return Value of block status register
    @retval MT_uint8 block_lock_status_reg: value of block status register

    @StartSteps
  	@Step Send block lock read status command (7Ah)
  	@Step Send block address (row only cycles)
  	@EndSteps
 */

MT_uint8 NAND_Read_Lock_Status(nand_addr_t block_addr) {
    MT_uint8 block_addr_stream[NUM_OF_ADDR_CYCLE];
    MT_uint8 block_lock_status_reg;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(block_addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* build address cycles for block address */
    __build_cycle_addr(block_addr, block_addr_stream);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command*/
    mt29f.PLATFORM_SendCmd(CMD_BLOCK_LOCK_READ_STATUS);

    /* send row address (3rd, 4th, 5th cycle) */
    mt29f.PLATFORM_SendAddr(block_addr_stream[2]);
    mt29f.PLATFORM_SendAddr(block_addr_stream[3]);
    mt29f.PLATFORM_SendAddr(block_addr_stream[4]);

    block_lock_status_reg =  mt29f.PLATFORM_ReadData();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* return the value of block status register */
    return block_lock_status_reg;

}

/*
 * NAND_Lock_Down
 */
MT_uint8 NAND_Lock_Down(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Unlock_Down
 */
MT_uint8 NAND_Unlock_Down(void) {

    /*
     * TO BE IMPLEMENTED
     *
     * Please contact Micron support for any request
     */

    return NAND_UNIMPLEMENTED;

}

/*
 * NAND_Enter_OTP_Mode
 */

/**
	This function is used to set the device in OTP mode. When the device is in this mode,
	please use only the dedicated OTP functions set:

	- NAND_OTP_Page_Program()
	- NAND_OTP_Spare_Program()
	- NAND_OTP_Page_Read()
	- NAND_OTP_Spare_Read()

    @param[in] none

    @return Function returns DRIVER_STATUS_NOT_INITIALIZED, NAND_SUCCESS or NAND_GENERIC_FAIL

    @StartSteps
  	@Step Check if the device is in normal mode (ADDR_FEATURE_ARRAY_OPMODE)
  	@Step Set the device in OTP mode using set_feature
  	@Step Check if the device is now in OTP mode (FEATURE_ARRAY_OTP_OPERATION)
  	@EndSteps
 */
MT_uint8 NAND_OTP_Mode_Enter(void) {
    MT_uint8 ret;
    flash_width subfeature;

    /* check if driver is in a valid state */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check if device is NOT in OTP status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    if((FEATURE_ARRAY_NORMAL != subfeature) && (FEATURE_ARRAY_OTP_PROTECTION != subfeature))
        return NAND_GENERIC_FAIL;

    /* set OTP status */
    ret = NAND_Set_Feature(ADDR_FEATURE_ARRAY_OPMODE, FEATURE_ARRAY_OTP_OPERATION);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    /* check if device is in OTP status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    if(FEATURE_ARRAY_OTP_OPERATION != subfeature)
        return NAND_GENERIC_FAIL;

    return NAND_SUCCESS;
}

/**
	This function is used to exit the OTP mode. Only after the issue of this command
	user can use the standard functions to read/program/erase the NAND.

    @param[in] none

    @return Function returns DRIVER_STATUS_NOT_INITIALIZED, NAND_SUCCESS or NAND_GENERIC_FAIL

    @StartSteps
  	@Step Check if the device is in OTP mode (FEATURE_ARRAY_OTP_OPERATION)
  	@Step Set the device in normal mode (FEATURE_ARRAY_NORMAL) using set_feature
  	@Step Check if the device is now in normal mode (FEATURE_ARRAY_NORMAL)
  	@EndSteps
 */
MT_uint8 NAND_OTP_Mode_Exit(void) {
    MT_uint8 ret;
    flash_width subfeature;

    /* check if driver is in a valid state */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check if device is NOT in normal status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    if((FEATURE_ARRAY_OTP_OPERATION != subfeature) && (FEATURE_ARRAY_OTP_PROTECTION != subfeature))
        return NAND_GENERIC_FAIL;

    /* exit OTP status */
    ret = NAND_Set_Feature(ADDR_FEATURE_ARRAY_OPMODE, FEATURE_ARRAY_NORMAL);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    /* check if device is in normal status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    if(FEATURE_ARRAY_NORMAL != subfeature)
        return NAND_GENERIC_FAIL;

    return NAND_SUCCESS;
}

/**
	This function is used to lock PERMANENTLY a page in the OTP area.
	Warning! If this function is not called after an OTP data program, you can still
	perform bit manipulation program in the OTP area!!

    @param[in] none

    @return Function returns DRIVER_STATUS_NOT_INITIALIZED, NAND_SUCCESS or NAND_GENERIC_FAIL

    @StartSteps
  	@Step Check if the device is previously in OTP mode (FEATURE_ARRAY_OTP_OPERATION)
  	@Step Set the device in protect mode (FEATURE_ARRAY_OTP_PROTECTION) using set_feature
  	@Step Check if the device is now in protect mode (FEATURE_ARRAY_OTP_PROTECTION)
  	@Step Send fake program on the OTP page with 0x00 filled buffer
  	@Step Re-enter in OTP mode
  	@EndSteps
 */
MT_uint8 NAND_OTP_Mode_Protect(nand_addr_t addr) {
    MT_uint8 ret;
    flash_width subfeature;
    flash_width write_buf[device_info.data_bytes_per_page + device_info.spare_bytes_per_page];

    /* check if driver is in a valid state */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check if device is NOT in normal status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    if(FEATURE_ARRAY_OTP_OPERATION != subfeature)
        return NAND_GENERIC_FAIL;

    /* issue protect OTP area command */
    ret = NAND_Set_Feature(ADDR_FEATURE_ARRAY_OPMODE, FEATURE_ARRAY_OTP_PROTECTION);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    /* check if device is in normal status */
    ret = NAND_Get_Feature(ADDR_FEATURE_ARRAY_OPMODE, &subfeature);

    /* return with error if a fail occurs */
    if(NAND_SUCCESS != ret)
        return ret;

    if(FEATURE_ARRAY_OTP_PROTECTION != subfeature)
        return NAND_GENERIC_FAIL;

    /* issue the PROGRAM command to lock permanently the page */

    /* data buffer is filled with 0x00 */
    memset(write_buf, 0x00, device_info.data_bytes_per_page);

    /* x8 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) == 0)
        ret = NAND_Page_Program(addr, write_buf, device_info.data_bytes_per_page);
        /* x16 */
    else
        ret = NAND_Page_Program(addr, write_buf, (device_info.data_bytes_per_page>>2));

    if(NAND_SUCCESS != ret) {
        /* in case of program error, exit in OTP mode */
        NAND_OTP_Mode_Enter();
        return NAND_GENERIC_FAIL;
    }

    /* restore OTP mode before return */
    ret = NAND_OTP_Mode_Enter();

    if(NAND_SUCCESS != ret)
        return NAND_GENERIC_FAIL;

    return NAND_SUCCESS;
}

/**
    The PROGRAM PAGE (80h-10h) command enables the host to input data to a cache
    register, and moves the data from the cache register to the specified block and page
    address in the array of the selected die (LUN).
    Use this function only to program OTP area!

    @param[in] nand_addr_t addr: address where start reading
    @param[in] flash_width *buffer: the buffer contains the data to program into the flash
    @parma[in] MT_uint32 lenght: number of byte (or word) to program

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_OTP_Page_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 address[5];
    MT_uint8 status_reg;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* x16 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        if(lenght > (device_info.data_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.data_bytes_per_page)
        return NAND_INVALID_LENGHT;

    __build_cycle_addr(addr, address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(address[i]);

    /* send data */
    for(k=0; k<lenght; k++)
        mt29f.PLATFORM_SendData(buffer[k]);

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM_CONFIRM);

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    return NAND_SUCCESS;
}

/**
    The spare program function allows to program the spare area of a page.
    Please read the datasheet for more info about ECC-on-die feature.
    Use this function only to program OTP area!

    @param[in] nand_addr_t addr: address where to start reading
    @param[in] flash_width *buffer: the buffer contains the data to program into the flash
    @param[in] MT_uint32 lenght: number of byte (or word) to program

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send program command (80h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_OTP_Spare_Program(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 address[5];
    MT_uint8 status_reg;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* x16 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        if(lenght > (device_info.spare_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.spare_bytes_per_page)
        return NAND_INVALID_LENGHT;

    /* spare area starts after last main area byte */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) == 0)
        /* x8 bus width */
        addr.column=device_info.data_bytes_per_page;
    else
        /* x16 bus width */
        addr.column=device_info.data_bytes_per_page >> 1;

    __build_cycle_addr(addr, address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(address[i]);

    /* send data */
    for(k=0; k<lenght; k++)
        mt29f.PLATFORM_SendData(buffer[k]);

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_PAGE_PROGRAM_CONFIRM);

    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    /* check if program is good */
    if(status_reg & STATUS_FAIL)
        return NAND_PROGRAM_FAILED;

    return NAND_SUCCESS;
}

/**
    The READ PAGE (00h�30h) command copies a page from the NAND Flash array to its
    respective cache register and enables data output. This command is accepted by the die
    (LUN) when it is ready (RDY = 1, ARDY = 1).
    Use this function only to read OTP area!

    @param[in] nand_addr_t addr: address where to start reading
    @param[in] MT_uint32 lenght: number of byte (or word if x16 device) to read
    @param[out] flash_width *buffer: the buffer contains the data read from the flash

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_READ_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_OTP_Page_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 row_address[5];
    MT_uint8 status_reg;
    MT_uint8 ret;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    /* x16 */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        if(lenght > (device_info.data_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.data_bytes_per_page)
        return NAND_INVALID_LENGHT;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command  */
    mt29f.PLATFORM_SendCmd(CMD_READ_MODE);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(row_address[i]);

    /* return to read mode */
    mt29f.PLATFORM_SendCmd(CMD_READ_CONFIRM);

    /* wait */
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read data */
    for(i=0; i<lenght; i++)
        buffer[i] =  mt29f.PLATFORM_ReadData();

    /* read status register on exit */
    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    if(status_reg & STATUS_FAIL)
        return NAND_READ_FAILED;

    return ret;
}

/**
    The spare read function allows to read the spare area of a page.
    Please read the datasheet for more info about ECC-on-die feature.
    Use this function only to read OTP area!

    @param[in] nand_addr_t addr: address where to start reading (column is ignored)
    @param[in] MT_uint32 lenght: number of byte (or word) to read
    @parma[out] flash_width *buffer: the buffer contains the data read from the spare area

    @return Return code
    @retval DRIVER_STATUS_NOT_INITIALIZED
    @retval NAND_INVALID_NAND_ADDRESS
    @retval NAND_PROGRAM_FAILED_WRITE_PROTECT
    @retval NAND_PROGRAM_FAILED
    @retval NAND_SUCCESS
    @retval NAND_TIMEOUT


    @StartSteps
  	@Step Send read command (00h)
  	@Step Send address (5 cycles)
  	@Step Send data
  	@Step Send confirm command (30h)
  	@Step Wait for ready
  	@Step Check status register value
  	@EndSteps
 */

MT_uint8 NAND_OTP_Spare_Read(nand_addr_t addr, flash_width *buffer, MT_uint32 lenght) {
    MT_uint8 row_address[5];
    MT_uint8 status_reg;
    MT_uint8 ret;
    MT_uint32 k;
    int i;

    /* verify if driver is initialized */
    if(DRIVER_STATUS_INITIALIZED != driver_status)
        return DRIVER_STATUS_NOT_INITIALIZED;

    /* check input parameters */
    if(NAND_SUCCESS != __is_valid_addr(addr))
        return NAND_INVALID_NAND_ADDRESS;

    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) != 0) {
        /* x16 */
        if(lenght > (device_info.spare_bytes_per_page >> 1) )
            return NAND_INVALID_LENGHT;
    }

    /* x8 */
    if(lenght > device_info.spare_bytes_per_page)
        return NAND_INVALID_LENGHT;

    /* spare area starts after last main area byte */
    if((device_info.feature & SUPPORTED_16_BIT_DATA_BUS_WIDTH) == 0)
        /* x8 bus width */
        addr.column=device_info.data_bytes_per_page;
    else
        /* x16 bus width */
        addr.column=device_info.data_bytes_per_page >> 1;

    __build_cycle_addr(addr, row_address);

    /* init board transfer */
    mt29f.PLATFORM_Open();

    /* send command */
    mt29f.PLATFORM_SendCmd(CMD_READ_MODE);

    /* send address */
    for(i=0; i<NUM_OF_ADDR_CYCLE; i++)
        mt29f.PLATFORM_SendAddr(row_address[i]);

    /* return to read mode */
    mt29f.PLATFORM_SendCmd(CMD_READ_CONFIRM);

    /* wait */
    ret = __wait_for_ready();

    /* return if timeout */
    if (NAND_SUCCESS != ret)
        return ret;

    /* read data */
    for(k=0; k<lenght; k++)
        buffer[k] =  mt29f.PLATFORM_ReadData();

    /* read status register on exit */
    status_reg = NAND_Read_Status();

    /* close board transfer */
    mt29f.PLATFORM_Close();

    if(status_reg & STATUS_FAIL)
        return NAND_READ_FAILED;

    return ret;
}

/******************************************************************************
 * 						Internal functions, not API
 *****************************************************************************/

/*
 * This function is used internally from the driver in order to know when
 * an operation (program or erase) is completed.
 */

#define BIT_USED_TO_POLL STATUS_RDY

#ifdef TIMEOUT_SUPPORT
#define NUM_OF_TICKS_TO_TIMEOUT (TIME_OUT_SECOND * CLOCKS_PER_SEC)
#endif

MT_uint8 __wait_for_ready() {
    MT_uint8 ret;

#ifdef TIMEOUT_SUPPORT
    MT_uint32 clock_start = (MT_uint32) clock();
#endif

    mt29f.PLATFORM_SendCmd(CMD_READ_STATUS);

#ifndef TIMEOUT_SUPPORT

    while (BIT_USED_TO_POLL != (BIT_USED_TO_POLL & PLATFORM_ReadData()))
    { /* do nothing */ 	}

    PLATFORM_SendCmd(CMD_READ_MODE);
    return SUCCESS;

#else

    while ( (BIT_USED_TO_POLL != (BIT_USED_TO_POLL &  mt29f.PLATFORM_ReadData())) \
				&& ((MT_uint32) clock() < (MT_uint32) (clock_start + NUM_OF_TICKS_TO_TIMEOUT)) )
			{  /* do nothing */ }

		/* check exit condition */
		if(clock() >= clock_start + NUM_OF_TICKS_TO_TIMEOUT)
			ret = NAND_TIMEOUT;
		else
			ret = NAND_SUCCESS;

        mt29f.PLATFORM_SendCmd(CMD_READ_MODE);
		return ret;

#endif

}

/*
 * Verify that an address is valid with the current
 * device size
 */
MT_uint8 __is_valid_addr(nand_addr_t addr) {
    if((addr.column < device_info.data_bytes_per_page) &&
       (addr.page < device_info.pages_per_block) &&
       (addr.block < device_info.blocks_per_lun) &&
       (addr.lun < device_info.luns_per_ce))
        return NAND_SUCCESS;
    return NAND_INVALID_NAND_ADDRESS;
}

/*
 * Compare two address
 */
MT_uint8 __compare_addr(nand_addr_t first_addr, nand_addr_t second_addr) {

    /* first_addr = second_addr */
    if((first_addr.lun == second_addr.lun) && \
		(first_addr.block == second_addr.block) && \
		(first_addr.page == second_addr.page) && \
		(first_addr.column == first_addr.column))
        return ADDR_A_EQ_B;

    /* first_addr > second_addr */
    if((first_addr.lun > second_addr.lun) && \
		(first_addr.block > second_addr.block) && \
		(first_addr.page > second_addr.page) && \
		(first_addr.column > first_addr.column))
        return ADDR_A_GT_B;

    /* first_addr < second_addr */
    return ADDR_A_LT_B;
}

/*
 * __as_uint16
 */
MT_uint16 __as_uint16(MT_uint8 byte1, MT_uint8 byte0) {
#ifdef LITTLE_ENDIAN
    return ((MT_uint16) ((byte0 << 8) | byte1));
#endif
#ifdef BIG_ENDIAN
    return ((MT_uint16) ((byte1 << 8) | byte0));
#endif
}

/*
 * __as_uint32
 */
MT_uint32 __as_uint32(MT_uint8 byte3, MT_uint8 byte2, MT_uint8 byte1, MT_uint8 byte0) {
#ifdef LITTLE_ENDIAN
    return ((MT_uint32) ((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3));
#endif
#ifdef BIG_ENDIAN
    return ((MT_uint32) ((byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0));
#endif
}
/*
 * __as_string
 */
void __as_string(MT_uint8 *src_ptr, char *dest_ptr, int start, int stop) {
#ifdef LITTLE_ENDIAN
    strncpy((char *)dest_ptr, (const char *)src_ptr+start, stop-start+1);
		dest_ptr[stop-start+1] = '\0';
#endif
#ifdef BIG_ENDIAN
    strncpy((char *)dest_ptr, (const char *)src_ptr+start, stop-start+1);
		dest_ptr[stop-start+1] = '\0';
#endif
}

/*
 * __build_cycle_addr
 */
void __build_cycle_addr(nand_addr_t addr, MT_uint8 *addr_cycle_stream) {
#define LOW				0
#define HIGH			1

    /* extract n-th bit from a value */
#define CHECK_BIT(val, n) ((val & (1 << n)) >> n)

    /* extract from column address */
#define COL(n) 			CHECK_BIT(addr.column, n)

    /* extract from page address */
#define PAGE(n) 		CHECK_BIT(addr.page, n)

    /* extract from block address */
#define BLOCK(n) 		CHECK_BIT(addr.block, n)

    /* extract from lun number */
#define LUN(n) 			CHECK_BIT(addr.lun, n)

    /* build a single row of address cycle */
#define BUILD_ADDR_ROW(i_07, i_06, i_05, i_04, i_03, i_02, i_01, i_00) (\
		 ((i_07) << 7) \
	   | ((i_06) << 6) \
	   | ((i_05) << 5) \
	   | ((i_04) << 4) \
	   | ((i_03) << 3) \
	   | ((i_02) << 2) \
	   | ((i_01) << 1) \
	   | ((i_00) << 0) \
	);

    int cycle;

    /* build the address cycle stream (64 pages per block) */
    if (64 == device_info.pages_per_block) {

        /* Col 1 - I cycle */
        addr_cycle_stream[0] =
                (MT_uint8) BUILD_ADDR_ROW(COL(7), COL(6), COL(5), COL(4), COL(3), COL(2), COL(1), COL(0));

        /* Col 2 - II cycle */
        addr_cycle_stream[1] =
                (MT_uint8) BUILD_ADDR_ROW(LOW, LOW, COL(13), COL(12), COL(11), COL(10),COL(9),COL(8));

        /* Row 1 - III cycle */
        addr_cycle_stream[2] =
                (MT_uint8) BUILD_ADDR_ROW(BLOCK(1), BLOCK(0), PAGE(5), PAGE(4), PAGE(3), PAGE(2), PAGE(1), PAGE(0));

        /* Row 2 - IV cycle */
        addr_cycle_stream[3] =
                (MT_uint8) BUILD_ADDR_ROW(BLOCK(9), BLOCK(8), BLOCK(7), BLOCK(6), BLOCK(5), BLOCK(4), BLOCK(3), BLOCK(2));

        /* Row 3 - V cycle */
        addr_cycle_stream[4] =
                (MT_uint8) BUILD_ADDR_ROW(LOW, LOW, LOW, LOW, LUN(0), BLOCK(12), BLOCK(11), BLOCK(10));

    }

    /* build the address cycle stream (128 pages per block) */
    if (128 == device_info.pages_per_block) {

        /* Col 1 - I cycle */
        addr_cycle_stream[0] =
                (MT_uint8) BUILD_ADDR_ROW(COL(7), COL(6), COL(5), COL(4), COL(3), COL(2), COL(1), COL(0));

        /* Col 2 - II cycle */
        addr_cycle_stream[1] =
                (MT_uint8) BUILD_ADDR_ROW(LOW, LOW, COL(13), COL(12), COL(11), COL(10),COL(9),COL(8));

        /* Row 1 - III cycle */
        addr_cycle_stream[2] =
                (MT_uint8) BUILD_ADDR_ROW(BLOCK(0), PAGE(6), PAGE(5), PAGE(4), PAGE(3), PAGE(2), PAGE(1), PAGE(0));

        /* Row 2 - IV cycle */
        addr_cycle_stream[3] =
                (MT_uint8) BUILD_ADDR_ROW(BLOCK(8), BLOCK(7), BLOCK(6), BLOCK(5), BLOCK(4), BLOCK(3), BLOCK(2), BLOCK(1));

        /* Row 3 - V cycle */
        addr_cycle_stream[4] =
                (MT_uint8) BUILD_ADDR_ROW(LOW, LOW, LOW, LOW, LUN(0), BLOCK(11), BLOCK(10), BLOCK(9));

    }

#ifdef DEBUG_PRINT_ADDRESS
    printf("DEBUG: addr: LUN=%x, BLOCK=%x, PAGE=%x, COL=%x\n", \
			addr.lun, addr.block, addr.page, addr.column);
	printf("DEBUG: addr_cycl = I=%x II=%x III=%x IV=%x V=%x\n", \
			addr_cycle_stream[0], addr_cycle_stream[1], addr_cycle_stream[2], addr_cycle_stream[3], addr_cycle_stream[4]);
#endif
}
