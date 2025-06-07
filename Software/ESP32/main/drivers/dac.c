/******************************************************************************
 *
 * file: dac.c
 *
 * Use the external DAC to set the reference voltages
 *
 *****************************************************************************
 *
 * 4 Channel DAC MCP4728
 * See: https://www.microchip.com/en-us/product/MCP4728#document-table
 * https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/22187E.pdf
 *
 * 1 Channel DAC MCP4725
 * https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP4725-12-Bit-Digital-to-Analog-Converter-with-EEPROM-Memory-DS20002039.pdf
 *
 *****************************************************************************/
#include "stdio.h"
#include "driver\gpio.h"
#include "adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "i2c.h"
#include "math.h"

#include "freETarget.h"
#include "board_assembly.h"
#include "helpers.h"
#include "diag_tools.h"
#include "gpio.h"
#include "analog_io.h"
#include "json.h"
#include "serial_io.h"
#include "dac.h"

/*
 *  Definitions
 */
#define DAC_MCP4728_ADDR  0x60    // DAC I2C address
#define DAC_MCP4728_WRITE 0x0b    // Multi Write
#define UDAC              0x01    // Output immediatly
#define V_INTERNAL        0x80    // Select internal refernce in DAC
#define VREF_INT          2.048   // Internal reference set at 2.048V
#define V_EXTERNAL        0x00    // Select exteranl referece to DAC
#define VREF_EXT          5.0     // Exterma; referemce nominally 5V (not very regulated)
#define DAC_FS            (0xfff) // 12 bit DAC{}

/*
 * Function Prototypes
 */
static void DAC_write_MCP4728(float volts[]); // What value are we setting it to
static void DAC_write_MCP4725(float volts[]); // What value are we setting it to

/*----------------------------------------------------------------
 *
 * @function: DAC_write()
 *
 * @brief:    Route the DAC request to the correct hardware
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void DAC_write(float volts[]) // What value are we setting it to
{
  if ( MCP4728 & board_mask )
  {
    DAC_write_MCP4728(volts); // MCP 4728 (four channel`)
  }
  else
  {
    DAC_write_MCP4725(volts); // MCP 4725 (single channel)
  }

  /*
   *  All done, return;
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: DAC_write_MCP4728()
 *
 * @brief:    Write a value (in Volts) to the DAC
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * This function sets the DACs to the desired value.
 *
 * The DAC has two modes of operation:
 *  * Internal from the precision 2.048 reference
 *  * External from the VCC supply (3.3 or 5.0V)
 *
 * The function looks as the selected voltage and determines
 * if it can be done by the internal reference or the external
 * one and selects the source automatically.
 *
 *--------------------------------------------------------------*/
