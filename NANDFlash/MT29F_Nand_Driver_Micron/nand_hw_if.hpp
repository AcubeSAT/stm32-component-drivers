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

#include "common.hpp"
#include "NANDFlash.hpp"

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

void PLATFORM_Init(void);

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

#endif
