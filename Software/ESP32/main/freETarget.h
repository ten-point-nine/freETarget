/*----------------------------------------------------------------
 *
 * freETarget.h
 *
 * Software to run the Air-Rifle / Small Bore Electronic Target
 *
 *--------------------------------------------------------------*/

#ifndef _FREETARGET_H
#define _FREETARGET_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "serial_io.h"

#ifdef FREETARGET_C
#define EXTERN
#else
#define EXTERN extern
#endif

#define SOFTWARE_VERSION "\"6.1.19 August 21, 2025\""
#define _DONE_           "\r\nDone\r\n"
#define _GREETING_       "CONNECTED" // Message to send on connection

#define REV_500   500                // ESP32
#define REV_510   510
#define REV_520   520
#define BUILD_REV REV_520

#define INIT_DONE 0xabcd             // NON-VOL Initialization complete signature
#ifndef true
#define true  (1 == 1)
#define false (0 == 1)
#endif
#define CLOCK_TEST false

#define IN_STARTUP   0x0001 // The software is in initialization
#define IN_OPERATION 0x0002 // The software is operational
#define IN_TEST      0x0004 // A self test has been selected (Suspend operation)
#define IN_SLEEP     0x0008 // The unit has powered down
#define IN_SHOT      0x0010 // The target is actively in a shot
#define IN_REDUCTION 0x0020 // The data is being reduced
#define IN_FATAL_ERR 0x0040 // A fatal error has occured and cannot be fixed
#define IN_HTTP      0x0080 // HTTP output enabled

#define IF_NOT(x) if ( (run_state & (x)) == 0 )
#define IF_IN(x)  if ( (run_state & (x)) != 0 )

#define SEND(who, message) {message} serial_to_all(_xs, who);

#define IS_DC_WITNESS      (json_paper_time != 0) // Determine the witness paper drive (DC Motor)
#define IS_STEPPER_WITNESS (json_step_count != 0) // Determine the witness paper drive (stepper)

#define JSON_NAME_TEXT 99                         // Use 99 to identify a user named target

/*
 * Options
 */
#define SAMPLE_CALCULATIONS  (1 == 0)    // Trace the COUNTER values
#define COMPENSATE_RISE_TIME (1 == 0)    // Use PCNT4-7 to compensate for rise time
#define LONG_TEXT            (512 + 256) // Long text strings are 512 long
#define MEDIUM_TEXT          256         // Medimum length strings are 256 long
#define SHORT_TEXT           128         // Short text strings are 128 long
#define TINY_TEXT            64          // Tiny text strings are 64 long

/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ 10.0                        // 10,000 cycles in 1 ms
#define CLOCK_PERIOD   (1.0 / OSCILLATOR_MHZ)      // Seconds per bit
#define ONE_SECOND     (100)                       // 10 ms delay per LSB
#define TICK_10ms      (1)                         // Minimum timeout 10ms
#define FULL_SCALE     0xffffffff                  // Full scale timer
#define MS_TO_TICKS(x) (ONE_SECOND * (x) / 1000)   // Convert from time in ms to time ticks

#define SHOT_TIME  ((int)(json_sensor_dia / 0.33)) // Worst case delay (microseconds) = sensor diameter / speed of sound)
#define SHOT_SPACE 100                             // 40 Sighters + 60 on Score

#define HI(x)    (((x) >> 8) & 0x00ff)             // High nibble
#define LO(x)    ((x) & 0x00ff)                    // Low nibble
#define HHH10(x) (((x) / 10000) % 10)              // Highest digit    2xxxx
#define HHI10(x) (((x) / 1000) % 10)               // High High digit  x2xxx
#define HLO10(x) (((x) / 100) % 10)                // High Low digit   xx2xx
#define HI10(x)  (((x) / 10) % 10)                 // High digit       xxx2x
#define LO10(x)  ((x) % 10)                        // Low digit        xxxx2

#define N    0                                     // Index to North Timer
#define E    1                                     // Index to East Timer
#define S    2                                     // Index to South Timer
#define W    3                                     // Index to West Timer
#define N_HI 4
#define E_HI 5
#define S_HI 6
#define W_HI 7

#define MISS 9                                     // Timer was a miss

#define PI      3.14159269
#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define SESSION_EMPTY 0                            // No session data
#define SESSION_VALID 1                            // Session is valid but undefined
#define SESSION_SIGHT 2                            // Session is a sighter
#define SESSION_MATCH 4                            // Session is a match
#define SESSION_PRINT 10                           // Print out the session

#define SCORE_LEFT_BRACE  '{'                      // Opening JSON string
#define SCORE_RIGHT_BRACE '}'                      // Closing JSON string
#define SCORE_NEW_LINE    'n'                      // Add a newline
#define SCORE_PRIME       '#'                      // Prime a reply to the client
#define SCORE_SHOT        'S'                      // Include shot number
#define SCORE_MISS        'M'                      // Include miss status
#define SCORE_SESSION     '?'                      // Include session type
#define SCORE_TIME        'T'                      // Include time stamp
#define SCORE_ELAPSED     'D'                      // Include elapsed time
#define SCORE_XY          'X'                      // Include X-Y coordinates
#define SCORE_POLAR       'P'                      // Include polar coordinates
#define SCORE_HARDWARE    'H'                      // Include hardware values
#define SCORE_TARGET      'O'                      // Include target name
#define SCORE_EVENT       'E'                      // Include the athelte name
#define SCORE_TEST        '$'                      // Test the client with a test shot

