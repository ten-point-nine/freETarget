/*----------------------------------------------------------------
 * 
 * freETarget.h
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 *
 */
#ifndef _FREETARGET_H
#define _FREETARGET_H
#include "esp-01.h"
#include "json.h"
#include "token.h"

#define RESCUE     (1==0)

#if ( RESCUE )
#define SOFTWARE_VERSION "\"RESCUE March 1, 2023\""
#else
#define SOFTWARE_VERSION "\"4.2.2 August 2, 2023\""
#endif

#define REV_100    100
#define REV_210    210
#define REV_220    220
#define REV_290    290
#define REV_300    300   // Third Generation
#define REV_310    310   // Onboard Hi Pass Filter
#define REV_320    320   // No USB adapter

#define INIT_DONE       0xabcd                    // Initialization complete signature
#define T(s)            F(s)                      // Move text string to the flash segment

/*
 * Options
 */
#define SAMPLE_CALCULATIONS false                 // Trace the COUNTER values

/*
 * Tracing 
 */
#define DLT(level)      ( do_dlt(level) )
#define DLT_NONE          0                       // No DLT messages displayed
#define DLT_CRITICAL      0x80                    // Display messages that will compromise the target
#define DLT_APPLICATION   0x01                    // Application level messages displayed
#define DLT_DIAG          0x02                    // Diagnostics messages displayed
#define DLT_INFO          0x04                    // Informational messages

/*
 * Three way Serial Port
 */
#define AUX_SERIAL         Serial3    // Auxilary Connector
#define DISPLAY_SERIAL     Serial2    // Serial port for slave display

/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ   8.0                          // 8000 cycles in 1 ms
#define CLOCK_PERIOD  (1.0/OSCILLATOR_MHZ)            // Seconds per bit
#define ONE_SECOND      1000L                         // 1000 ms delay 
#define ONE_SECOND_US   1000000u                      // One second in us
#define FULL_SCALE      0xffffffff                    // Full scale timer


#define SHOT_TIME     ((int)(json_sensor_dia / 0.33)) // Worst case delay (microseconds) = sensor diameter / speed of sound)
#define SHOT_STRING   20                              // Allow a maximum of SHOT_STRING for rapid fire

#define HI(x) (((x) >> 8 ) & 0x00ff)                  // High nibble
#define LO(x) ((x) & 0x00ff)                          // Low nibble
#define HHH10(x) (((x) / 10000 ) % 10)                // Highest digit    2xxxx
#define HHI10(x) (((x) / 1000 ) % 10)                 // High High digit  x2xxx
#define HLO10(x) (((x) / 100 ) % 10)                  // High Low digit   xx2xx
#define HI10(x)  (((x) / 10 ) % 10)                   // High digit       xxx2x
#define LO10(x)  ((x) % 10)                           // Low digit        xxxx2

#define N       0                                     // Index to North Timer
#define E       1                                     // Index to East Timer
#define S       2                                     // Index to South Timer
#define W       3                                     // Index to West Timer
#define MISS    4                                     // Timer was a miss

struct shot_r
{
  unsigned int shot_number;     // Current shot number
  double       xphys_mm;        // Physical X location of shot (in mm)
  double       yphys_mm;        // Physical Y location of shot (in mm)
  unsigned int timer_count[4];  // Array of timer values
  unsigned int face_strike;     // Recording of face strike
  unsigned int sensor_status;   // Triggering register
  unsigned long shot_time;      // Shot time since start of after tabata start
};

typedef struct shot_r shot_record_t;

struct GPIO {
  byte port;
  char* gpio_name;
  byte in_or_out;
  byte value;
};

typedef struct GPIO GPIO_t;
extern const GPIO init_table[];

extern double  s_of_sound;

extern const char* namesensor[];
extern const char to_hex[];
extern unsigned int face_strike;
extern const char nesw[];             // Cardinal Points
extern shot_record_t record[];

/*----------------------------------------------------------------
 * 
 * function: soft_reset
 * 
 * brief:    Reset the board
 * 
 * return:   Never
 * 
 *----------------------------------------------------------------
 *
 * When the keep alive expires, send a new one out and reset.
 * 
 * It is sent out to the USB port as a diagnostic check 
 * 
 *--------------------------------------------------------------*/
static void(* soft_reset) (void) = 0;

#endif
