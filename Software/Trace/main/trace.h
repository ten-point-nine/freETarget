/*----------------------------------------------------------------
 *
 * trace.h
 *
 * IMU for guns to work with FreeETarget
 *
 *--------------------------------------------------------------*/

#ifndef _TRACE_H
#define _TRACE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "serial_io.h"

#ifdef TRACE_C
#define EXTERN
#else
#define EXTERN extern
#endif

#define SOFTWARE_VERSION "\"1.0.0 May 17, 2026\""
#define _DONE_           "\r\nDone\r\n"
#define _GREETING_       "CONNECTED"   // Message to send on connection
#define _BYE_            "BYE"         // Message to send on disconnection
#define _HELLO_          "HELLO WORLD" // Message to send on reconnection

#define INIT_DONE 0xabcd               // NON-VOL Initialization complete signature
#ifndef true
#define true  (1 == 1)
#define false (0 == 1)
#endif

#define IN_STARTUP    0x0001           // The software is in initialization
#define IN_OPERATION  0x0002           // The software is operational
#define IN_TEST       0x0004           // A self test has been selected (Suspend operation)
#define IN_COLLECTION 0x0008           // Collecting data
#define IN_SINGLE     0x0010           // Reading a single sample directly from BMK270
#define IN_REDUCTION  0x0020           // The data is being reduced
#define IN_FATAL_ERR  0x0040           // A fatal error has occured and cannot be fixed

#define IF(x)     if ( (run_state & (x)) != 0 )
#define IF_NOT(x) if ( (run_state & (x)) == 0 )

#define SEND(who, message) {message} serial_to_all(_xs, who);

#define IS_DC_WITNESS      (json_paper_time != 0) // Determine the witness paper drive (DC Motor)
#define IS_STEPPER_WITNESS (json_step_count != 0) // Determine the witness paper drive (stepper)

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
#define ONE_SECOND     (100)                     // 10 ms delay per LSB
#define TICK_10ms      (1)                       // Minimum timeout 10ms
#define FULL_SCALE     0xffffffff                // Full scale timer
#define MS_TO_TICKS(x) (ONE_SECOND * (x) / 1000) // Convert from time in ms to time ticks

#define PI      3.14159269
#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)
#define TWO_PI  (2.0d * PI)

#define SCORE_LEFT_BRACE  '{'         // Opening JSON string
#define SCORE_RIGHT_BRACE '}'         // Closing JSON string
#define SCORE_NEW_LINE    'n'         // Add a newline
#define SCORE_PRIME       '#'         // Prime a reply to the client
#define SCORE_SHOT        'S'         // Include shot number
#define SCORE_MISS        'M'         // Include miss status
#define SCORE_SESSION     '?'         // Include session type
#define SCORE_TIME        'T'         // Include time stamp
#define SCORE_ELAPSED     'D'         // Include elapsed time
#define SCORE_XY          'X'         // Include X-Y coordinates
#define SCORE_POLAR       'P'         // Include polar coordinates
#define SCORE_HARDWARE    'H'         // Include hardware values
#define SCORE_TARGET      'O'         // Include target name
#define SCORE_EVENT       'E'         // Include the athelte name
#define SCORE_TEST        '$'         // Test the client with a test shot

#define SCORE_ALL        "{S?TXPHOE}" // shot / miss / target / time / x-y / radius-angle / North-East-South-West / target type
#define SCORE_USB        "{S?TX}"     // USB score elements
#define SCORE_TCPIP      "{S?TXE}"    // TCP score elements
#define SCORE_BLUETOOTH  "{S?TX}"     // Bluetooth score elements
#define SCORE_HTTP       "{S?TXPOE}"  // HTTP score elements
#define SCORE_HTTP_PRIME "{#}"        // HTTP Prime the client
#define SCORE_HTTP_TEST  "{$}"        // HTTP Test the client
#define SCORE_SEND_MISS  "{SMT}n"     // Send a miss

#define HTTP_CLOSE_TIME 15l           // Time to close the HTTP connection after the last shot

                                      /*
                                       *  Memory Calculations (needed for memory allocation)
                                       *
                                       *  A frame is a single sample read from the ACCEL/GYRO
                                       *  A sample buffer is the space to store a single FIFO pull
                                       *  A trace is the path drawn by the gun on the target
                                       *  A vector is the locaton and direction of a sample
                                       */
