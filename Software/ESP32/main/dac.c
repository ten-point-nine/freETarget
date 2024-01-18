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

#include "freETarget.h"
#include "diag_tools.h"
#include "gpio.h"
#include "analog_io.h"
#include "json.h"
#include "serial_io.h"

/*
 *  Definitions
 */
#define DAC_ADDR  0x60    // DAC I2C address
#define DAC_WRITE 0x58    // Single write

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
#define V_REF 5.0                         // VREF = Supply, nominally 5V
#define DAC_FS (0xfff)                    // 12 bit DAC

void DAC_write
(
  const unsigned int channel,             // What register are we writing to
  const float        volts                // What value are we setting it to
)
{
  unsigned char data[3];                  // Bytes to send to the I2C
  unsigned int  scaled_value;             // Value (12 bits) to the DAC

  scaled_value = ((int)(volts / V_REF * DAC_FS)) & 0xfff;  // Figure the bits to send
  gpio_set_level(LDAC, 0);

  DLT(DLT_DIAG, printf("DAC_write(channel:%d Volts:%f scale:%d)", channel, volts, scaled_value);)

  data[0] = DAC_WRITE                     // Write
                + ((channel & 0x3) << 1)  // Channel
                + 0;                      // UDAC = 0  update now
  data[1] = 0x00                          // External Supply
                + 0x00                    // Normal Power Down
                + 0x00                    // Gain x 1
                + ((scaled_value >> 8) & 0x0f);// Top 4 bits of the setting
  data[2] = scaled_value & 0xff;          // Bottom 8 bits of the setting

  i2c_write(DAC_ADDR, data, 3 );
//  gpio_set_level(LDAC, 0);
//  gpio_set_level(LDAC, 1);

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
 * The DACS are ramped
 * 
 ***************************************************************************/
void DAC_test(void)
{
  float volts;

  printf("\r\nRamping DAC 0");
  volts = 0.0;
  while (volts < json_vref_lo )
  {
    DAC_write(0, volts);                 // Vref_LO ramps up
    volts += 0.0005;
    if ( serial_available(ALL) )
    {
      break;
    }
  }
  DAC_write(0, json_vref_lo);

  volts = 2.048;
  while (volts > json_vref_hi )
  {
    DAC_write(1, volts);                 // Vref_LO ramps up
    volts -= 0.0005;
    if ( serial_available(ALL) )
    {
      break;
    }
  }
  DAC_write(1, json_vref_hi);

/*
 *  Test Complete
 */
  printf("\r\nDone\r\n");
  return;
}