#define SCORE_ALL        "{S?TXPHOE}"              // shot / miss / target / time / x-y / radius-angle / North-East-South-West / target type
#define SCORE_USB        "{S?TX}"                  // USB score elements
#define SCORE_TCPIP      "{S?TXE}"                 // TCP score elements
#define SCORE_BLUETOOTH  "{S?TX}"                  // Bluetooth score elements
#define SCORE_HTTP       "{S?TXPOE}"               // HTTP score elements
#define SCORE_HTTP_PRIME "{#}"                     // HTTP Prime the client
#define SCORE_HTTP_TEST  "{$}"                     // HTTP Test the client

#define HTTP_CLOSE_TIME 15l                        // Time to close the HTTP connection after the last shot

/*
 *  Types
 */
typedef struct sensor_ID
{
  char         short_name;      // Short name, ex 'N'
  char        *long_name;       // Long name, ex "NORTH_HI"
  char        *diag_LED;        // LEDs to be set if a fault occurs
  unsigned int sensor_GPIO;     // What GPIO is used with this sensor
  unsigned int run_mask;        // What bit is set in the RUN latch
} sensor_ID_t;

typedef struct
{
  unsigned int index;           // Which sensor is this one
  sensor_ID_t  low_sense;       // Information about the low trip point
  sensor_ID_t  high_sense;      // Information about the high trip point
  bool         is_valid;        // TRUE if the sensor contains a valid time
  double       angle_A;         // Angle to be computed
  double       diagonal;        // Diagonal angle to next sensor (45')
  double       x;               // Sensor Location (X us)
  double       y;               // Sensor Location (Y us)
  double       count;           // Working timer value
  double       a, b, c;         // Working dimensions
  double       xs;              // Computed X shot value
  double       ys;              // Computed Y shot value
} sensor_t;

typedef struct
{
  unsigned int  shot;           // Shot number associated with this record, may not be 1:1
  unsigned int  miss;
  unsigned int  session_type;   // What kind of information is contained in the score
  double        x;              // X location of shot as computed in clock cycles
  double        y;              // Y location of shot as computed in clock cycles
  double        xs;             // X location of shot rotated onto target in clock cycles
  double        ys;             // Y location of shot rotated onto target in clock cycles
  double        x_mm;           // Computed location in mm
  double        y_mm;           // Computed location in mm
  double        radius;         // Radius of shot from the center of the target
  double        angle;          // Angle of shot from the center of the target
  int           timer_count[8]; // Array of timer values 4 in hardware and 4 in software
  unsigned int  face_strike;    // Recording of face strike
  unsigned int  sensor_status;  // Triggering register
  unsigned long shot_time;      // Shot time since start of after tabata start
} shot_record_t;

typedef unsigned char          byte_t;
typedef volatile unsigned long time_count_t;

/*
 *  Global Variables
 */
EXTERN char                  _xs[LONG_TEXT];                        // General purpose string buffer
EXTERN double                s_of_sound;
EXTERN unsigned int          face_strike;
EXTERN unsigned int          is_trace;                              // Tracing level(s)
EXTERN unsigned int          shot_in;                               // Index into the shot array (The shot that has JUST arrived)
EXTERN unsigned int          shot_out;                              // Index into the shot array (Last shot processed)
EXTERN unsigned int          shot_number;                           // Current shot number
EXTERN unsigned long         shot_start;                            // Time when shot become valid
EXTERN time_count_t          power_save;                            // Power down timer
EXTERN volatile unsigned int run_state;                             // IPC states
EXTERN time_count_t          LED_timer;                             // Turn off the LEDs when not in use
EXTERN time_count_t          keep_alive;                            // Keep alive timer
EXTERN time_count_t          power_save;                            // Power save timer
EXTERN time_count_t          time_since_last_shot;                  // 15 minutes since last shot
EXTERN time_count_t          session_time[];                        // Time in each session
EXTERN shot_record_t         record[SHOT_SPACE];
#ifdef FREETARGET_C
EXTERN char        *yes_no[]       = {"No", "Yes"};                 // Yes or No
EXTERN time_count_t session_time[] = {1000 * 60, 15 * 60, 75 * 60}; // Time in each session EMPTY, SIGHT, SCORE // Array of shot records
#else
EXTERN char        *yes_no[]; // Yes or No strings
EXTERN time_count_t session_time[];
#endif

/*
 * FreeETarget functions
 */
void         freeETarget_init(void);              // Get the target software ready
void         freeETarget_target_loop(void *arg);  // Target polling loop
void         send_keep_alive(void);               // Send out the keep alive signal for TCPIP
void         hello(void);                         // Say Hello World
void         bye(unsigned int force_bye);         // Shut down and say goodbye
void         polled_target_test(void);            // Test the target aquisition software
void         interrupt_target_test(void);         // Test the target aquisition software
void         tabata_task(void);                   // Run the TABATA timersArm the Tabata counter
void         rapid_fire_task(void);               // Run the Rapid Fire state machine
sensor_ID_t *find_sensor(unsigned int run_mask);  // Locate the sensor settings for the run_latch
void         start_new_session(int session_type); // Start a new shooting session
bool         prompt_for_confirm(void);            // Prompt for a confirmation

#endif