#define AVAILABLE_FIFO (6 * 1024)                                          // (6144) 6K FIFO available
#define WATERMARK      (AVAILABLE_FIFO * 3 / 4)                            // (4608) Use 75% of the FIFO

#define RAW_FRAME_SIZE  (6 * 2)                                            // (12)   6 entries @ 2 bytes per entry
#define RAW_FRAME_COUNT (WATERMARK / RAW_FRAME_SIZE)                       // (384)  entries in the FIFO

#define SAMPLE_RATE   (800)                                                // 400 samples per second, 2.5ms / sample
#define SAMPLE_PERIOD (10)                                                 // Accumulate sampls for 10 seconds
#define SAMPLE_BUFFER_COUNT                                                                                                                \
  ((SAMPLE_RATE * SAMPLE_PERIOD) / RAW_FRAME_COUNT) + 2                    // (12) Number of frames needed to store 10 seconds of data

#define VECTOR_FRAME_SIZE  (6 * 4)                                         // (24) 6 entries at 4 bytes (32 bits) each
#define VECTOR_BUFFER_SIZE (SAMPLE_RATE * SAMPLE_PERIOD)                   // Memory used to store vectors

#define TRACE_FRAME_SIZE  (2 * 4)                                          // (24) 6 entries at 4 bytes (32 bits) each
#define TRACE_MEMORY_SIZE (SAMPLE_RATE * SAMPLE_PERIOD * TRACE_FRAME_SIZE) // (96000)

/*
 *  Types
 */
typedef unsigned char byte_t;
typedef volatile long time_count_t;
typedef float         real_t;

typedef struct
{
  real_t x_dotdot;  // X-axis acceleration in g
  real_t y_dotdot;  // Y-axis acceleration in g
  real_t z_dotdot;  // Z-axis acceleration in g
  real_t x_dot;     // Velocity in the X axis
  real_t y_dot;     // Velocity in the Y axis
  real_t z_dot;     // Velocity in the Z axis
  real_t x;         // X position
  real_t y;         // Y position
  real_t z;         // Z position
  real_t rho_dot;   // X anglular velocity
  real_t theta_dot; // Y anglular velocity
  real_t phi_dot;   // Z anglular velocity
} trace_vector_t;   // Vector at the point

typedef struct
{
  real_t x;         // X position
  real_t y;         // Y position
} trace_point_t;    // computed point
/*
 *  Global Variables
 */
EXTERN trace_vector_t trace_vector[2];                              // Space for the trace vector
EXTERN trace_point_t   trace_point[SAMPLE_RATE * SAMPLE_PERIOD];     // Space for the trace
EXTERN char           _xs[1024 + 512];                              // General purpose string buffer
EXTERN unsigned int   is_trace;                                     // Tracing level(s)

EXTERN unsigned int board_revision;                                 // Board revision number
EXTERN time_count_t shot_start;                                     // Time when shot become valid
EXTERN time_count_t LED_timer;                                      // Turn off the LEDs when not in use
EXTERN time_count_t keep_alive;                                     // Keep alive timer
EXTERN time_count_t power_save;                                     // Power save timer
                                                                    // 15 minutes since last shot
EXTERN time_count_t session_time[];                                 // Time in each session
EXTERN unsigned int run_state;                                      // Current running state of the software

EXTERN int           sample_in;                                     // Index to entry from sensor (<0 - wraps around)
EXTERN int           sample_out;                                    // Index to output to application  (<0 - wraps around)
EXTERN trace_point_t present;                                       // Present sample
EXTERN trace_point_t previous;                                      // Prior sample

#ifdef TRACE_C
EXTERN char        *no_yes[]       = {"No", "Yes"};                 // Yes or No
EXTERN time_count_t session_time[] = {1000 * 60, 15 * 60, 75 * 60}; // Time in each session EMPTY, SIGHT, SCORE // Array of shot records
#else
EXTERN char        *no_yes[]; // Yes or No strings
EXTERN time_count_t session_time[];
#endif

/*
 * trace functions
 */
void trace_init(void);         // Get the target software ready
void trace_loop(void *arg);    // Target polling loop
void send_keep_alive(void);    // Send out the keep alive signal for TCPIP
bool prompt_for_confirm(void); // Prompt for a confirmation
#endif
