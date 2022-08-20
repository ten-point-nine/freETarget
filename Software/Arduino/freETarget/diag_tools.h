/*----------------------------------------------------------------
 *
 * diag_tools.h
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/
#ifndef _DIAG_TOOLS_H_
#define _DIAG_TOOLS_H_

#define T_HELP         0       // Help test
#define T_DIGITAL      1       // Digital test
#define T_TRIGGER      2       // Test microphone trigger
#define T_CLOCK        3       // Trigger clock internally
#define T_OSCOPE       4       // Microphone digital oscilliscope
#define T_OSCOPE_PC    5       // Display oscillisope on PC
#define T_PAPER        6       // Advance paper backer
#define T_SPIRAL       7       // Generate sprial pattern
#define T_GRID         8       // Generate grid pattern
#define T_ONCE         9       // Generate single calculation @ 45North
#define T_PASS_THRU   10       // Serial port pass through
#define T_SET_TRIP    11       // Set the microphone trip point
#define T_XFR_LOOP    12       // Transfer loopback
#define T_SERIAL_PORT 13       // Hello World n
#define T_LED         14       // Test the PWM
#define T_FACE        15       // Test the face detector
#define T_WIFI        16       // WiFi Test
#define T_NONVOL      17       // Dump Nonvol
#define T_SHOT        18       // Send shot record
#define T_WIFI_STATUS 19       // Obtain the WiFi Status
#define T_SPARE2      20
#define T_LOG         21       // Log the shot levels
#define T_SWITCH      25       // Test the switches

/*
 * LED status messages
 */
// Normal operation        RDY Light On
#define LED_RESET         L('.', '.', '.') // Force them all off
#define LED_READY         L('*', '.', '.') // The shot is ready to go
#define LED_OFF           L('.', '-', '-') // Turn off the READY light
#define LED_TABATA_ON     L('-', '*', '-') // Tabata is ready to go, leave the others alone
#define LED_TABATA_OFF    L('-', '.', '-') // Tabata is turned off, leave the others alone
#define LED_DONE          L('*', '*', '*') // A shot has been detected
#define LED_WIFI_SEND     L('.', '.', '*') // There is something going over the WiFi

// Sensor failed while waiting for a shot X Light On
#define NORTH_FAILED       L('.', '*', '.') // North sensor failed
#define EAST_FAILED        L('.', '*', '*') // East sensor failed
#define SOUTH_FAILED       L('*', '*', '*') // South sensor failed
#define WEST_FAILED        L('*', '*', '.') // West sensor failed

// Spare                   Y light On

#define UNUSED_2           L('*', '-', '*')

// Blinking fault message
#define POST_COUNT_FAILED  0b001            // LED fault code
#define VREF_OVER_UNDER    0b010            // Trip point over or under spec
#define UNUSED_2           0b100
#define SHOT_MISS          0b000            // Shot missed

void self_test(uint16_t test);
void show_sensor_status(unsigned int sensor_status);  // Display the sensor status as text
void blink_fault(unsigned int fault_code);            // Blink a fault
void POST_version(void);        // Show the version string
void POST_LEDs(void);           // Verify the LED operation
bool POST_counters(void);       // Verify the counter operation
void POST_trip_point(void);     // Display the set point
void set_trip_point(int v);     // Calibrate the trip point

#endif
