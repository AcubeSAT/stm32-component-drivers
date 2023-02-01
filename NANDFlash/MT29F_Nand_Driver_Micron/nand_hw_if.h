/*******************************************************************************

  Filename:		nand_hw_if.h
  Description:  Header file for hardware NAND interface

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

*******************************************************************************/

#include "common.h"

/******************************************************************************
 * 								USER SETTINGS AREA
 *****************************************************************************/

/* support x16 NAND devices */
// #define BUS_WIDTH_16BIT;

/******************************************************************************
 * 							END OF USER SETTINGS AREA
 * 					(Please do not modify the following lines)
 *****************************************************************************/

/* support ASYNC mode */
#define ASYNC_MODE

/* define bus width */
#ifndef BUS_WIDTH_16BIT
typedef MT_uint8 bus_t;
#else
typedef MT_uint16 bus_t;
#endif

#ifdef ASYNC_MODE

/**
	Initialize HW NAND Controller.
	This function is called during driver initialization.
*/

void PLATFORM_Init();

/**
 	Open NAND device.
 	This function is called every time an I/O is performed.
*/
void PLATFORM_Open(void);

/**
	NAND Command input.
	This function is used to send a command to NAND.
*/
void PLATFORM_SendCmd(bus_t ubCommand);

/**
	NAND Address input.
	This function is used to send an address to NAND.
*/
void PLATFORM_SendAddr(bus_t ubAddress);

/**
	NAND Data input.
	This function is used to send data to NAND.
*/
void PLATFORM_SendData(bus_t data);

/**
	NAND Data output.
	This function is used to read data from NAND.
*/
bus_t PLATFORM_ReadData(void);

/**
	NAND Write protect (set WP = L).
	This function is used to set Write Protect (WP) pin to LOW
*/
void PLATFORM_SetWriteProtect(void);

/**
	NAND Write protect (set WP = H).
	This function is used to set Write Protect (WP) pin to HIGH
*/
void PLATFORM_UnsetWriteProtect(void);

/**
	Wait for microseconds.
	This function should call a platform or OS wait() function.
*/
void PLATFORM_Wait(int microseconds);

/**
	Close HW NAND Controller.
	This function is used to close the NAND HW controller in the right way.
*/
void PLATFORM_Close(void);

#endif
