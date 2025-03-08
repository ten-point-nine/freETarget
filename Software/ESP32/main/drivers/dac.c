/******************************************************************************
 *
 * file: dac.c
 *
 * Use the external DAC to set the reference voltages
 *
 *****************************************************************************
 *
 * See: https://www.microchip.com/en-us/product/MCP4728#document-table
 * https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/22187E.pdf
 *
 *****************************************************************************/
#include "stdio.h"
#include "driver\gpio.h"
#include "adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "i2c.h"
#include "math.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "gpio.h"
#include "analog_io.h"
#include "json.h"
#include "serial_io.h"
#include "dac.h"

/*
 *  Definitions
 */
#define DAC_ADDR   0x60    // DAC I2C address
#define DAC_WRITE  0x40    // Multi Write
#define V_INTERNAL 0x80    // Select internal refernce in DAC
#define VREF_INT   2.048   // Internal reference set at 2.048V
#define V_EXTERNAL 0x00    // Select exteranl referece to DAC
#define VREF_EXT   5.0     // Exterma; referemce nominally 5V (not very regulated)
#define DAC_FS     (0xfff) // 12 bit DAC

/*----------------------------------------------------------------
 *
 * @function: DAC_write()
 *
 * @brief:    Write a value (in mV) to the DAC
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
void DAC_write(float volts[4]          // What value are we setting it to
)
{
  unsigned char data[3 * 4];           // Bytes to send to the I2C
  unsigned int  scaled_value;          // Value (12 bits) to the DAC
  int           i;
  float         max;
  int           v_source = V_INTERNAL; // Default to internal reference
  float         v_ref    = VREF_INT;   // Default to 2.048 volts

  /*
   *  Step 1, figure out what VREF should be
   */
  max = 0;
  for ( i = 0; i != 4; i++ )
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
  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "DAC v_source:%d  v_ref:%4.2f)", v_source, v_ref);))

  /*
   *  Fill up the I2C buffer
   */
  for ( i = 0; i != 4; i++ )
  {
    scaled_value = ((int)(volts[i] / v_ref * DAC_FS)) & 0xfff; // Figure the bits to send
    DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "DAC_write(channel:%d Volts:%4.2f scale:%d)", i + 1, volts[i], scaled_value);))
    data[(i * 3) + 0] = DAC_WRITE                              // Write
                        + ((i & 0x3) << 1)                     // Channel
                        + 1;                                   // UDAC = 1  update automatically
    data[(i * 3) + 1] = v_source                               // Internal or external VREF
                        + 0x00                                 // Normal Power Down
                        + 0x00                                 // Gain x 1
                        + ((scaled_value >> 8) & 0x0f);        // Top 4 bits of the setting
    data[(i * 3) + 2] = scaled_value & 0xff;                   // Bottom 8 bits of the setting
  }

  i2c_write(DAC_ADDR, data, (4 * 3));                          // Data transferred on last bit.

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
    volts[VREF_LO] = VREF_EXT * ((float)(i % 200) / 200.0);        // Ramp Up
    volts[VREF_HI] = VREF_EXT * ((float)((i - 10) % 200) / 200.0); // Ramp Up delayed
    volts[VREF_2]  = 0.0;
    volts[VREF_3]  = 0.0;

    DAC_write(volts);
    vTaskDelay(TICK_10ms);
    i++;
  }

  /*
   *  Test Complete
   */
  set_VREF();
  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}
