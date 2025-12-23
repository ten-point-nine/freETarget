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
#include "board_assembly.h"
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
void          set_vset_PWM(unsigned int pwm);
static double temperature_C_HDC3022(void);  // Temperature in degrees C
static double temperature_C_TMP1075D(void); // Temperature in degrees C

/*
 *  Variables
 */
int          board_version = -1; // Board Revision number
unsigned int board_mask    = 0;  // Mask for the board revision
static float rh;                 // Humidity from sensor

/*
 * Constants
 */
#define ADC_BIAS     0.1                    // ADC reads 0.1 volts low
#define ADC_REF      3.3                    // ADC reference voltage
#define ADC_FULL     4095.0                 // 12 bit ADC full scale
#define VREF_DIVIDER ((4700 + 4700) / 4700) // Voltage divider ratio

#define V12_RESISTOR ((40.2 + 4.7) / 4.7)   // Resistor divider

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
 * There is noise on the power supply lines, and since all of the
 * analog inputs are driven off of the 5 volt supply, there is
 * a lot of noise in the sample.
 *
 * To get around this problem, the input is sampled FILTER times
 * and averaged.
 *
 *--------------------------------------------------------------*/
#define FILTER 16                              // How many averages

unsigned int adc_read(unsigned int adc_channel // What input are we reading?
)
{
  unsigned int adc;                            // Which ADC (1/2)
  unsigned int channel;                        // Which channel attached to the ADC (0-10)
  int          raw, sum;                       // Raw value from the ADC
  int          i;

  adc     = ADC_ADC(adc_channel);              // What ADC are we on
  channel = ADC_CHANNEL(adc_channel);          // What channel are we using

  /*
   *  Read the appropriate channel
   */
  sum = 0;
  for ( i = 0; i != FILTER; i++ ) // Add up FILTER samples
  {
    switch ( adc )
    {
      case 1:
        raw = adc1_get_raw(channel);
        break;

      case 2:
        adc2_get_raw(channel, ADC_WIDTH_BIT_DEFAULT, &raw);
        break;
    }
    sum += raw;
  }

  /*
   *  Done
   */
  sum /= FILTER;

  return (sum & 0x0fff);
}

#define V12_RESISITOR   ((40.2 + 5.0) / 5.0) // Resistor divider
#define V12_ATTENUATION 3.548                // 11 DB
#define V12_REF         1.1                  // ESP32 VREF
#define V12_CAL         0.88

