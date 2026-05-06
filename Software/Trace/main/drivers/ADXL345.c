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
 * See: https://www.analog.com/en/products/ADXL345.html
 *      https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1021.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1020.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1022.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1023.pdf
 *      https://www.analog.com/media/en/technical-documentation/application-notes/AN-1024.pdf
 *
 *
 * IMPORTANT
 *
 * This driver polls the FIFO to remove samples when the watermark is met.
 * The interrupt is not used to trigger the reading of the FIFO.  This is because
 * the I2C driver does not work with interrupts.
 *
 *****************************************************************************/
#include "stdio.h"
#include "driver\gpio.h"
#include "i2c.h"
#include "math.h"
#include "assert.h"

#include "trace.h"
#include "board_assembly.h"
#include "diag_tools.h"
#include "gpio.h"
#include "json.h"
#include "helpers.h"
#include "ADXL345.h"

/*
 * Definitions
 */

#define ADXL345_ADDR 0x53                   // I2C address of the ADXL345

#define BW_RATE_3200HZ 0x0F                 // BW_RATE register value for 1600 Hz data rate
#define BW_RATE_1600HZ 0x0E                 // BW_RATE register value for 1600 Hz data rate
#define BW_RATE_800HZ  0x0D                 // BW_RATE register value for 800 Hz data rate
#define BW_RATE_400HZ  0x0C                 // BW_RATE register value for 400 Hz data rate
#define BW_RATE_200HZ  0x0B                 // BW_RATE register value for 200 Hz data rate
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
#define INT_WATERMARK    0x02 // Interrupt when watermark reached
#define INT_SOURCE_CLEAR 0x00 // INT_SOURCE register value to clear all interrupts

#define SQ(x) ((x) * (x))
/*
 *  Typedefs
 */
typedef struct
{
  int address; // Device address
  int value;   // Value to write
} ADXL345_write_t;

/*
 * Function Prototypes
 */

/*
 * Variables
 */
ADXL345_write_t ADXL345_init_data[] = {
    {0x2C, BW_RATE_200HZ  }, // BW_RATE register
    {0x2D, 0b00001000     }, // Power control register, Measure mode
    {0x2E, 0b00000010     }, // Interrupt map register, Watermark,  Map all interrupts to INT1 pin
    {0x2F, 0b00000000     }, // Send all interrupts to INT1
    {0x31, 0b00100000     }, // Data format register, Full resolution, Interrupt low, right justified, +/- 2g range
    {0x38, 0b10000000 + 20}, // FIFO mode, Stream, send INT to INT1, interrupt on 20 samples
    {0,    0              }  // End of the list
};

real_t      ADXL345_lsb_per_g[] = {g2_lsb, g4_lsb, g8_lsb, g16_lsb}; // LSB per g for each range setting
trace_raw_t ADXL345_zero_sample;                                     // Sample to hold the zeroed acceleration data

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
 * Setup the accelerometer from the table
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
  while ( ADXL345_init_data[i].address != 0 )
  {
    data[0] = ADXL345_init_data[i].address; // Register address
    data[1] = ADXL345_init_data[i].value;   // Value to write
    i2c_write(ADXL345_ADDR, data, 2);       // Data transferred on last bit.
    i++;
  }

  /*
   * Clear any internal data
   */
  for ( i = 0; i != SAMPLE_DEPTH; i++ )
  {
    samples[i].x = 0;
    samples[i].y = 0;
    samples[i].z = 0;
  }

  ADXL345_zero_sample.x = 0;
  ADXL345_zero_sample.y = 0;
  ADXL345_zero_sample.z = 0;

  /*
   *  All done, return;
   */

  while ( gpio_get_level(FIFO_INTERRUPT) != 0 )
  {
    ADXL345_read_raw_accel(&ADXL345_zero_sample, false);
  }
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_FIFO_read
 *
 * @brief:    Pull all of the samples out of the FIFO and store them in the sample buffer
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function is called if the FIFO watermark is reached.
 * It reads all the samples in the FIFO and stores them in the
 * sample buffer.
 *
 *---------------------------------------------------------------*/
