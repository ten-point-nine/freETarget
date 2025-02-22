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
 *  Port Definitions
 */
#define V_REFERENCE 0                                // Reference Input
#define V_12_LED    ADC(1, 0)                        // 12 Volt LED input
#define K_12        ((10000.0d + 2200.0d) / 2200.0d) // Resistor divider
#define BOARD_REV   ADC(1, 3)                        // Analog Version Input
#define LED_PWM     0                                // PWM mapped to PWM channel 0
#define MAX_ANALOG  0x3ff                            // Largest analog input
#define MAX_PWM     0xff                             // PWM is an 8 bit port

#define TO_VOLTS(x) (((double)(x) * 5.0) / 1024.0)

#define TEMP_IC (0x44)                               // TI HDC3022
#define DAC_IC  (0x60)                               // Microchip HCP4728

/*
 * Global functions
 */
void         adc_init(unsigned int channel,
                      unsigned int attenuation);       // Setup the analog hardware
unsigned int adc_read(unsigned int channel);           // Return the raw value
unsigned int revision(void);                           // Return the board revision
double       temperature_C(void);                      // Temperature in degrees C
double       humidity_RH(void);                        // Relative humidity in %
void         set_VREF(void);                           // Set the output of the VREF DAC(s)
void         set_LED_PWM(int percent);                 // Ramp the PWM duty cycle
void         set_LED_PWM_now(int percent);             // Set the PWM duty cycle
float        v12_supply(void);                         // Read the 12V supply
void         analog_input_test(void);                  // Read the analog input
double       speed_of_sound(double temperature,
                            double relative_humidity); // Calculate speed of sound

#endif
