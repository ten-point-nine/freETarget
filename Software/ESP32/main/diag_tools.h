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
void zapple(unsigned int test);                           // ZAPPLE console monitor
bool factory_test(void);                                  // Test the hardware in production
void set_diag_LED(char* new_LEDs, unsigned int duration); // Display the LED failure code
bool check_12V(void);                                     // Check the 12 volt supply

/* 
 *  Definitions
 */
#define T_HELP           0        // Help test
#define T_FACTORY        1        // Factory Test

#define T_DIGITAL       10        // Digital test
#define T_PAPER         11        // Advance paper backer
#define T_LED           12        // Test the LED PWM
#define T_STATUS        13        // Send colours across the status LEDs
#define T_TEMPERATURE   14        // Read the temperature and humidity
#define T_DAC           15        // Ramp the DAC outputs 
#define T_RAPID_LEDS    16        // Test the Rapid Fire LEDs

#define T_PCNT          20        // PCNT register test
#define T_PCNT_STOP     21        // PCNT timers stopped
#define T_PCNT_SHORT    22        // PCNT timers start-stop  
#define T_PCNT_FREE     23        // PCNT timers free running
#define T_PCNT_CLEAR    24        // PCNT timers cleared after running
#define T_CYCLE_CLOCK   25        // Turn the clock on and off
#define T_RUN_ALL       26        // Toggle the RUN lines on and off 
#define T_PCNT_CAL      27        // Trigger PCNT via a triangle wave and watch the outcome

#define T_AUX_SERIAL    30        // AUX Serial Port loopback
#define T_WIFI_AP       31        // Test WiFi as an Access Point
#define T_WIFI_STATION  32        // Test WiFI as a station 
#define T_WIFI_SERVER   33        // Enable the WiFI Server
#define T_WIFI_STATION_LOOPBACK 34 // Send an receive over the WiFi conduit
#define T_WIFI_AP_LOOPBACK 35     // Send an receive over the WiFi conduit

#define T_SENSOR        40        // Sensor POST test
#define T_TARGET        41        // Polled target sensor test
#define T_TARGET_2      42        // Interrupt target test

/*
 * LED status messages
 *
 */
//                           R              // RDY indicates operating status 
//                            X             // X indicates communications status
//                             Y            // Y indicates feature status
//                              C           // Optional C output
//                               D          // Optional D ouput

#define LED_OFF             "     "         // Turn off all of the LEDs
#define LED_HELLO_WORLD     "RWB--"         // Hello World
#define LED_GOOD            "G----"         // The software has started but not in shot mode
#define LED_READY           "g----"         // The shot is ready to go.  Blink to show we are alive
#define LED_BYE             "B----"         // Go to sleep
#define LED_READY_OFF       " ----"         // Turn off the READY light

#define LED_WIFI_OFF        "- ---"         // The WiFi is not operational
#define LED_WIFI_STATION    "-g---"         // The WiFi is in station mode but not connected
#define LED_WIFI_STATION_CN "-G---"         // The WiFI is in station mode and connected 
#define LED_WIFI_ACCESS     "-b---"         // The WiFi is in access mode and not connected
#define LED_WIFI_ACCESS_CN  "-B---"         // The WiFI is in access mode and connected 

#define LED_NO_12V          "--R--"         // The 12 Volt supply is not present
#define LED_LOW_12V         "--Y--"         // 12 Volt supply out of spec
#define LED_OK_12V          "--g--"         // The 12 Volt supply is in spec
#define LED_12V_NOT_USED    "--b--"         // The 12V is not used, 
#define LED_C_OFF           "--- -"         // LED C is OFF
#define LED_C_BLINK         "---g-"         // LED C is blinking
#define LED_C_ON            "---G-"         // LED C is ON
#define LED_D_OFF           "-----"         // LED D is OFF
#define LED_D_BLINK         "----r"         // LED D is blinking
#define LED_D_ON            "----R"         // LED D is ON
#define LED_RAPID_OFF       "---  "         // Rapid fire LEDs are OFF
#define LED_RAPID_RED       "----R"         // Rapid fire RED is ON
#define LED_RAPID_RED_WARN  "----r"         // Rapid fire RED is BLINKING
#define LED_RAPID_RED_OFF   "---- "         // Rapid fire RED is OFF
#define LED_RAPID_GREEN     "---G-"         // Rapid fire GREEN is ON
#define LED_RAPID_GREEN_OFF "--- -"         // Rapid fire GREEN is OFF
#define LED_RAPID_GREEN_WARN "---g-"        // Rapid fire GREEN is BLINKING
#define LED_TABATA_OFF      "----R"         // TABATA LEDs are OFF
#define LED_TABATA_WARN     "---g-"         // TABATA LEDS are blinking
#define LED_TABATA_ON       "---G-"         // TABATA LEDs are ON


// Fatal Error.  Halts operation

#define LED_FAIL_CLOCK_STOP  "RBR--"        // The reference clock cannot be stopped 
#define LED_FAIL_CLOCK_START "RBG--"        // The reference clock cannot be started
#define LED_FAIL_RUN_STUCK   "RBB--"        // There is a stuck bit in the RUN latch
#define LED_FAIL_RUN_OPEN    "RBW--"        // The sensor line is open circuit 

// Fault Codes - RDY LED set to RED to indiate a fault
#define LED_NORTH_FAILED   "R-R--"          // North sensor failed
#define LED_EAST_FAILED    "R-G--"          // East sensor failed
#define LED_SOUTH_FAILED   "R-B--"          // South sensor failed
#define LED_WEST_FAILED    "R-Y--"          // West sensor failed
#define LED_MISS           "R-r--"          // Shot was detected as a miss

/*
 *  On board expected values
 */
#define V12_WORKING  10.0                   // Expect that the 12 Volt supply is over 10 volts
#define V12_CAUTION   5.0                   // Caution if the 12 Volts supply is less than 5 volts

/*
 * Tracing 
 */
#define DLT_NONE          0                       // No DLT messages displayed
#define DLT_CRITICAL      0x80                    // Display messages that will compromise the target
#define DLT_APPLICATION   0x40                    // Application level messages displayed
#define DLT_DIAG          0x20                    // Hardware diagnostics messages displayed
#define DLT_INFO          0x08                    // Informational messages
#define DLT_DEBUG         0x01                    // Specific debug information
#define DLT(level, z)  if ( do_dlt(level)){z}                              

/*
 *  Variables
 */
#endif
