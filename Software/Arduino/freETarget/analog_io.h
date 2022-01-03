#ifndef _ANALOG_IO_H_
#define _ANALOG_IO_H_

/*
 * Global functions
 */
void init_analog_io(void);              // Setup the analog hardware
unsigned int read_reference(void);      // Read the feedback channel
void show_analog(int v);                // Display the analog values
double temperature_C(void);             // Temperature in degrees C
unsigned int revision(void);            // Return the board revision
void set_LED_PWM(int percent);          // Ramp the PWM duty cycle
void set_LED_PWM_now(int percent);      // Set the PWM duty cycle
void set_V_SET_PWM(unsigned int value);  // Value to write to PWM
void compute_V_SET_PWM(int value);       // Reference voltage control loop

/*
 *  Port Definitions
 */

#define NORTH_ANA    1          // North Analog Input
#define EAST_ANA     2          // East Analog Input
#define SOUTH_ANA    3          // South Analog Input
#define WEST_ANA     4          // West Analog Input

#define SPARE_2A     7          // Not Used
#define V_REFERENCE  0          // Reference Input
#define V_TEST       8          // Read the test voltage
#define ANALOG_VERSION 5        // Analog Version Input
#define LED_PWM      5          // PWM Port
#define V_SET_PWM     8          // Reference Voltage Control
#define MAX_ANALOG  0x3ff       // Largest analog input
#define MAX_PWM      0xff       // PWM is an 8 bit port

#define TO_VOLTS(x) ( ((double)(x) * 5.0) / 1024.0 )

#define TEMP_IC   (0x9E >> 1)

#define LED_PWM_OFF     0x00
#define LED_PWM_TOGGLE  0xAB
#endif
