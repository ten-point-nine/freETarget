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
 *
 */
//                         R                // RDY indicates operating status 
//                          X               // X indicates communications status
//                           Y              // Y indicates feature status
#define LED_OFF           "   "             // Turn off all of the LEDs
#define LED_ALL_PUSH      "PPP"             // Push all of the LEDs
#define LED_ALL_POP       "ppp"             // Pop all of the LEDs
#define LED_HELLO_WORLD   "RWB"             // Hello World
#define LED_RESET         "   "             // Force them all off
#define LED_READY_PUSH    "P--"             // Push the READY LED onto the stack
#define LED_READY_POP     "p--"             // Take the READY LED from the stack 
#define LED_GOOD          "G  "             // The software has started but not in shot mode
#define LED_READY         "g  "             // The shot is ready to go.  Blik to show we are alive
#define LED_READY_OFF     " --"             // Turn off the READY light

#define LED_WIFI_OFF      "- -"             // The WiFi is not operational
#define LED_STATION       "-g-"             // The WiFi is in station mode but not connected
#define LED_STATION_CN    "-G-"             // The WiFI is in station mode and connected 
#define LED_ACCESS        "-b-"             // The WiFi is in access mode and not connected
#define LED_ACCESS_CN     "-B-"             // The WiFI is in access mode and connected 
#define LED_RX            "-R-"             // Receiving over WiFi/Serial
#define LED_TX            "-W-"             // Transmitting over WiFi / Serial
#define LED_SIO_PUSH      "-P-"             // Push the LEDs
#define LED_SIO_POP       "-p-"             // Pop the LEDs

#define LED_MFS_PUSH      "-PP"             // Push the MFS LEDs
#define LED_MFS_POP       "-pp"             // Pop the MFS LEDs

#define LED_MFS_A         "-G-"             // Copy MFS to the LEDs
#define LED_MFS_B         "--G"

#define LED_TABATA_ON     "--G"             // Tabata is ready to go, leave the others alone
#define LED_TABATA_OFF    "-- "             // Tabata is turned off, leave the others alone
#define LED_RAPID_ON      "--G"             // Rapidfire course of fire on
#define LED_RAPID_OFF     "-- "             // Rapidfire course of fire off
#define LED_WIFI_DONE     "-- "             // Finished sending

// Fault Codes - RDY LED set to RED to indiate a fault
#define LED_NORTH_FAILED   "RRR"            // North sensor failed
#define LED_EAST_FAILED    "RRG"            // East sensor failed
#define LED_SOUTH_FAILED   "RRB"            // South sensor failed
#define LED_WEST_FAILED    "RRY"            // 3 West sensor failed

#define LED_MISS           "RGR"            // Shot was detected as a miss
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
#define DZZ(level, z) if ( do_dlt(level)){z}
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
