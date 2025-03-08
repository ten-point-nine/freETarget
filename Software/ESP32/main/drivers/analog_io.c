/******************************************************************************
 *
 * analog_io.c
 *
 * General purpose Analog driver
 *
 *****************************************************************************/

#include "stdbool.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "gpio.h"
#include "analog_io.h"
#include "gpio_types.h"
#include "adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "pwm.h"
#include "gpio_define.h"
#include "i2c.h"
#include "dac.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"

/*
 * Function prototypes
 */
void set_vset_PWM(unsigned int pwm);

/*----------------------------------------------------------------
 *
 * @function: adc_init()
 *
 * @brief:  Initialize the ADC channel
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The ADC channel is initialized and the handle set up
 *
 * https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/adc.html
 *
 *--------------------------------------------------------------*/

void adc_init(unsigned int adc_channel,    // What ADC channel are we accessing
              unsigned int adc_attenuation // What is the channel attenuation
)
{
  unsigned int adc;                        // Which ADC (1/2)
  unsigned int channel;                    // Which channel attached to the ADC (0-10)

  adc     = ADC_ADC(adc_channel);          // What ADC are we on
  channel = ADC_CHANNEL(adc_channel);

  /*
   * Setup the channel
   */
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
  switch ( adc )
  {
    case 1:
      ESP_ERROR_CHECK(adc1_config_channel_atten(channel, adc_attenuation));
      break;

    case 2:
      ESP_ERROR_CHECK(adc2_config_channel_atten(channel, adc_attenuation));
      break;
  }

  /*
   *  Ready to go
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: adc_read()
 *
 * @brief:  Read a value from teh ADC channel
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The ADC channel is initialized and the handle set up
 *
 * https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/adc.html
 *
 *--------------------------------------------------------------*/
unsigned int adc_read(unsigned int adc_channel // What input are we reading?
)
{
  unsigned int adc;                            // Which ADC (1/2)
  unsigned int channel;                        // Which channel attached to the ADC (0-10)
  int          raw;                            // Raw value from the ADC

  adc     = ADC_ADC(adc_channel);              // What ADC are we on
  channel = ADC_CHANNEL(adc_channel);          // What channel are we using

  /*
   *  Read the appropriate channel
   */
  switch ( adc )
  {
    case 1:
      raw = adc1_get_raw(channel);
      break;

    case 2:
      adc2_get_raw(channel, ADC_WIDTH_BIT_DEFAULT, &raw);
      break;
  }

  /*
   *  Done
   */
  return raw;
}

#define V12_RESISITOR   ((40.2 + 5.0) / 5.0) // Resistor divider
#define V12_ATTENUATION 3.548                // 11 DB
#define V12_REF         1.1                  // ESP32 VREF
#define V12_CAL         0.88
float v12_supply(void)
{
  float raw;                                 // Raw voltage from ADC

  raw = (float)adc_read(V_12_LED);

  return V12_REF * V12_ATTENUATION * (raw / 4095.0) * V12_RESISITOR * V12_CAL;
}

/*----------------------------------------------------------------
 *
 * @function: set_LED_PWM()
 * @function: set_LED_PWM_now()
 *
 * @brief: Program the PWM value
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * json_LED_PWM is a number 0-100 %  It must be scaled 0-255
 *
 * The function ramps the level between the current and desired
 *
 *--------------------------------------------------------------*/
static unsigned int old_LED_percent = 0;

void set_LED_PWM_now(int new_LED_percent // Desired LED level (0-100%)
)
{
  if ( new_LED_percent == old_LED_percent )
  {
    return;
  }

  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "new_LED_percent: %d  old_LED_percent: %d", new_LED_percent, old_LED_percent);))

  pwm_set(LED_PWM, new_LED_percent); // Write the value out

  old_LED_percent = new_LED_percent;

  return;
}

void set_LED_PWM         // Theatre lighting
    (int new_LED_percent // Desired LED level (0-100%)
    )
{
  if ( new_LED_percent == old_LED_percent )
  {
    return;
  }

  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "new_LED_percent: %d  old_LED_percent: %d", new_LED_percent, old_LED_percent);))

  /*
   * Loop and ramp the LED  PWM up or down slowly
   */
  while ( new_LED_percent != old_LED_percent )   // Change in the brightness level?
  {

    if ( new_LED_percent < old_LED_percent )
    {
      old_LED_percent--;                         // Ramp the value down
    }
    else
    {
      old_LED_percent++;                         // Ramp the value up
    }
    pwm_set(LED_PWM, old_LED_percent);           // Write the value out
    timer_delay((unsigned long)ONE_SECOND / 50); // Worst case, take 2 seconds to get there
  }

  /*
   * All done, begin the program
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: revision(void)
 *
 * @brief: Return the board revision
 *
 * @return: Board revision level
 *
 *--------------------------------------------------------------
 *
 *  Read the analog value from the resistor divider, keep only
 *  the top 4 bits, and return the version number.
 *
 *  The analog input is a number 0-1024 which is banded and
 *  used to look up a table of revision numbers.
 *
 *  To accomodate unknown hardware builds, if the revision is
 *  undefined (< 100) then the last 'good' revision is returned
 *
 *--------------------------------------------------------------*/
