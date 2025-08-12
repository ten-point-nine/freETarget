/*----------------------------------------------------------------
 *
 * diag_tools.h
 *
 * Debug and test tools
 *
 *---------------------------------------------------------------*/
#ifndef _DIAG_TOOLS_H_
#define _DIAG_TOOLS_H_
#include "gpio.h"
/*
 * @function Prototypes
 */
void self_test(unsigned int test);
void show_sensor_status(unsigned int sensor_status);      // Display the sensor status as text
void show_sensor_fault(unsigned int sensor_status);       // Use the LEDs to show what sensor didn't work
void blink_fault(unsigned int fault_code);                // Blink a fault
void POST_version(void);                                  // Show the version string
bool POST_counters(void);                                 // Verify the counter operation
void POST_trip_point(void);                               // Display the set point
void set_trip_point(int x);                               // Calibrate the trip point
bool do_dlt(unsigned int level);                          // Diagnostics Log and Trace
bool factory_test(void);                                  // Test the hardware in production
void set_diag_LED(char *new_LEDs, unsigned int duration); // Display the LED failure code
bool check_12V(void);                                     // Check the 12 volt supply
void heartbeat(void);                                     // Send out regular status messages
void test_build_fake_shots(void);                         // Generate a list of shots

/*
 *  Definitions
 */

/*
 * LED status messages
 *
 */
//                       R              // RDY indicates operating status
//                        X             // X indicates communications status
//                         Y            // Y indicates feature status
//                          C           // Optional C output
//                           D          // Optional D ouput

#define LED_OFF                "     "  // Turn off all of the LEDs
#define LED_HELLO_WORLD        "RWB--"  // Hello World
#define LED_DLROW_OLLOH        "BWR--"  // Hello World Backwards
#define LED_GOOD               "G----"  // The software has started but not in shot mode
#define LED_PASS               "GGG---" //
#define LED_FATAL              "RRR--"  // A fatal error prevents operation
#define LED_READY              "g----"  // The shot is ready to go.  Blink to show we are alive
#define LED_BYE                "B----"  // Go to sleep
#define LED_OTA_WAITING        "b  --"  // Waiting for OTA to start
#define LED_OTA_DOWNLOAD       "B  --"  // The OTA is downloading
#define LED_OTA_DOWNLOAD_T     "BB --"  // The OTA is downloading blink
#define LED_OTA_FAILED_CONNECT "BR --"  // The OTA has failed to connect (Check the network)
#define LED_OTA_FAILED_LOAD    "B R--"  // The OTA has failed to load
#define LED_OTA_FATAL          "BRR--"  // The data was OK, but the OTA should not be used
#define LED_OTA_FINSHED        "BG --"  // The download has completed
#define LED_OTA_READY          "BGG--"  // The OTA is ready to go.  Reset the board
#define LED_READY_OFF          " ----"  // Turn off the READY light

#define LED_WIFI_OFF        "- ---"     // The WiFi is not operational
#define LED_WIFI_STATION    "-g---"     // The WiFi is in station mode but not connected
#define LED_WIFI_STATION_CN "-G---"     // The WiFI is in station mode and connected
#define LED_WIFI_ACCESS     "-b---"     // The WiFi is in access mode and not connected
#define LED_WIFI_ACCESS_CN  "-B---"     // The WiFI is in access mode and connected
#define LED_WIFI_FAULT      "-R---"     // The WiFI is in a fault state and cannot be used

