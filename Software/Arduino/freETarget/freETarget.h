
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

#define SOFTWARE_VERSION "\"3.08.2 May 22, 2022\""
#define REV_100    100
#define REV_210    210
#define REV_220    220
#define REV_290    290
#define REV_300    300
#define REV_310    310

#define INIT_DONE       0xabcd        // Initialization complete signature
#define T(s)            F(s)          // Move text string to the flash segment

/*
 * Three way Serial Port
 */
#define SAMPLE_CALCULATIONS false     // Trace the COUNTER values
#define AUX_SERIAL         Serial3    // Auxilary Connector
#define WIFI_SERIAL        Serial3    // WiFi Port
#define DISPLAY_SERIAL     Serial2    // Serial port for slave display

char GET (void) 
{
  if ( Serial.available() )
  {
    return Serial.read(); 
  }
  if ( esp01_available() )
  {
    return esp01_read();
  }
  if ( DISPLAY_SERIAL.available() )
  {
    return DISPLAY_SERIAL.read();
  }
  return 0;
}
                   
#define AVAILABLE ( Serial.available() | esp01_available() | DISPLAY_SERIAL.available() )

#define PORT_SERIAL   1
#define PORT_AUX      2
#define PORT_DISPLAY  4
#define PORT_ALL      (PORT_SERIAL + PORT_AUX + PORT_DISPLAY)

/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ   8.0                          // 8000 cycles in 1 ms
#define CLOCK_PERIOD  (1.0/OSCILLATOR_MHZ)            // Seconds per bit
#define ONE_SECOND      1000                          // 1000 ms delay
#define SHOT_TIME     ((int)(json_sensor_dia / 0.33)) // Worst case delay Sensor diameter / speed of sound)

#define HI(x) (((x) >> 8 ) & 0x00ff)                  // High nibble
#define LO(x) ((x) & 0x00ff)                          // Low nibble
#define HHH10(x) (((x) / 10000 ) % 10)                // Highest digit    2xxxx
#define HHI10(x) (((x) / 1000 ) % 10)                 // High High digit  x2xxx
#define HLO10(x) (((x) / 100 ) % 10)                  // High Low digit   xx2xx
#define HI10(x)  (((x) / 10 ) % 10)                   // High digit       xxx2x
#define LO10(x)  ((x) % 10)                           // Low digit        xxxx2

#define N 0
#define E 1
#define S 2
#define W 3

struct record
{
  unsigned int shot;          // Current shot number
  double       x;             // X location of shot
  double       y;             // Y location of shot
  long         shot_time;     // Shot time after tabata start
};

typedef struct record this_shot;

struct GPIO {
  byte port;
  char* gpio_name;
  byte in_or_out;
  byte value;
};

typedef struct GPIO GPIO_t;
extern const GPIO init_table[];

extern double  s_of_sound;

extern const char* names[];
extern const char to_hex[];
extern bool  face_strike;
extern bool  is_trace;                // True if tracing is enabled 
extern const char nesw[];             // Cardinal Points
extern bool  target_hot;              // True if the target is active

/*
 *  Factory settings via Arduino monitor
 */
/*
#define FACTORY        {"NAME_ID":1, "TRGT_1_RINGx10":1550, "ECHO":2}
#define FACTORY_BOSS   {"NAME_ID":1, "TRGT_1_RINGx10":1550, "ECHO":2}
#define FACTORY_MINION {"NAME_ID":2, "TRGT_1_RINGx10":1550, "ECHO":2}
#define SERIAL_NUMBER  {"NAME_ID":1, "TRGT_1_RINGx10":1550, "SN":1234, "ECHO":2}
#define LONG_TEST      {"SENSOR":231, "Z_OFFSET":5, "STEP_TIME":50, "STEP_COUNT":0, "NORTH_X":0, "NORTH_Y":0, "EAST_X":0, "EAST_Y":0, "SOUTH_X":0, "SOUTH_Y":0, "WEST_X":0, "WEST_Y":0, "LED_BRIGHT":50, "NAME_ID":0, "ECHO":9}
*/

#endif
