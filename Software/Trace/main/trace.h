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

#define SOFTWARE_VERSION "\"1.0.0 June 3, 2026\""
#define _DONE_           "\r\nDone\r\n"
#define _GREETING_       "CONNECTED"           // Message to send on connection
#define _BYE_            "BYE"                 // Message to send on disconnection
#define _HELLO_          "HELLO WORLD"         // Message to send on reconnection

#define INIT_DONE 0xabcd                       // NON-VOL Initialization complete signature
#ifndef true
#define true  (1 == 1)
#define false (0 == 1)
#endif

#define IN_STARTUP      0x0001                 // The software is in initialization
#define IN_NO_CAL       (IN_STARTUP << 1)      // The unit has not been calibrated
#define IN_FIFO_FILLING (IN_NO_CAL << 1)       // The FIFO is filling up
#define IN_SINGLE       (IN_FIFO_FILLING << 1) // Reading a single sample directly from BMK270
#define IN_REDUCTION    (IN_SINGLE << 1)       // The data is being reduced
#define IN_OPERATION    (IN_REDUCTION << 1)    // FIFO has data, unit has been zeroed
#define IN_FATAL_ERROR  (IN_OPERATION << 1)    // A fatal error has occured and cannot be fixed
#define IN_TEST         (IN_FATAL_ERROR << 1)  // Running a test

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
#define TICK_10ms  (1)                           // Minimum timeout 10ms
#define TICK_50ms  (5 * TICK_10ms)               // Minimum timeout 10ms
#define ONE_SECOND (100 * TICK_10ms)             // 10 ms delay per LSB

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
#define AVAILABLE_FIFO (6 * 1024)                                         // (6144) 6K FIFO available

#define RAW_FRAME_SIZE  (6 * 2)                                           // (12)   6 entries @ 2 bytes per entry
#define RAW_FRAME_COUNT (400)                                             // (400)  entries in the FIFO

#define WATERMARK (RAW_FRAME_SIZE * (RAW_FRAME_COUNT + 2))                // (4800 + 2 sample buffer) Use 75% of the FIFO

#if ( WATERMARK > (AVAILABLE_FIFO * 8 / 10) )                             // If the watermark is over 80% of the FIFO
#error "WATERMARK IS TOO HIGH"
#endif

#define APPROACH       5                                                  // Go back in time 5 seconds
#define FOLLOW_THROUGH 2                                                  // Go forwards 2 seconds
#define OVERSAMPLE     (SAMPLE_RATE / TRACE_RATE)                         // Only send 1/8 samples

#define SAMPLE_RATE   (1600)                                              // Output Data Rate samples per second
#define SAMPLE_PERIOD (APPROACH + FOLLOW_THROUGH)                         // Accumulate sampls for 8 seconds
#define SAMPLE_BUFFER_COUNT                                                                                                                \
  (((SAMPLE_RATE * SAMPLE_PERIOD) / RAW_FRAME_COUNT) + 1)                 // Number of frames needed to store the approach and follow through

#define TRACE_RATE        (100)                                           // Trace points per
#define TRACE_FRAME_SIZE  (2 * 4)                                         // (24) 6 entries at 4 bytes (32 bits) each
#define TRACE_MEMORY_SIZE (TRACE_RATE * SAMPLE_PERIOD * TRACE_FRAME_SIZE) // (96000)

/*
 *  Types
 */
typedef unsigned char     byte_t;
typedef volatile long int time_count_t;
typedef float             real_t;

/*
 * A single sample frame read from the BMI270 from each of the registers.
 * Internally, the target software will use the FIFO format for operations.
 * Any function using the register_raw format will need to convert to the
 * FIFO format before use.
 */
typedef struct          // A single raw frame read from registers
{
  uint8_t empty;        // Here to force uint16_t x to be on a word boundary
  uint8_t dummy;        // Dummy byte as read from the SPI bus
  int16_t x_dotdot;     // Sample frame directly from BMI270
  int16_t y_dotdot;
  int16_t z_dotdot;
  int16_t rho_dot;
  int16_t theta_dot;
  int16_t phi_dot;      // Z axis rotation speed
} register_raw_frame_t; // Value read from sensor

typedef struct
{
  real_t x_dotdot;      // X-axis acceleration in g
  real_t y_dotdot;      // Y-axis acceleration in g
  real_t z_dotdot;      // Z-axis acceleration in g
  real_t x_dot;         // Velocity in the X axis
  real_t y_dot;         // Velocity in the Y axis
  real_t z_dot;         // Velocity in the Z axis
  real_t x;             // X position
  real_t y;             // Y position
  real_t z;             // Z position
  real_t rho_dot;       // X anglular velocity
  real_t theta_dot;     // Y anglular velocity
  real_t phi_dot;       // Z anglular velocity
  real_t rho;           // X angle
  real_t theta;         // Y angle
  real_t phi;           // Z angle
} trace_vector_t;       // Vector at the point

typedef struct
{
  real_t x;             // X position in mm
  real_t y;             // Y position in mm
} trace_point_t;        // computed point

/*
 *  Global Variables
 */
EXTERN trace_vector_t trace_vector[2];                              // Space for the trace vector
EXTERN trace_point_t  trace_point[TRACE_RATE * SAMPLE_PERIOD];      // Space for the trace
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
void trace_init(void);                 // Get the target software ready
void trace_loop(void *arg);            // Target polling loop
void trace_push_button(void *arg);     // Monitor the push button
void trace_build(int timestamp);       // Build and send a trace
void trace_send(int oversample);       // Build and send a trace
void send_keep_alive(void);            // Send out the keep alive signal for TCPIP
bool prompt_for_confirm(char *prompt); // Prompt for a confirmation
#endif
