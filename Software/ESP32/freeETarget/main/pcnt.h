/*----------------------------------------------------------------
 *
 * pcnt.h
 *
 * Header file for Pulse Counter Module
 *
 *---------------------------------------------------------------*/
#ifndef _PCNT_H_
#define _PCNT_H_

/*
 * Global functions
 */
void pcnt_init(int  unit, int  control, int  signal);   // pcnt Control
unsigned int pcnt_read(int unit);                       // Read timer contents
void pcnt_clear(int unit);                              // Clear the timer contents

/*
 * Typedefs
 */

/*
 * Definitions
 */
#define NORTH_LOW  0                    // Sensor counter registers
#define EAST_LOW   1
#define SOUTH_LOW  2
#define WEST_LOW   3
#define NORTH_HIGH 4
#define EAST_HIGH  5
#define SOUTH_HIGH 6
#define WEST_HIGH  7

#endif