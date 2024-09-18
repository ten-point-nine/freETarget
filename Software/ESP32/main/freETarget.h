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
#ifndef _FREETARget_H
#define _FREETARget_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SOFTWARE_VERSION "\"5.2.6 September 17, 2024\""

#define REV_500    500   // ESP32
#define REV_510    510
#define REV_520    520
#define BUILD_REV  REV_520

#define INIT_DONE       0xabcd                    // NON-VOL Initialization complete signature
#ifndef true
#define true        (1==1)
#define false       (0==1)
#endif
#define CLOCK_TEST   false

#define IN_STARTUP    0x0001                      // The software is in initialization
#define IN_OPERATION  0x0002                      // The software is operational
#define IN_TEST       0x0004                      // A self test has been selected (Suspend operation)
#define IN_SLEEP      0x0008                      // The unit has powered down 
#define IF_NOT(x) if ( (run_state & (x)) == 0) 
#define IF_IN(x)  if ( (run_state & (x)) != 0) 

#define SEND(message) {message} serial_to_all(_xs, ALL);

#define IS_DC_WITNESS      (json_paper_time != 0) // Determine the witness paper drive (DC Motor)
#define IS_STEPPER_WITNESS (json_step_count != 0) // Determine the witness paper drive (stepper)

/*
 * Options
 */
#define SAMPLE_CALCULATIONS  (1==0)                   // Trace the COUNTER values
#define COMPENSATE_RISE_TIME (1==1)                   // Use PCNT4-7 to compensate for rise time

/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ   10.0                         // 10,000 cycles in 1 ms
#define CLOCK_PERIOD  (1.0/OSCILLATOR_MHZ)            // Seconds per bit
#define ONE_SECOND    (100)                           // 10 ms delay per LSB
#define MIN_DELAY     (1)                             // Minimum timeout 10ms
#define FULL_SCALE      0xffffffff                    // Full scale timer
#define MS_TO_TICKS(x) (ONE_SECOND * (x) / 1000)      // Convert from time in ms to time ticks

#define SHOT_TIME     ((int)(json_sensor_dia / 0.33)) // Worst case delay (microseconds) = sensor diameter / speed of sound)
#define SHOT_SPACE   100                             // 40 Sighters + 60 on Score

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
#define N_HI    4
#define E_HI    5
#define S_HI    6
#define W_HI    7

#define MISS    9                                     // Timer was a miss

#define PI 3.14159269
#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

/*
 *  Types
 */
struct sensor_ID
{
  char         short_name;  // Short name, ex 'N'
  char*        long_name;   // Long name, ex "NORTH_HI"
  char*        diag_LED;    // LEDs to be set if a fault occurs
  unsigned int run_mask;    // What bit is set in the RUN latch
};

typedef struct sensor_ID sensor_ID_t;

struct sensor
{
  unsigned int index;       // Which sensor is this one
  sensor_ID_t  low_sense;   // Information about the low trip point
  sensor_ID_t  high_sense;  // Information about the high trip point
  bool is_valid;            // TRUE if the sensor contains a valid time
  double angle_A;           // Angle to be computed
  double diagonal;          // Diagonal angle to next sensor (45')
  double x;                 // Sensor Location (X us)
  double y;                 // Sensor Location (Y us)
  double count;             // Working timer value
  double a, b, c;           // Working dimensions
  double xs;                // Computed X shot value
  double ys;                // Computed Y shot value
};

typedef struct sensor sensor_t; // Sensor information

typedef unsigned char byte_t;

struct shot_r
{
  bool         is_valid;        // This record contains valid information
  double       x;               // X location of shot as computed
  double       y;               // Y location of shot
  double       xs;              // X location of shot as scored
  double       ys;              // Y location of shot
           int timer_count[8];  // Array of timer values 4 in hardware and 4 in software
  unsigned int face_strike;     // Recording of face strike
  unsigned int sensor_status;   // Triggering register
  unsigned long shot_time;      // Shot time since start of after tabata start
};

typedef struct shot_r shot_record_t;
extern shot_record_t record[SHOT_SPACE];      //Array of shot records


/*
 *  Global Variables
 */
extern double        s_of_sound;
extern const char*   names[];
extern const char    to_hex[];
extern unsigned int  face_strike;
extern unsigned int  is_trace;                // Tracing level(s)
extern unsigned int  shot_in;      // Index into the shot array (The shot that has JUST arrived)
extern unsigned int  shot_out;            // Index into the shot array (Last shot processed)
extern unsigned int  shot_number;
extern volatile unsigned long power_save;     // Power down timer
extern volatile unsigned int  run_state;      // IPC states 
extern volatile unsigned long LED_timer;      // Turn off the LEDs when not in use
extern char _xs[512];                         // General purpose string buffer

/* 
 * FreeETarget functions
 */
void  freeETarget_init(void);                            // Get the target software ready
void  freeETarget_target_loop(void* arg);                // Target polling loop
void  send_keep_alive(void);                             // Send out the keep alive signal for TCPIP
void  hello(void);                                       // Say Hello World
void  bye(void);                                         // Shut down and say goodbye
void  tabata_enable(int enable);                         // Arm the Tabata counters
void  polled_target_test(void);                          // Test the target aquisition software
void  interrupt_target_test(void);                       // Test the target aquisition software
void  tabata_task(void);                                 // Run the TABATA timersArm the Tabata counter
void  rapid_fire_task(void);                             // Run the Rapid Fire state machine
sensor_ID_t* find_sensor(unsigned int run_mask);         // Locate the sensor settings for the run_latch
void  start_new_session(void);                           // Start a new shooting session

#endif