void ADXL345_FIFO_read(void)
{
  unsigned char data[8];
  unsigned int  i;

  run_state |= IN_COLLECTION;

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "ADXL345_FIFO_read()");))

  i = 0;
  do
  {
    data[0] = 0x32;                                          // Register address for the X-axis acceleration data
    i2c_write(ADXL345_ADDR, data, 1);                        // Write the register address to the device
    i2c_read(ADXL345_ADDR, data, sizeof(data));              // Read 6 bytes of acceleration data (X, Y, Z)

    samples[sample_in].x = (data[1] << 8) | data[0];         // Combine low and high bytes for X-axis
    samples[sample_in].y = (data[3] << 8) | data[2];         // Combine low and high bytes for Y-axis
    samples[sample_in].z = (data[5] << 8) | data[4];         // Combine low and high bytes for Z-axis
    sample_in            = (sample_in + 1) % (SAMPLE_DEPTH); // Point to the next entry
    i++;
  } while ( (data[7] & 0x1F) != 0 );

                                                             /*
                                                              * Return from interrupts
                                                              */
  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "Samples read: %d", i);))
  run_state &= ~IN_COLLECTION;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_read_accel()
 *
 * @brief:    Read acceleration data from the ADXL345
 *
 * @return:   Number of bytes remaining in the FIFO
 *
 *----------------------------------------------------------------
 *
 * The Acceleration data is read from the ADXL345 in 6 bytes,
 * with the X, Y, and Z axis data each consisting of a low byte
 * followed by a high byte. The raw acceleration data is stored
 * in the provided sample structure.
 *
 * The raw acceleration data is in 10-bit resolution and is
 * right justified with sign extension.
 *
 * The function removes the DC bias for a level sensor
 * by subtracting the zero offset from the raw data if the
 * zero_offset parameter is true.
 *
 *--------------------------------------------------------------*/
