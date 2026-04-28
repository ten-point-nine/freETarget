/*----------------------------------------------------------------
 *
 * board_assembly.h
 *
 * Header file to manage board configurations
 *
 *---------------------------------------------------------------*/
#ifndef __BOARD_ASSEMBLY_H__
#define __BOARD_ASSEMBLY_H__

/*
 *  Global Variables
 */
extern unsigned int board_mask;     // Board mask for the current board
extern int          board_revision; // Board revision number

/*
 *  Board Variants
 */
#define REV_100 100       // ESP32

#define MASK_100 (1 << 0) // First Revision of V5

/*
 * Processor Variants
 */
#define EPP32_8MB (MASK_100) // ESP32 with 8MB of flash (Standard)

                             /*
                              * Hardware Variants
                              */

#define COMMON (MASK_100) // Common to all boards

#endif
