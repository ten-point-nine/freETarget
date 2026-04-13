/******************************************************************************
 *
 * analog_io.c
 *
 * General purpose Analog driver
 *
 *****************************************************************************
 *
 * Revised for oneshot operation
 *
 * See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
 *
 * This file manages the analog inputs and outputs, including the ADC, DAC, and PWM.
 *
 *****************************************************************************/

#include "stdbool.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
// #include "driver/adc.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"

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
static real_t temperature_C_HDC3022(void);  // Temperature in degrees C
static real_t temperature_C_TMP1075D(void); // Temperature in degrees C
static bool   adc_calibration_init(int unit, int channel, int atten, adc_cali_handle_t *out_handle);

/*
 *  Variables
 */
int           board_version = -1;                                     // Board Revision number
unsigned int  board_mask    = 0;                                      // Mask for the board revision
static real_t rh;                                                     // Humidity from sensor

static bool                            adc_used[2] = {false, false};  // Track which ADC channels are in use
static adc_oneshot_unit_handle_t       adc_handle[2];                 // ADC handles for ADC1 and ADC2
static adc_oneshot_unit_init_cfg_t     adc_init_config[2];            // ADC unit configuration for ADC1 and ADC2
static adc_oneshot_chan_cfg_t          channel_config[2][10];         // Channel configuration for each channel
static adc_cali_handle_t               adc_calibration_handle[2][10]; // Calibration handles for ADC1 and ADC2
static adc_cali_curve_fitting_config_t adc_calibration_config[2][10]; // Calibration configuration for ADC1 and ADC2
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
 * https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/api-reference/peripherals/adc/adc_oneshot.html
 *
 *--------------------------------------------------------------*/
void adc_init(unsigned int adc_channel,    // What ADC channel are we accessing
              unsigned int adc_attenuation // What is the channel attenuation
)
{
  int adc     = ADC_ADC(adc_channel);      // Which ADC (1/2)
  int channel = ADC_CHANNEL(adc_channel);  // Which channel attached to the ADC (0-9)

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initializing ADC%d channel %d with attenuation %d  ", adc, channel, adc_attenuation);))

  /*
   *  Initialize the ADC unit if not already initialized
   */
  if ( adc_used[adc] == false )                                                     // Check if the ADC unit is already initialized
  {
    adc_init_config[adc].unit_id = adc;                                             // ADC unit configuration
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config[adc], &adc_handle[adc])); // Create a new ADC unit handle
    adc_used[adc] = true;                                                           // Mark the ADC unit as used
  }

  /*
   *  Configure the ADC channel
   */
  channel_config[adc][channel].bitwidth = ADC_BITWIDTH_12; // 12-bit resolution
  channel_config[adc][channel].atten    = adc_attenuation;
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle[adc], channel, &channel_config[adc][channel]));

  /*
   *  Initialize calibration
   */
  adc_calibration_handle[adc][channel] = NULL;
  adc_calibration_init(adc, channel, adc_attenuation, &adc_calibration_handle[adc][channel]);
  printf("ADC%d channel %d: Calibration handle: %p\n", adc, channel, adc_calibration_handle[adc][channel]);
  adc_cali_curve_fitting_config_t calibration_config = {
      .unit_id = adc, .chan = channel, .atten = adc_attenuation, .bitwidth = ADC_BITWIDTH_DEFAULT};

  esp_err_t ret = adc_cali_create_scheme_curve_fitting(&calibration_config, &adc_calibration_handle[adc][channel]);

  if ( ret != ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Calibration failed %d", ret);))
  }

  /*
   *  All done
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: adc_calibration_init
 *
 * @brief:  Setup the calibration for the ADC channel
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Using curve fitting calibration.  See ESP-IDF documentation for details
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html#calibration
 *
 *--------------------------------------------------------------*/

static bool adc_calibration_init(int                adc,         // Which ADC (1/2)
                                 int                channel,     // Which channel attached to the ADC (0-10)
                                 int                attenuation, // Attenuation level
                                 adc_cali_handle_t *out_handle   // Output handle for the calibration
)
{
  adc_cali_curve_fitting_config_t calibration_config = {
      .unit_id  = adc,
      .chan     = channel,
      .atten    = attenuation,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  /*
   *  Create the calibration scheme
   */
  if ( adc_cali_create_scheme_curve_fitting(&calibration_config, out_handle) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "ADC%d channel %d: Calibration failed", adc, channel);))
    return false;
  }

  return true;
}

/*----------------------------------------------------------------
 *
 * @function: adc_read()
 *
 * @brief:  Read a value from the ADC channel
 *
 * @return: Analog value for the ADC channel in mV
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
#define FILTER 16                                  // How many averages

int adc_read(int adc_channel)                      // What input are we reading?
{
  unsigned int adc     = ADC_ADC(adc_channel);     // Which ADC (1/2)
  unsigned int channel = ADC_CHANNEL(adc_channel); // Which channel attached to the ADC (0-10)
  int          raw;                                // Raw value from the ADC
  int          sum;
  int          i;
  int          volt_mV;                            // Voltage in mV

  printf("Reading ADC%d channel %d\n", adc, channel);

  /*
   *  Read the appropriate channel
   */
  sum = 0;
  for ( i = 0; i != FILTER; i++ ) // Add up FILTER samples
  {
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle[adc], channel, &raw));
    sum += raw;
  }

  /*
   *  Done
   */
  sum /= FILTER;
  sum &= 0x0fff;                              // Mask to 12 bits

  printf("ADC%d channel %d: Raw reading: %d\n", adc, channel, sum);

  adc_cali_raw_to_voltage(adc_calibration_handle[adc][channel], sum,
                          &volt_mV);          // Convert the ADC reading to millivolts using the calibration handle

  printf("ADC%d channel %d: Calibrated voltage: %d mV\n", adc, channel, volt_mV);
  return (volt_mV);
}