unsigned int ADXL345_read_raw_accel(trace_raw_t *sample,     // Where to save results
                                    bool         zero_offset // TRUE if a zero offset it to be applied
)
{
  int samples_remaining;                                     // Number of samples remaining in the FIFO

  /*
   * Check if there are any samples available
   */
  if ( sample_in == sample_out )
  {
    return 0; // No new samples available
  }

  /*
   * Read the sample from the buffer
   */
  *sample    = samples[sample_out];               // Read from the sample buffer
  sample_out = (sample_out + 1) % (SAMPLE_DEPTH); // Point to the next entry

  if ( zero_offset == true )
  {
    sample->x -= ADXL345_zero_sample.x;           // Add the zero offset to the raw data
    sample->y -= ADXL345_zero_sample.y;           // to remove the DC bias for a level sensor
    sample->z -= ADXL345_zero_sample.z;
  }

  /*
   *  Return the number of samples remaining
   */
  samples_remaining = sample_in - sample_out; // Calculate the number of samples remaining in the buffer
  if ( samples_remaining < 0 )
  {
    samples_remaining += SAMPLE_DEPTH;        // Adjust for wrap-around
  }

  return samples_remaining;                   // Return FIFO samples remaining // 0x39
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_zero()
 *
 * @brief:    Determine the resting g levels for the ADXL345
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The acceleration data always contains the earth's gravity,
 * so to get the actual acceleration of the device, we need to
 * zero the data by taking a sample when the device is stationary
 * and subtracting that from future samples.
 *
 *--------------------------------------------------------------*/
#define NUM_ZERO_SAMPLES   100
#define SCALE_ZERO_SAMPLES 1

trace_raw_t zero_samples;  // Buffer to hold multiple samples for averaging

void ADXL345_find_zero(void)
{
  int         i;
  trace_raw_t ADXL345_raw; // As read from the accelerometer
  trace_raw_t ADXL345_sum;
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "ADXL345_find_zero()");))

  /*
   * Loop and collect samples
   */

  ADXL345_sum.x = 0; // Zero out the sum
  ADXL345_sum.y = 0;
  ADXL345_sum.z = 0;

  i = 0;
  while ( i != NUM_ZERO_SAMPLES )
  {
    if ( ADXL345_read_raw_accel(&ADXL345_raw, false) != 0 ) // Take a sample of the raw acceleration data
    {
      ADXL345_sum.x += ADXL345_raw.x;                       // Accumulate the X-axis raw acceleration data
      ADXL345_sum.y += ADXL345_raw.y;                       // Accumulate the Y-axis raw acceleration data
      ADXL345_sum.z += ADXL345_raw.z;                       // Accumulate the Z-axis raw acceleration data
      i++;
      printf("Sample %d: X: %04X, Y: %04X, Z: %04X\r\n", i, ADXL345_raw.x, ADXL345_raw.y, ADXL345_raw.z);
    }
  }

  /*
   * Average the samples to get a more accurate zero level
   */
  ADXL345_zero_sample.x = ADXL345_sum.x / NUM_ZERO_SAMPLES; // Average the X-axis raw acceleration data
  ADXL345_zero_sample.y = ADXL345_sum.y / NUM_ZERO_SAMPLES; // Average the Y-axis raw acceleration data
  ADXL345_zero_sample.z = ADXL345_sum.z / NUM_ZERO_SAMPLES; // Average the Z-axis raw acceleration data

  /*
   *  All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Axis offset - X: %04X, Y: %04X, Z: %04X", ADXL345_zero_sample.x, ADXL345_zero_sample.y,
                                  ADXL345_zero_sample.z);))

  set_status_LED(LED_READY); // Indicate that we are ready
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
void ADXL345_convert_to_g(trace_raw_t *sample, trace_point_t *actual)
{
  real_t lsb_per_g = ADXL345_lsb_per_g[DATA_FORMAT & 0b00000011]; // Get the LSB per g for the current range setting

  actual->ax = (sample->x) * lsb_per_g;                           // Convert raw X-axis data to g
  if ( F_ABS(actual->x) < 0.010 )
  {
    actual->ax = 0;
  }

  actual->ay = (sample->y) * lsb_per_g;                           // Convert raw Y-axis data to g
  if ( F_ABS(actual->ay) < 0.010 )
  {
    actual->ay = 0;
  }

  actual->az = (sample->z) * lsb_per_g;                           // Convert raw Z-axis data to g
  if ( F_ABS(actual->az) < 0.010 )
  {
    actual->az = 0;
  }
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
#define TIME_STEP    (0.010)
#define TIME_STEP_SQ (TIME_STEP * TIME_STEP)
#define G_mm_s2      9806.65

void ADXL345_test(void)
{
  real_t              vector_magnitude;                           // Magnitude of the acceleration vector
  ADXL345_trace_raw_t previous;                                   // Previous sample
  ADXL345_trace_raw_t present;                                    // Present sample

  ADXL345_read_raw_accel(&previous, true);

  while ( ADXL345_read_raw_accel(&present, true) != 0 )           // Read the acceleration dataP )
  {
    ADXL345_convert_to_g(&samples[next_sample], &present);        // Convert raw data to g

    vector_magnitude = sqrt(SQ(present.ax) + SQ(present.ay) + SQ(present.az)); // Calculate the magnitude of the acceleration vector

    /*
     * Integrate the positition
     */
    present.x = (previous.x) + (previous.vx * TIME_STEP) +
                (present.ax * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the X position using the acceleration data
    present.y = (previous.y) + (previous.vy * TIME_STEP) +
                (present.ay * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Y position using the acceleration data
    present.z = (previous.z) + (previous.vz * TIME_STEP) +
                (present.az * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Z position using the acceleration data
    /*
     * Integrate the velocity
     */
    present.vx = previous.vx + (present.ax * G_mm_s2) * TIME_STEP;
    present.vy = previous.vy + (present.ay * G_mm_s2) * TIME_STEP;
    present.vz = previous.vz + (present.az * G_mm_s2) * TIME_STEP;

    previous = present;

    SEND(ALL, sprintf(_xs,
                      "\r\nax: %+.3fg, ay: %+.3fg, az: %+.3fg, |a|: %.3fg  vx: %+.3fmm/s,  "
                      "vy:%+.3fmm/s, vz: %+.3fmm/s   x: %+.3fmm, y: %+.3fmm, z: %+.3fmm,   ",
                      present.ax, present.ay, present.az, vector_magnitude, present.vx, present.vy, present.vz, present.x, present.y,
                      present.z);)

    /*
     * Point to the next sample space
     */
    next_sample = (next_sample + 1) % SAMPLE_DEPTH;
  }

  /*
   * All done
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_oscilliscope()
 *
 * @brief:    Create a real time oscilliscope for the accelerometer
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Poll the ADXL345 and print out the acceleration data
 *
 *--------------------------------------------------------------*/
void ADXL345_oscilliscope(void)
{
  static unsigned int next_sample = 0;  // Index to the raw acceleration data
  real_t              vector_magnitude; // Magnitude of the acceleration vector
  bool                pause = false;

  while ( 1 )
  {
    if ( pause == false )
    {
      while ( ADXL345_read_raw_accel(&samples[0], false) != 0 )
      {
        ADXL345_convert_to_g(&samples[0], &present);                               // Convert raw data to g

        vector_magnitude = sqrt(SQ(present.ax) + SQ(present.ay) + SQ(present.az)); // Calculate the magnitude of the acceleration vector

        //        SEND(ALL, sprintf(_xs, "\r\nFIFO: %d, rx: %d,  ax: %+.3fg, ry: %d  ay: %+.3fg, rz: %d, az: %+.3fg, |a|: %.3fg",
        //        FIFO_samples,
        //                          samples[next_sample].x, present.ax, samples[next_sample].y, present.ay, samples[next_sample].z,
        //                          present.az, vector_magnitude);)
      }
    }

    if ( serial_available(ALL) != 0 )
    {
      char ch = serial_getch(ALL);
      if ( ch == '!' )                  // Exit the test
      {
        break;
      }
      if ( (ch == 'P') || (ch == 'p') ) // Reset the test
      {
        pause = !pause;
      }
    }
  }

  /*
   * All done
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
