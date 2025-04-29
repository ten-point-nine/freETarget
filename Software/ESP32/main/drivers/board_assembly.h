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
#define REV_500 500        // ESP32
#define REV_510 510
#define REV_520 520
#define REV_530 530        // India 4MB
#define REV_600 600        // Value Engineered

#define MASK_500 (1 << 0)  // First Revision of V5
#define MASK_510 (1 << 0)  // First Test Board of V5
#define MASK_520 (1 << 15) // First Production
#define MASK_530 (1 << 4)  // Test Board of V6
#define MASK_600 (1 << 8)  // Value Engineered Board

/*
 * Processor Variants
 */
#define EPP32_8MB (MASK_500 | MASK_510 | MASK_520 | MASK_530) // ESP32 with 8MB of flash (Standard)
#define ESP32_4MB (MASK_600)                                  // ESP32 with 4MB of flash (Development)

                                                              /*
                                                               * Hardware Variants
                                                               */
#define HDC3022  (MASK_500 | MASK_510 | MASK_520 | MASK_530)                  // TI HDC3022 Temperature Humidity
#define TMP1075D (MASK_600)                                                   // TI TMP1075D Temperature Sensor

#define PCNT_LOW_GPIO  (MASK_500 | MASK_510 | MASK_520 | MASK_530 | MASK_600) // PCNT  LOW on GPIO
#define PCNT_HIGH_GPIO (MASK_500 | MASK_510 | MASK_520)                       // PCNT HIGH on GPIO

#define MCP4728 (MASK_500 | MASK_510 | MASK_520 | MASK_530)                   // Microchip MCP4728 4 channel Dac
#define MCP4725 (MASK_600)                                                    // Microchip MCP4725 1 channel Dac

#define VREF_FB        (MASK_530 | MASK_600)                                  // VREF Feedback
#define LDAC_GPIO      (0)                                                    // LDAC Control no longer used
#define FACE_HALF_GPIO (MASK_500 | MASK_510 | MASK_520)                       // FACE GPIO

#define COMMON (MASK_500 | MASK_510 | MASK_520 | MASK_530 | MASK_600)         // Common to all boards
#endif