static void DAC_write_MCP4728(float volts[]) // What value are we setting it to
{
  unsigned char data[3];                     // Bytes to send to the I2C
  unsigned int  scaled_value;                // Value (12 bits) to the DAC
  int           i;
  float         max;
  int           v_source      = V_INTERNAL;  // Default to internal reference
  float         v_ref         = VREF_INT;    // Default to 2.048 volts
  unsigned int  channel_count = 1;           // Program one channel

  /*
   *  Check for a valid voltage setting
   */
  for ( i = 0; i != channel_count; i++ )
  {
    if ( volts[i] < 0 )
    {
      volts[i] = 0;
    }
    if ( volts[i] > VREF_EXT )
    {
      volts[i] = VREF_EXT;
    }
  }
  /*
   *  Step 1, figure out what VREF should be
   */
  max = 0;
  for ( i = 0; i != channel_count; i++ )
  {
    if ( volts[i] > max )
    {
      max = volts[i];
    }
  }

  if ( max < VREF_INT )
  {
    v_source = V_INTERNAL; // If the max setting is less than 2.048 Volts
    v_ref    = VREF_INT;   // Default to the internal voltage
  }
  else
  {
    v_source = V_EXTERNAL; // Otherwise
    v_ref    = VREF_EXT;   // Default to 5.0 volts
  }

  /*
   *  Fill up the I2C buffer
   */
  for ( i = 0; i != channel_count; i++ )
  {
    scaled_value      = ((int)(volts[i] / v_ref * DAC_FS)) & 0xfff; // Figure the bits to send
    data[(i * 3) + 0] = (DAC_MCP4728_WRITE << 3)                    // Write
                        + ((i & 0x3) << 1)                          // Channel
                        + UDAC;                                     // UDAC = 1  update automatically
    data[(i * 3) + 1] = v_source                                    // Internal or external VREF
                        + 0x00                                      // Normal Power Down
                        + 0x00                                      // Gain x 1
                        + ((scaled_value >> 8) & 0x0f);             // Top 4 bits of the setting
    data[(i * 3) + 2] = scaled_value & 0xff;                        // Bottom 8 bits of the setting
    i2c_write(DAC_MCP4728_ADDR, data, sizeof(data));                // Data transferred on last bit.
  }

  /*
   *  All done, return;
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: DAC_write_MCP4725()
 *
 * @brief:    Write a value (in Volts) to the DAC
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * This function sets the DAC
 *
 *
 *--------------------------------------------------------------*/
#define VREF_MCP4725      5.0                                       // External reference for the MCP4725
#define DAC_MCP4725_WRITE 0x00                                      // Write to the DAC

static void DAC_write_MCP4725(float volts[])                        // What value are we setting it to
{
  unsigned char data[4];                                            // Bytes to send to the I2C
  unsigned int  scaled_value;                                       // Value (12 bits) to the DAC

  scaled_value = ((int)(volts[0] / VREF_MCP4725 * DAC_FS)) & 0xfff; // Figure the bits to send
  data[0]      = (DAC_MCP4725_WRITE << 6)                           // Write
            + (0x00 << 4)                                           // Power Down Select
            + ((scaled_value >> 8) & 0x0f);                         // Top 4 bits of the setting
  data[1] = scaled_value & 0xff;                                    // Bottom 8 bits of the setting
  data[2] = data[0];                                                // Repeat bytes
  data[3] = data[1];                                                // Repeat bytes
  i2c_write(DAC_MCP4725_ADDR, data, sizeof(data));                  // Data transferred on last bit.

  /*
   *  All done, return;
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: DAC_read()
 *
 * @brief:    Read the DAC registers
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/
void DAC_read(void)          // What value are we setting it to
{

  unsigned char data[4 * 6]; // Bytes read from the I2C
  unsigned int  i;           // Iteration counter
  char          str[20];     // String hold binary number
  unsigned int  sizeof_data;

  SEND(ALL, sprintf(_xs, "DAC_read: ");)

  if ( MCP4728 & board_mask )
  {
    sizeof_data = sizeof(data);                     // MCP4728 (4 channel`)
    i2c_read(DAC_MCP4728_ADDR, data, sizeof(data)); // Data transferred on last bit.
  }
  else
  {
    sizeof_data = 5;                                // MCP4725 (single channel)
    i2c_read(DAC_MCP4725_ADDR, data, sizeof_data);  // Data transferred on last bit.
  }

  for ( i = 0; i != sizeof_data; i++ )
  {
    to_binary(data[i], 8, str);                     // Convert to binary string
    SEND(ALL, sprintf(_xs, "0b%s ", str);)
    if ( ((i + 1) % 6) == 0 )
    {
      SEND(ALL, sprintf(_xs, "\r\n");)
    }
  }

  /*
   *  All done, return;
   */
  return;
}

/*************************************************************************
 *
 * @function:     DAC_test
 *
 * @description:  Ramp the DACs
 *
 * @return:       None
 *
 **************************************************************************
 *
 * The DACs are ramped from 0 to VREF_EXT (5V) in a smooth ramp.
 *
 * Below 2.048 Volts, the output is generated from the internal precision
 * voltage reference.  Above 2.045 the VREF is switched to the external
 * supply, nominally 5V.
 *
 * The test demonstrates
 *    1 - The output is continious between the limits
 *    2 - There is a smooth transition from the internal and external
 *        reference voltages.
 *
 * When working correctly, the output is two ramps, the VREF_HI output
 * slightly delayed from VREF_LO
 ***************************************************************************/
void DAC_test(void)
{
  float volts[4];
  int   i;

  SEND(ALL, sprintf(_xs, "\r\nDAC 0 Up ramp 0-5V");)
  if ( MCP4728 & board_mask )
  {
    SEND(ALL, sprintf(_xs, "\r\nDAC 1 Up ramp 0-5V delayed");)
  }

  SEND(ALL, sprintf(_xs, "\r\nDAC 1 Up ramp delayed");)
  SEND(ALL, sprintf(_xs, "\r\nPress ! to end test\r\n");)

  i = 100;
  while ( 1 )
  {
    if ( serial_available(CONSOLE) != 0 )
    {
      if ( serial_getch(CONSOLE) == '!' )
      {
        break;
      }
    }
    if ( MCP4728 & board_mask )
    {
      volts[VREF_LO] = VREF_EXT * ((float)(i % 200) / 200.0);                  // Ramp Up
      volts[VREF_HI] = VREF_EXT * ((float)((i - 10) % 200) / 200.0);           // Ramp Up delayed
      volts[VREF_2]  = 0.0;
      volts[VREF_3]  = 0.0;
    }
    else
    {
      volts[VREF_LO] = VREF_EXT * ((float)(i % 200) / 200.0);                  // Ramp Up
    }

    DAC_write(volts);
    vTaskDelay(TICK_10ms * 100);                                               // Wait 100ms

    if ( MCP4725 & board_mask )
    {
      printf("\r\nWrite: %4.2f  Read: %4.2f", volts[VREF_LO], vref_measure()); // Read the VREF voltage
    }
    else
    {
      printf("\r\nWrite: %4.2f, %4.2f", volts[VREF_LO], volts[VREF_HI]);       // Read the VREF voltage
    }

    i++;
  }

  /*
   *  Test Complete
   */
  set_VREF();
  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}
