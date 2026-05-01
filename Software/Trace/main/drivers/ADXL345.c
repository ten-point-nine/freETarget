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
#define ADXL345_ADDR 0x53                   // I2C address of the ADXL345

#define BW_RATE_1600HZ 0x0F                 // BW_RATE register value for 1600 Hz data rate
#define BW_RATE_800HZ  0x0E                 // BW_RATE register value for 800 Hz data rate
#define BW_RATE_400HZ  0x0D                 // BW_RATE register value for 400 Hz data rate
#define BW_RATE_200HZ  0x0C                 // BW_RATE register value for 200 Hz data rate
#define BW_RATE_100HZ  0x0A                 // BW_RATE register value for 100 Hz data rate
#define BW_RATE_50HZ   0x09                 // BW_RATE register value for 50 Hz data rate
#define BW_RATE_25HZ   0x08                 // BW_RATE register value for 25 Hz data rate
#define BW_RATE_12_5HZ 0x07                 // BW_RATE register value for 12.5 Hz data rate
#define BW_RATE_6_25HZ 0x06                 // BW_RATE register value for 6.25 Hz data rate
#define BW_RATE_3_13HZ 0x05                 // BW_RATE register value for 3.13 Hz data rate
#define BW_RATE_1_56HZ 0x04                 // BW_RATE register value for 1.56 Hz data rate
#define BW_RATE_0_78HZ 0x03                 // BW_RATE register value for 0.78 Hz data rate
#define BW_RATE_0_39HZ 0x02                 // BW_RATE register value for 0.39 Hz data rate
#define BW_RATE_0_20HZ 0x01                 // BW_RATE register value for 0.20 Hz data rate

#define g2      0                           // Select +/- 2g range
#define g2_lsb  0.0039f                     // LSB value for +/- 2g range (4 mg/LSB)
#define g4      1                           // Select +/- 4g range
#define g4_lsb  0.0078f                     // LSB value for +/- 4g range (8 mg/LSB)
#define g8      2                           // Select +/- 8g range
#define g8_lsb  0.0156f                     // LSB value for +/- 8g range (16 mg/LSB)
#define g16     3                           // Select +/- 16g range
#define g16_lsb 0.0312f                     // LSB value for +/- 16g range (31.2 mg/LSB)

#define POWER_CTL_MEASURE 0x08              // POWER_CTL register value for Measure mode
#define DATA_FORMAT       (0b00001000 + g2) // DATA_FORMAT register
//                           |   ||        Selftest off
//                               ||        10 bit sample
//                                |        Justify right with sign extension
#define INT_ENABLE_NONE  0x00 // INT_ENABLE register value to disable all interrupts
#define INT_MAP_NONE     0x00 // INT_MAP register value to map all interrupts to INT1 pin
#define INT_SOURCE_CLEAR 0x00 // INT_SOURCE register value to clear all interrupts

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
    {0x31, DATA_FORMAT      }, // Data format register, Full resolution, right justified, +/- 2g range
    {0,    0                }  // End of the list
};

real_t adxl345_lsb_per_g[] = {g2_lsb, g4_lsb, g8_lsb, g16_lsb}; // LSB per g for each range setting

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

/*----------------------------------------------------------------
 *
 * @function: ADXL345_read_accel()
 *
 * @brief:    Read acceleration data from the ADXL345
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void ADXL345_read_raw_accel(ADXL345_sample_t *sample)
{
  unsigned char data[6];

  data[0] = 0x32;                           // Register address for the X-axis acceleration data
  i2c_write(ADXL345_ADDR, data, 1);         // Write the register address to the device
  i2c_read(ADXL345_ADDR, data, 6);          // Read 6 bytes of acceleration data (X, Y, Z)

  sample->raw_x = (data[1] << 8) | data[0]; // Combine low and high bytes for X-axis
  sample->raw_y = (data[3] << 8) | data[2]; // Combine low and high bytes for Y-axis
  sample->raw_z = (data[5] << 8) | data[4]; // Combine low and high bytes for Z-axis

  return;
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_convert_to_g()
 *
 * @brief:    Convert raw acceleration data to g
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Multiply the raw acceleration data by the LSB per g for the
 * current range setting to convert to g
 *
 *--------------------------------------------------------------*/
void ADXL345_convert_to_g(ADXL345_sample_t *sample)
{

  real_t lsb_per_g = adxl345_lsb_per_g[DATA_FORMAT & 0b00000011]; // Get the LSB per g for the current range setting

  sample->x = sample->raw_x * lsb_per_g;                          // Convert raw X-axis data to g
  sample->y = sample->raw_y * lsb_per_g;                          // Convert raw Y-axis data to g
  sample->z = sample->raw_z * lsb_per_g;                          // Convert raw Z-axis data to g

  return;
}
/*----------------------------------------------------------------
 *
 * @function: ADXL345_test()
 *
 * @brief:    Test the ADXL345
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Poll the ADXL345 and print out the acceleration data
 *
 *--------------------------------------------------------------*/
void ADXL345_test(void)
{
  ADXL345_sample_t sample;           // Buffer to hold the raw acceleration data

  while ( 1 )
  {
    ADXL345_read_raw_accel(&sample); // Read the acceleration data
    ADXL345_convert_to_g(&sample);   // Convert raw data to g
    SEND(ALL, sprintf(_xs, "\r\nraw X: %d, Y: %d, Z: %d", sample.raw_x, sample.raw_y, sample.raw_z);)
    SEND(ALL, sprintf(_xs, "  X: %.3fg, Y: %.3fg, Z: %.3fg", sample.x, sample.y, sample.z);)

    vTaskDelay(ONE_SECOND / 4);

    if ( serial_available(ALL) != 0 )
    {
      char ch = serial_getch(ALL);
      if ( ch == '!' )
      {
        break;
      }
    }
  }

  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