float v12_supply(void)
{
  return (float)adc_read(V_12_LED) / ADC_FULL * ADC_REF * V12_RESISTOR + ADC_BIAS;
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
  while ( new_LED_percent != old_LED_percent )  // Change in the brightness level?
  {

    if ( new_LED_percent < old_LED_percent )
    {
      old_LED_percent--;                        // Ramp the value down
    }
    else
    {
      old_LED_percent++;                        // Ramp the value up
    }
    pwm_set(LED_PWM, old_LED_percent);          // Write the value out
    vTaskDelay((unsigned long)ONE_SECOND / 50); // Worst case, take 2 seconds to get there
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
//                                        0     1  2     3     4  5     6     7  8  9   A   B   C   D   E   F
const static unsigned int version[] = {REV_510, 1, 2, REV_610, 4, 5, REV_600, 7, 8, 9, 10, 11, 12, 13, 14, REV_520};

unsigned int revision(void)
{
  int index;                // Index into the version table

  if ( board_version >= 0 ) // Already read the revision?
  {
    return board_version;   // Return the cached value
  }

  /*
   *  Read the resistors and determine the board revision
   */
  index = (adc_read(BOARD_REV) >> (12 - 4)) & 0x0f; // Top 4 bits only

  board_version = version[index];                   // Get the board revision number

  board_mask = 1 << index;                          // Set the mask for the board revision

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Board Revision: %d  Board Mask: %04X", board_version, board_mask);))

  return board_version;
}

/*----------------------------------------------------------------
 *
 * @function: vref_measure()
 *
 * @brief:  Measure the reference voltage
 *
 * @return: VREF in volts
 *
 *--------------------------------------------------------------
 *
 *  Read the VREF voltage.  Only applies on board revisions
 *  530 and later.  The VREF is read from the ADC and converted
 *  to a voltage
 *
 *--------------------------------------------------------------*/
double vref_measure(void)
{
  if ( TMP1075D )
  {
    return ((double)adc_read(VMES_LO)) / ADC_FULL * ADC_REF * VREF_DIVIDER + ADC_BIAS; // 4096 full scale, 3.3 VREF 1/2 voltage divider
  }
  else
  {
    return -1.0;                                                                       // Not available
  }
}

/*----------------------------------------------------------------
 *
 * @function: temperature_C()
 *
 * @brief: Read the temperature sensor and return temperature in degrees C
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/
double temperature_C(void)
{
  if ( HDC3022 )
  {
    return temperature_C_HDC3022();  // TI HDC3022
  }
  else
  {
    return temperature_C_TMP1075D(); // TI TMP1075D
  }
}

/*----------------------------------------------------------------
 *
 * @function: temperature_C_HDC3022()
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
static double temperature_C_HDC3022(void)
{
  unsigned char temp_buffer[6];
  int           raw;
  static float  t_c; // Remember the temperature

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
 * @function: temperature_C_TMP1075D()
 *
 * @brief: Special driver for TMP1075D temperature sensor
 *
 *----------------------------------------------------------------
 *
 * See TI Documentation for TMP1075D
 * https://www.ti.com/lit/ds/symlink/tmp1075.pdf?ts=1745106831531&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTMP1075
 *
 * A simple interrogation is used.
 *
 * The function has two modes.
 *
 * Revision 6.0, read the temperature only once at power up, and
 * return the same value thereafter.
 *
 * For all other revisions, read the temperature each time.
 *--------------------------------------------------------------*/
#define TC_CAL (0.0625 / 16) // 'C / LSB
static double temperature_C_TMP1075D(void)
{
  unsigned char temp_buffer[6];
  int           raw;
  static float  t_c = -274;  // Remember the temperature Set to below absolute zero

                             /*
                              * Check for a Rev 6.0 board
                              */
  if ( board_version == REV_600 ) // Revision 6.0 board?
  {
    if ( v12_supply() >= 5.0 )     // 12V supply present.  Possible self heating.
    {
      if ( t_c > -273 )            // If we have a valid temperature, return it
      {
        return t_c;
      }
    }
  }

  /*
   * Read in the temperature
   */
  temp_buffer[0] = 0x00;                       // Trigger read on demand
  i2c_write(TEMP_IC_TMP1075D, temp_buffer, 1); // Send pointer to register
  i2c_read(TEMP_IC_TMP1075D, temp_buffer, 2);  // Read 16 bit temperature

  /*
   *  Return the temperature in C
   */
  raw = (temp_buffer[0] << 8) + temp_buffer[1];
  t_c = ((float)raw * TC_CAL);
  rh  = 40.0; // Force humidity to 40% RH

  /*
   * Compensate for self heating not used in Indian boards
   */
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
  double volts[4];

  if ( (json_vref_lo == 0) // Check for an uninitialized VREF
       || (json_vref_hi == 0) )
  {
    json_vref_lo = 1.25;   // and force to something other than 0
  }

  if ( MCP4728 )
  {
    if ( json_vref_hi == 0 )
    {
      json_vref_hi = json_vref_lo + 0.75; // Otherwise the sensors continioustly interrupt
    }
  }

  if ( MCP4728 )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Set VREF_LO: %4.2f   VREF_HI: %4.2f", json_vref_lo, json_vref_hi);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Set VREF_LO: %4.2f", json_vref_lo);))
  }

  if ( MCP4728 ) // Check for four channel DAC
  {
    if ( json_vref_lo >= json_vref_hi )
    {
      DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "ERROR: json_vref_lo or json_vref_hi are out of order.");))
    }
  }

  volts[VREF_LO] = json_vref_lo;
  volts[VREF_HI] = json_vref_hi;
  volts[VREF_2]  = 0.0;
  volts[VREF_3]  = 0.0;
  DAC_write(volts);

  /*{
   *  All done, return
   */
  if ( MCP4725 )
  {
    DAC_calibrate(); // Adjust the DAC output
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Read VREF_LO: %4.2f", vref_measure());))
  }

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
  SEND(ALL, sprintf(_xs, "\r\n12V: %5.3f", v12_supply());)
  if ( VREF_FB )
  {
    SEND(ALL, sprintf(_xs, "\r\nVREF_MEASURE: %5.3f", vref_measure());)
  }
  SEND(ALL, sprintf(_xs, "\r\nBoard Rev: %4.2f", (float)revision() / 100.0);)
  SEND(ALL, sprintf(_xs, "\r\nTemperature: %4.2f", temperature_C());)
  if ( HDC3022 )
  {
    SEND(ALL, sprintf(_xs, "\r\nHumidity: %4.2f", humidity_RH());)
  }
  else
  {
    SEND(ALL, sprintf(_xs, "\r\nHumidity: N/A");)
  }
  SEND(ALL, sprintf(_xs, "\r\nSpeed of Sound: %4.2fmm/us", speed_of_sound(temperature_C(), humidity_RH()));)
  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: analog_input_raw()
 *
 * @brief:    Virtual adc oscilliscope
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Read the raw analog inputs and display them as hex
 *
 *--------------------------------------------------------------*/
typedef struct analog_raw
{
  char        *name;     // Text of input
  int          channel;  // Channel ID
  unsigned int min, max; // Previous inputs
} analog_raw_t;

static analog_raw_t analog_sample[] = {
    {"12V",     V_12_LED,  0xffff, 0},
    {"BD Rev",  BOARD_REV, 0xffff, 0},
    {"VREF_LO", VMES_LO,   0xffff, 0},
    {"",        0,         0,      0}
};

void analog_input_raw(void)
{
  unsigned int i, raw;

  while ( serial_available(ALL) == 0 )
  {

    SEND(ALL, sprintf(_xs, "\r\n");)
    i = 0;
    while ( analog_sample[i].name[0] != 0 )
    {
      raw = adc_read(analog_sample[i].channel);
      if ( analog_sample[i].min > raw )
      {
        analog_sample[i].min = raw;
      }
      if ( analog_sample[i].max < raw )
      {
        analog_sample[i].max = raw;
      }
      SEND(ALL, sprintf(_xs, "%s: 0X%04X 0X%04X 0X%04X     ", analog_sample[i].name, analog_sample[i].min, raw, analog_sample[i].max);)
      i++;
    }
    vTaskDelay(ONE_SECOND);
  }

  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}
