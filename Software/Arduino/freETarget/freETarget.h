
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
#include "esp-01.h"
#include "json.h"

#define SOFTWARE_VERSION "\"3.02.3 July 22, 2021\""
#define REV_100    100
#define REV_210    210
#define REV_220    220
#define REV_290    290
#define REV_300    300

#define INIT_DONE       0xabcd        // Initialization complete signature

/*
 * Three way Serial Port
 */
#define SAMPLE_CALCULATIONS false     // Trace the COUNTER values
#define AUX_SERIAL         Serial3    // Auxilary Connector
#define DISPLAY_SERIAL     Serial2    // Serial port for slave display

#define PRINT(x)  {Serial.print(x); AUX_SERIAL.print(x); DISPLAY_SERIAL.print(x);}




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

extern double  s_of_sound;

extern const char* names[];
extern bool  face_strike;
extern bool  is_trace;                // True if tracing is enabled 
extern char* nesw;                    // Cardinal Points

/*
 *  Factory settings via Arduino monitor
 */
/*
#define FACTORY        {"NAME_ID":1, "TRGT_1_RINGx10":1550, "ECHO":2}
#define FACTORY_BOSS   {"NAME_ID":1, "TRGT_1_RINGx10":1550, "ECHO":2}
#define FACTORY_MINION {"NAME_ID":2, "TRGT_1_RINGx10":1550, "ECHO":2}
#define SERIAL_NUMBER  {"NAME_ID":1, "TRGT_1_RINGx10":1550, "SN":1234, "ECHO":2}
*/
#endif