#define LED_NO_12V           "--R--"    // The 12 Volt supply is not present
#define LED_LOW_12V          "--Y--"    // 12 Volt supply out of spec
#define LED_OK_12V           "--g--"    // The 12 Volt supply is in spec
#define LED_12V_NOT_USED     "--b--"    // The 12V is not used,
#define LED_C_OFF            "--- -"    // LED C is OFF
#define LED_C_BLINK          "---g-"    // LED C is blinking
#define LED_C_ON             "---G-"    // LED C is ON
#define LED_D_OFF            "-----"    // LED D is OFF
#define LED_D_BLINK          "----r"    // LED D is blinking
#define LED_D_ON             "----R"    // LED D is ON
#define LED_RAPID_OFF        "---  "    // Rapid fire LEDs are OFF
#define LED_RAPID_RED        "--- R"    // Rapid fire RED is ON
#define LED_RAPID_RED_WARN   "--- r"    // Rapid fire RED is BLINKING
#define LED_RAPID_RED_OFF    "---- "    // Rapid fire RED is OFF
#define LED_RAPID_GREEN      "---G "    // Rapid fire GREEN is ON
#define LED_RAPID_GREEN_OFF  "--- -"    // Rapid fire GREEN is OFF
#define LED_RAPID_GREEN_WARN "---gr"    // Rapid fire GREEN is BLINKING
#define LED_TABATA_OFF       "-----"    // TABATA LEDs are OFF
#define LED_TABATA_WARN      "---g-"    // TABATA LEDS are blinking
#define LED_TABATA_ON        "---G-"    // TABATA LEDs are ON

// Fatal Error.  Halts operation

#define LED_FAIL_CLOCK_STOP  "RBR--" // The reference clock cannot be stopped
#define LED_FAIL_CLOCK_START "RBG--" // The reference clock cannot be started
#define LED_FAIL_RUN_STUCK   "RBB--" // There is a stuck bit in the RUN latch
#define LED_FAIL_RUN_OPEN    "RBW--" // The sensor line is open circuit

// Fault Codes - RDY LED set to RED to indiate a fault
#define LED_NORTH_FAILED "R-R--" // North sensor failed
#define LED_EAST_FAILED  "R-G--" // East sensor failed
#define LED_SOUTH_FAILED "R-B--" // South sensor failed
#define LED_WEST_FAILED  "R-Y--" // West sensor failed
#define LED_MISS         "R-r--" // Shot was detected as a miss

/*
 *  On board expected values
 */
#define V12_WORKING 10.0 // Expect that the 12 Volt supply is over 10 volts
#define V12_CAUTION 5.0  // Caution if the 12 Volts supply is less than 5 volts

/*
 * Tracing
 */
#define DLT_NONE          0      // No DLT messages displayed
#define DLT_CRITICAL      0x0001 // Action failed and needs to be reported
#define DLT_INFO          0x0002 // Information which is always displayed
#define DLT_APPLICATION   0x0004 // Application level messages displayed (freeETarget.c compute_hit.c)
#define DLT_COMMUNICATION 0x0008 // Communications messages (wifi.c token.c serial_io.c)
#define DLT_DIAG          0x0010 // Hardware diagnostics messages displayed
#define DLT_DEBUG         0x0020 // Specific debug information
#define DLT_SCORE         0x0040 // Display extended score record
#define DLT_HTTP          0x0080 // Log HTTP requests
#define DLT_OTA           0x0100 // Log OTA requests
#define DLT_HEARTBEAT     0x0200 // Kick out the time to see if we are alive

                                 /*
                                  *  Enable compile level tracing
                                  */
// #define TRACE_APPLICATION
// #define TRACE_COMMUNICATION
// #define TRACE_DIAG
// #define TRACE_DEBUG
// #define TRACE_SCORE
// #define TRACE_HTTP
// #define TRACE_OTA
// #define TRACE_HEARTBEAT

// clang-format off
#define DLT(level, z) if ( do_dlt(level) )  { z }
// clang-format on

typedef struct
{
  unsigned int dlt_mask; // ex DLT_CRITICAL
  char        *dlt_text; // ex "DLT_CRITICAL"
  char         dlt_id;   // ex C
} dlt_name_t;            // Names and masks for DLT levels

extern const dlt_name_t dlt_names[];

/*
 *  Variables
 */
#endif
