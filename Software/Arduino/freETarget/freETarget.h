
/*----------------------------------------------------------------
 * 
 * freETarget.h
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
#ifndef _FREETARGET_H
#define _FREETARGET_H

#define SOFTWARE_VERSION "\"2.99.0 January 24, 2021\""
#define REV_21    21
#define REV_22    22
#define REV_29    29

#define INIT_TRIP_POINT     1250      // Set the trip point to 1250 mV
#define INIT_TRIP_POINT_299 1750      // Set the trip point to 1750 mV

#define INIT_DONE       0xabcd        // Initialization complete signature

/*
 * Compilation Flags
 */
#define SAMPLE_CALCULATIONS false     // Trace the COUNTER values
#define AUX_SERIAL        Serial2     // Auxilary Connector
#define MINION_SERIAL     Serial3     // Version 2.99 debug 



/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ   8.0    // 8000 cycles in 1 ms
#define CLOCK_PERIOD  (1.0/OSCILLATOR_MHZ)            // Seconds per bit
#define ONE_SECOND      1000                          // 1000 ms delay
#define SHOT_TIME     ((int)(json_sensor_dia / 0.33)) // Worst case delay Sensor diameter / speed of sound)

/*
 * DIP Switch enabled tests.  Set (0<<x) to (1<<x) to enable always
 */
//                      From DIP    From Software
#define CALIBRATE       ((1 << 0) + (0 << (4 + 0)))     // 1 Go to Calibration Mode
#define VERBOSE_TRACE   ((1 << 1) + (0 << (4 + 1)))     // 2 Show the verbose software trace
#define BOSS            ((1 << 2) + (0 << (4 + 2)))     // 4 Master processor in Version 2.99
#define VERSION_2       ((1 << 3) + (0 << (4 + 3)))     // 8 Override Version 2.99 programming
//#define FACTORY         ((1 << 3) + (0 << (4 + 3)))     // 8 Reset all settings to factory defaults
#define FACTORY         0                               // Patched out for Version 2.99

#define HI(x) (((x) >> 8 ) & 0x00ff)
#define LO(x) ((x) & 0x00ff)

#define N 0
#define E 1
#define S 2
#define W 3

struct history
{
  unsigned int shot;    // Current shot number
  double       x;       // X location of shot
  double       y;       // Y location of shot
};

typedef struct history history_t;

extern double     s_of_sound;

extern char* names[];

/*
 *  Factory settings via Arduino monitor
 * /
 */

#define FACTORY_BOSS   {"NAME_ID":2, "TRGT_1_RINGx10":1550, "TRIP_POINT":1750, "ECHO":2}
#define FACTORY_MINION {"NAME_ID":2, "TRGT_1_RINGx10":1550, "TRIP_POINT":1750, "ECHO":2}

#endif