//                                       0      1  2  3  4  5  6  7  8  9   A   B   C   D   E   F
const static unsigned int version[] = {REV_510, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, REV_520};

unsigned int revision(void)
{
  int revision;

  /*
   *  Read the resistors and determine the board revision
   */
  revision = version[adc_read(BOARD_REV) >> (12 - 4)];

  /*
   * Nothing more to do, return the board revision
   */
  return revision;
}

/*----------------------------------------------------------------
 *
 * @function: temperature_C()
 *
 * @brief: Read the temperature sensor and return temperature in degrees C
 *
 *----------------------------------------------------------------
 *
 * See TI Documentation for HDC3022
 * https://www.ti.com/product/HDC3022, Page 13
 *
 * A simple interrogation is used.
 *
 *--------------------------------------------------------------*/
static float t_c; // Temperature from sensor
static float rh;  // Humidity from sensor

double temperature_C(void)
{
  unsigned char temp_buffer[6];
  int           raw;

  /*
   * Read in the temperature and humidity together
   */
  temp_buffer[0] = 0x24; // Trigger read on demand
  temp_buffer[1] = 0x00;
  i2c_write(TEMP_IC, temp_buffer, 2);
  i2c_read(TEMP_IC, temp_buffer, 6);

  /*
   *  Return the temperature in C
   */
  raw = (temp_buffer[0] << 8) + temp_buffer[1];
  t_c = -45.0 + (175.0 * (float)raw / 65535.0);
  raw = (temp_buffer[3] << 8) + temp_buffer[4];
  rh  = 100.0 * (float)raw / 65535.0;

  return t_c;
}

/*----------------------------------------------------------------
 *
 * @function: humidity_RH()
 *
 * @brief: Return the previoudly read humidity
 *
 * @return: Humidity in RH (0-100%)
 *----------------------------------------------------------------
 *
 * See TI Documentation for HDC3022
 * https://www.ti.com/product/HDC3022
 *
 * A simple interrogation is used.
 *
 *--------------------------------------------------------------*/
double humidity_RH(void)
{
  temperature_C(); // Read in the temperature and humidity
  return rh;
}

/*----------------------------------------------------------------
 *
 * @function: set_VREF()
 *
 * @brief: Set the refererence voltage for the comparitor
 *
 *----------------------------------------------------------------
 *
 * See Microchip documentation for MCP4728
 * https://ww1.microchip.com/downloads/en/DeviceDoc/22187E.pdf
 * Figure 5-8
 *
 *--------------------------------------------------------------*/
void set_VREF(void)
{
  float volts[4];

  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "Set VREF: %4.2f %4.2f", json_vref_lo, json_vref_hi);))

  if ( (json_vref_lo == 0) // Check for an uninitialized VREF
       || (json_vref_hi == 0) )
  {
    json_vref_lo = 1.25;   // and force to something other than 0
    json_vref_hi = 2.00;   // Otherwise the sensors continioustly interrupt
  }

  if ( json_vref_lo >= json_vref_hi )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "ERROR: json_vref_lo or json_vref_hi are out of order.");))
  }

  volts[VREF_LO] = json_vref_lo;
  volts[VREF_HI] = json_vref_hi;
  volts[VREF_2]  = 0.0;
  volts[VREF_3]  = 0.0;
  DAC_write(volts);

  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: analog_input_test()
 *
 * @brief:    Read the analog input and display the results
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void analog_input_test(void)
{
  SEND(ALL, sprintf(_xs, "\r\n12V %5.3f", v12_supply());)
  SEND(ALL, sprintf(_xs, "\r\nBoard Rev %d", revision());)
  SEND(ALL, sprintf(_xs, "\r\nTemperature: %4.2f", temperature_C());)
  SEND(ALL, sprintf(_xs, "\r\nHumidity: %4.2f", humidity_RH());)
  SEND(ALL, sprintf(_xs, "\r\nSpeed of Sound: %4.2fmm/us", speed_of_sound(temperature_C(), humidity_RH()));)
  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}
