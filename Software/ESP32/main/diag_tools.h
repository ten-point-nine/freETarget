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
void    self_test(unsigned int test);
void    show_sensor_status(unsigned int sensor_status); // Display the sensor status as text
void    blink_fault(unsigned int fault_code);           // Blink a fault
void    POST_version(void);                             // Show the version string
bool    POST_counters(void);                            // Verify the counter operation
void    POST_trip_point(void);                          // Display the set point
void    set_trip_point(int x);                          // Calibrate the trip point
bool    do_dlt(unsigned int level);                     // Diagnostics Log and Trace
void    zapple(unsigned int test);                      // ZAPPLE console monitor

#define T_HELP           0        // Help test
#define T_DIGITAL        1        // Digital test
#define T_PAPER          2        // Advance paper backer
#define T_LED            3        // Test the LED PWM
#define T_STATUS         4        // Send colours across the status LEDs
#define T_TEMPERATURE    5        // Read the temperature and humidity
#define T_DAC            6        // Ramp the DAC outputs 
#define T_PCNT           7        // PCNT register test
#define T_SENSOR         8        // Sensor POST test
#define T_AUX_SERIAL     9        // AUX Serial Port loopback
#define T_TARGET        10        // Polled target sensor test
#define T_TARGET_2      11        // Interrupt target test
#define T_WIFI_AP       12        // Test WiFi as an Access Point
#define T_WIFI_STATION  13        // Test WiFI as a station 
#define T_WIFI_SERVER   14        // Enable the WiFI Server
#define T_WIFI_STATION_LOOPBACK 15 // Send an receive over the WiFi conduit
#define T_WIFI_AP_LOOPBACK 16     // Send an receive over the WiFi conduit
#define T_CYCLE_CLOCK   17        // Turn the clock on and off
#define T_RUN_ALL       18        // Toggle the RUN lines on and off 
#define T_PCNT_STOP     19        // PCNT timers stopped
#define T_PCNT_SHORT    20        // PCNT timers start-stop  
#define T_PCNT_FREE     21        // PCNT timers free running
#define T_PCNT_CLEAR    22        // PCNT timers cleared after running

/*
 * LED status messages
 */

#define LED_RESET         "   "             // Force them all off
#define LED_READY         "g  "             // The shot is ready to go.  Blik to show we are alive
#define LED_READY_OFF     " --"             // Turn off the READY light
#define LED_TABATA_ON     "--G"             // Tabata is ready to go, leave the others alone
#define LED_TABATA_OFF    "-- "             // Tabata is turned off, leave the others alone
#define LED_RAPID_ON      "--G"             // Rapidfire course of fire on
#define LED_RAPID_OFF     "-- "             // Rapidfire course of fire off
#define LED_HIT           "-G-"             // A shot has been detected
#define LED_MISS          "-R-"             // Last shot was a miss
#define LED_WIFI_SEND     "--B"             // There is something going over the WiFi
#define LED_WIFI_DONE     "-- "             // Finished sending
#define LED_HELLO_WORLD   "RWB"             // Hello World

// Fault Codes
#define LED_FAULT          "--R"            // Generic fault
#define LED_NORTH_FAILED   "RRR"            // North sensor failed
#define LED_EAST_FAILED    "GRR"            // East sensor failed
#define LED_SOUTH_FAILED   "BRR"            // South sensor failed
#define LED_WEST_FAILED    "WRR"            // 3 West sensor failed

#define LED_FAIL_A         "RGR"            // 
#define LED_FAIL_B         "RGG"            // 
#define LED_FAIL_C         "RGB"            // 
#define LED_FAIL_D         "RGW"            // 

#define LED_FAIL_E         "RBR"            // 
#define LED_FAIL_F         "RBG"            // 
#define LED_FAIL_G         "RBB"            // 
#define LED_FAIL_H         "RBW"            // 

#define LED_FAIL_I         "RWR"            // 
#define LED_FAIL_J         "RWG"            // 
#define LED_FAIL_K         "RWB"            // 
#define LED_FAIL_L         "RWW"            // 

/*
 * Tracing 
 */
#define DZZ(level, z) if ( do_dlt(level)){ z}
#define DLT(level)      ( do_dlt(level) )
#define DLT_NONE          0                       // No DLT messages displayed
#define DLT_CRITICAL      0x80                    // Display messages that will compromise the target
#define DLT_APPLICATION   0x01                    // Application level messages displayed
#define DLT_DIAG          0x02                    // Diagnostics messages displayed
#define DLT_INFO          0x04                    // Informational messages

/*
 *  Variables
 */
extern const char* which_one[8];
#endif