#define V12_RESISITOR ((40200 + 4700) / 4700) // Resistor divider

real_t v12_supply(void)
{
  int volt_mV;

  volt_mV = adc_read(V12_LED);                // Read the ADC value for the board revision referenced to 3.3 volts

  return (real_t)volt_mV / 1000.0 * V12_RESISTOR;
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
 *  The ADC for the ESP32 is accurate +/-10%.
 *
 *  To find the board revision, the board revision is read and
 *  provides a number 0-4096 (12 bits).  The values of the
 *  resistor divider for each board revision are known, and the
 *  ideal ADC reading for each board revision can be calculated.
 *  The actual ADC reading is compared to the ideal readings,
 *  and the closest match is used to determine the board revision.
 *
 *--------------------------------------------------------------*/
//                                                       0     1  2     3     4  5     6     7  8  9   A     B      C   D   E    F
const static unsigned int       version[]          = {REV_510, 1, 2, REV_610, 4, 5, REV_600, 7, 8, 9, 10, REV_620, 12, 13, 14, REV_520};
const static BD_REV_resistors_t bd_rev_resistors[] = {
    {10000, 0     }, // 0 5.1
    {10000, 1428  }, // 1
    {10000, 2308  }, // 2
    {10000, 3300  }, // 3 6.1

    {10000, 4545  }, // 4
    {10000, 6000  }, // 5
    {10000, 7777  }, // 6 6.0

    {10000, 10000 }, // 7
    {10000, 12857 }, // 8
    {10000, 16666 }, // 9

    {10000, 22222 }, // 10
    {10000, 30000 }, // 11 6.2
    {10000, 43333 }, // 12
    {10000, 70000 }, // 13
    {10000, 150000}, // 14
    {0,     1E6   }, // 15 5.2
};

unsigned int revision(void)
{
  int index;                            // Index into the version table
  int adc_reading;                      // ADC reading
  int adc_ideal;                        // Ideal ADC reading for this revision
  int i;                                // Loop counter
  int distance;                         // Distance from the ideal reading
  int milliVolts;                           // Voltage reading from the ADC
  int adc     = ADC_ADC(BOARD_REV);     // ADC to use
  int channel = ADC_CHANNEL(BOARD_REV); // Channel to use

  if ( board_version >= 0 )             // Already read the revision?
  {
    return board_version;               // Return the cached value
  }

  /*
   *  Read the resistors and determine the board revision
   */
  milliVolts = adc_read(BOARD_REV);                                 // Resistor divider in mV
  printf("milliVolts: %d\n", milliVolts);

  distance = 0x0fff;                                            // Start with the maximum possible distance

  for ( i = 0; i != sizeof(version) / sizeof(version[0]); i++ ) // Loop through the revisions
  {
    if ( version[i] >= REV_500 ) // Only consider revisions 5.0 and later, since the resistor values are well defined before then
    {
      adc_ideal = 3300 * (real_t)bd_rev_resistors[i].r2 /
                  (bd_rev_resistors[i].r1 + bd_rev_resistors[i].r2); // Calculate the ideal ADC reading for this revision

      if ( abs(milliVolts - adc_ideal) < distance ) // Is this revision closer to the actual reading than the previous best?
      {
        distance = abs(milliVolts - adc_ideal);     // Update the closest distance
        index    = i;                           // Update the index of the closest revision
      }
    }
  }

  board_version = version[index];               // Get the board revision number
  board_mask    = 1 << index;                   // Set the mask for the board revision

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Board Version: %d.%d.%d  Board Mask: 0X%04X", (board_version / 100), ((board_version % 100) / 10),
                                  (board_version % 10), board_mask);))
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
real_t vref_measure(void)
{
  if ( TMP1075D )
  {
    return ((real_t)adc_read(VMES_LO)) / ADC_FULL * ADC_REF * VREF_DIVIDER + ADC_BIAS; // 4096 full scale, 3.3 VREF 1/2 voltage divider
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
real_t temperature_C(void)
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
static real_t temperature_C_HDC3022(void)
{
  unsigned char temp_buffer[6];
  int           raw;
  static real_t t_c; // Remember the temperature

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
  t_c = -45.0 + (175.0 * (real_t)raw / 65535.0);
  raw = (temp_buffer[3] << 8) + temp_buffer[4];
  rh  = 100.0 * (real_t)raw / 65535.0;

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
static real_t temperature_C_TMP1075D(void)
{
  unsigned char temp_buffer[6];
  int           raw;
  static real_t t_c = -274;  // Remember the temperature Set to below absolute zero

                             /*
                              * Check for a Rev 6.0 board
                              */
  if ( board_version == REV_600 ) // Revision 6.0 board?
  {
    if ( v12_supply() >= 5.0 )    // 12V supply present.  Possible self heating.
    {
      if ( t_c > -273 )           // If we have a valid temperature, return it
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
  t_c = ((real_t)raw * TC_CAL);
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
real_t humidity_RH(void)
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
  real_t volts[4];

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
  SEND(ALL, sprintf(_xs, "\r\nBoard Rev: %4.2f", (real_t)revision() / 100.0);)
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
    {"12V",     V12_LED,   0xffff, 0},
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
