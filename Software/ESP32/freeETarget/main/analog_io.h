/*----------------------------------------------------------------
 *
 * analog_io.h
 *
 * Header file for Analog IO functions
 *
 *---------------------------------------------------------------*/
#ifndef _ANALOG_IO_H_
#define _ANALOG_IO_H_

/*
 * Global functions
 */
void adc_init(unsigned int channel, unsigned int gpio); // Setup the analog hardware
unsigned int adc_read(unsigned int channel);// Return the raw value
unsigned int read_12V(void);                // Read the 12V motor drive input
unsigned int revision(void);                // Return the board revision
double temperature_C(void);                 // Temperature in degrees C
double humidity_RH(void);                   // Relative humidity in %
void set_VREF(unsigned int channel, float volts); // Set the output of the VREF DAC(s)
void set_LED_PWM(int percent);              // Ramp the PWM duty cycle
void set_LED_PWM_now(unsigned int percent); // Set the PWM duty cycle
void set_vset_PWM(unsigned int value);      // Value to write to PWM

/*
 *  Port Definitions
 */
#define V_REFERENCE  0          // Reference Input
#define V_12_LED     ADC(1,0)   // 12 Volt LED input
#define K_12     ((10000.0d + 2200.0d)/ 2200.0d) // Resistor divider
#define BOARD_REV ADC(1,3) // Analog Version Input
#define LED_PWM      0          // PWM mapped to PWM channel 0
#define vset_PWM     8          // Reference Voltage Control
#define MAX_ANALOG  0x3ff       // Largest analog input
#define MAX_PWM      0xff       // PWM is an 8 bit port

#define TO_VOLTS(x) ( ((double)(x) * 5.0) / 1024.0 )

#define TEMP_IC      (0x44)     // TI HDC3022
#define DAC_IC       (0x60)     // Microchip HCP4728    

#endif