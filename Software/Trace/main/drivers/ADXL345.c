/******************************************************************************
 *
 * file: ADXL345.c
 *
 * Analog Devices ADXL345 3-axis accelerometer driver
 *
 *****************************************************************************
 *
 * This file contains the driver for the ADXL345 3-axis accelerometer.  The
 * driver is written to be as generic as possible and should work with any
 * implementation of the ADXL345.
 *
 * See: https://www.analog.com/en/products/adxl345.html
 *      https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1021.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1020.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1022.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1023.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1024.pdf
 *
 *
 *****************************************************************************/
#include "stdio.h"
#include "driver\gpio.h"
#include "adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "i2c.h"
#include "math.h"

#include "trace.h"
#include "board_assembly.h"
// #include "helpers.h"
#include "diag_tools.h"
#include "gpio.h"
#include "json.h"
#include "serial_io.h"
#include "ADXL345.h"

/*
 * Definitions
 */
#define ADXL345_ADDR 0x53         // I2C address of the ADXL345

#define BW_RATE_1600HZ 0x0F       // BW_RATE register value for 1600 Hz data rate
#define BW_RATE_800HZ  0x0E       // BW_RATE register value for 800 Hz data rate
#define BW_RATE_400HZ  0x0D       // BW_RATE register value for 400 Hz data rate
#define BW_RATE_200HZ  0x0C       // BW_RATE register value for 200 Hz data rate
#define BW_RATE_100HZ  0x0A       // BW_RATE register value for 100 Hz data rate
#define BW_RATE_50HZ   0x09       // BW_RATE register value for 50 Hz data rate
#define BW_RATE_25HZ   0x08       // BW_RATE register value for 25 Hz data rate
#define BW_RATE_12_5HZ 0x07       // BW_RATE register value for 12.5 Hz data rate
#define BW_RATE_6_25HZ 0x06       // BW_RATE register value for 6.25 Hz data rate
#define BW_RATE_3_13HZ 0x05       // BW_RATE register value for 3.13 Hz data rate
#define BW_RATE_1_56HZ 0x04       // BW_RATE register value for 1.56 Hz data rate
#define BW_RATE_0_78HZ 0x03       // BW_RATE register value for 0.78 Hz data rate
#define BW_RATE_0_39HZ 0x02       // BW_RATE register value for 0.39 Hz data rate
#define BW_RATE_0_20HZ 0x01       // BW_RATE register value for 0.20 Hz data rate

#define POWER_CTL_MEASURE    0x08 // POWER_CTL register value for Measure mode
#define DATA_FORMAT_FULL_RES 0x0B // DATA_FORMAT register value for Full resolution, +/- 16g, right justified
#define INT_ENABLE_NONE      0x00 // INT_ENABLE register value to disable all interrupts
#define INT_MAP_NONE         0x00 // INT_MAP register value to map all interrupts to INT1 pin
#define INT_SOURCE_CLEAR     0x00 // INT_SOURCE register value to clear all interrupts

/*
 *  Typedefs
 */
typedef struct
{
  int address; // Device address
  int value;   // Value to write
} adxl345_write_t;

/*
 * Function Prototypes
 */

/*
 * Variables
 */
adxl345_write_t adxl345_init_data[] = {
    {0x2C, BW_RATE_100HZ    }, // BW_RATE register, 100 Hz data rate
    {0x2D, POWER_CTL_MEASURE}, // Power control register, Measure mode
    {0x2E, INT_MAP_NONE     }, // Interrupt map register, Map all interrupts to INT1 pin
    {0x2F, INT_SOURCE_CLEAR }, // Interrupt source register, Clear all interrupts
    {0x31, 0b00001000       }, // Data format register, Full resolution, Left justified, +/- 2g range
    {0,    0                }  // End of the list
};

/*----------------------------------------------------------------
 *
 * @function: ADXL345_init()
 *
 * @brief:    Initalize the ADXL345
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void ADXL345_init(void)
{
  int           i;
  unsigned char data[2];           // Bytes to send to the I2C

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "ADXL345_init()");))

  data[0] = 0x00;                  // Register address for the device ID
  i2c_write(ADXL345_ADDR, data, 1);
  i2c_read(ADXL345_ADDR, data, 1); // Read the device ID to verify communication
  if ( data[0] != 0xE5 )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "ADXL345 not found! Device ID: 0x%02X", data[0]);))
    return;
  }

  i = 0;
  while ( adxl345_init_data[i].address != 0 )
  {
    data[0] = adxl345_init_data[i].address; // Register address
    data[1] = adxl345_init_data[i].value;   // Value to write
    i2c_write(ADXL345_ADDR, data, 2);       // Data transferred on last bit.
    i++;
  }

  /*
   *  All done, return;
   */
  return;
}
