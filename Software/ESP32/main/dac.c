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
#include "adc_oneshot.h"
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
#define DAC_ADDR  0x60    // DAC I2C address
#define DAC_WRITE 0x40    // Multi Write

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
 *   This function sets the DACs to the desired value
 *  
 *--------------------------------------------------------------*/
#define V_INTERNAL 0x80
#define V_REF_INT  2.048
#define V_EXTERNAL 0x00
#define V_REF_EXT  3.30

#define DAC_FS (0xfff)                    // 12 bit DAC

static int v_source = V_INTERNAL;         // Default to internal reference
static float v_ref  = V_REF_INT;          // Default to 2.048 volts

void DAC_write
(
  float volts[4]                          // What value are we setting it to
)
{
  unsigned char data[3*4];                // Bytes to send to the I2C
  unsigned int  scaled_value;             // Value (12 bits) to the DAC
  int i;
  int max;

/*
 *  Step 1, figure out what VREF should be
 */
  max = 0;
  for (i=0; i != 4; i++)
  {
    if ( volts[i] > max )
    {
      max = volts[i];
    }
  }

  if ( max < V_REF_INT )
  {
    v_source = V_INTERNAL;       // If the max setting is less than 2.048 Volts
    v_ref  = V_REF_INT;          // Default to the internal voltage
  }
  else
  {
    v_source = V_EXTERNAL;       // Otherwise 
    v_ref  = V_REF_EXT;          // Default to 5.0 volts
  }
  DLT(DLT_DIAG, SEND(sprintf(_xs, "DAC v_source:%d  v_ref:%4.2f)", v_source, v_ref);))

/*
 *  Fill up the I2C buffer
 */
  for (i=0; i != 4; i++)
  {
    scaled_value = ((int)(volts[i] / v_ref * DAC_FS)) & 0xfff;  // Figure the bits to send
    DLT(DLT_DIAG, SEND(sprintf(_xs, "DAC_write(channel:%d Volts:%4.2f scale:%d)", i+1, volts[i], scaled_value);))
    data[(i*3)+0] = DAC_WRITE             // Write
                  + ((i & 0x3) << 1)      // Channel
                  + 1;                    // UDAC = 1  update automatically
    data[(i*3)+1] = v_source              // Internal or external VREF
                  + 0x00                  // Normal Power Down
                  + 0x00                  // Gain x 1
                  + ((scaled_value >> 8
                     )  & 0x0f);          // Top 4 bits of the setting
    data[(i*3)+2] = scaled_value & 0xff;  // Bottom 8 bits of the setting
  }

  i2c_write(DAC_ADDR, data,  (4*3));      // Data transferred on last bit.

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
 * The DACS are ramped until a key is pressed
 * 
 ***************************************************************************/
void DAC_test(void)
{
  float volts[4];
  int i;

  SEND(sprintf(_xs, "\r\nDAC Test");)
  SEND(sprintf(_xs, "\r\nDAC 0 Up ramp");)
  SEND(sprintf(_xs, "\r\nDAC 1 Down ramp");)

  i = 0;
  while ( 1 )
  {
    if ( serial_available(CONSOLE) != 0 )
    {
      if ( serial_getch(CONSOLE) == '!' )
      {
        break;
      }
    }
    volts[VREF_LO] = 2.048*(float)(i%100)/100.0;
    volts[VREF_HI] = 2.048 - volts[VREF_LO];
    DAC_write(volts);
    vTaskDelay(5);
    i++;
  }

/*
 *  Test Complete
 */
  set_VREF();
  SEND(sprintf(_xs, "\r\nDone\r\n");)
  return;
}
