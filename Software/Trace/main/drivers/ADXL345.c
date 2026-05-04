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
 *****************************************************************************/
#include "stdio.h"
#include "driver\gpio.h"
#include "i2c.h"
#include "math.h"

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
#define ADXL345_ADDR 0x53   // I2C address of the ADXL345

#define BW_RATE_1600HZ 0x0F // BW_RATE register value for 1600 Hz data rate
#define BW_RATE_800HZ  0x0E // BW_RATE register value for 800 Hz data rate
#define BW_RATE_400HZ  0x0D // BW_RATE register value for 400 Hz data rate
#define BW_RATE_200HZ  0x0C // BW_RATE register value for 200 Hz data rate
#define BW_RATE_100HZ  0x0A // BW_RATE register value for 100 Hz data rate
#define BW_RATE_50HZ   0x09 // BW_RATE register value for 50 Hz data rate
#define BW_RATE_25HZ   0x08 // BW_RATE register value for 25 Hz data rate
#define BW_RATE_12_5HZ 0x07 // BW_RATE register value for 12.5 Hz data rate
#define BW_RATE_6_25HZ 0x06 // BW_RATE register value for 6.25 Hz data rate
#define BW_RATE_3_13HZ 0x05 // BW_RATE register value for 3.13 Hz data rate
#define BW_RATE_1_56HZ 0x04 // BW_RATE register value for 1.56 Hz data rate
#define BW_RATE_0_78HZ 0x03 // BW_RATE register value for 0.78 Hz data rate
#define BW_RATE_0_39HZ 0x02 // BW_RATE register value for 0.39 Hz data rate
#define BW_RATE_0_20HZ 0x01 // BW_RATE register value for 0.20 Hz data rate

#if ( SAMPLE_RATE == 1600 )
#define ACC_SAMPLE_RATE BW_RATE_1600HZ
#endif
#if ( SAMPLE_RATE == 800 )
#define ACC_SAMPLE_RATE BW_RATE_800HZ
#endif
#if ( SAMPLE_RATE == 400 )
#define ACC_SAMPLE_RATE BW_RATE_400HZ
#endif
#if ( SAMPLE_RATE == 200 )
#define ACC_SAMPLE_RATE BW_RATE_200HZ
#endif

#if ( SAMPLE_RATE == 100 )
#define ACC_SAMPLE_RATE BW_RATE_100HZ
#endif
#if ( SAMPLE_RATE == 50 )
#define ACC_SAMPLE_RATE BW_RATE_50HZ
#endif
#if ( SAMPLE_RATE == 25 )
#define ACC_SAMPLE_RATE BW_RATE_25HZ
#endif
#if ( SAMPLE_RATE == 12 )
#define ACC_SAMPLE_RATE BW_RATE_12HZ
#endif

#if ( SAMPLE_RATE == 615 )
#define ACC_SAMPLE_RATE BW_RATE_6_15HZ
#endif
#if ( SAMPLE_RATE == 313 )
#define ACC_SAMPLE_RATE BW_RATE_3_13HZ
#endif
#if ( SAMPLE_RATE == 156 )
#define ACC_SAMPLE_RATE BW_RATE_1_56HZ
#endif
#if ( SAMPLE_RATE == 78 )
#define ACC_SAMPLE_RATE BW_RATE_0_78HZ
#endif

#if ( SAMPLE_RATE == 39 )
#define ACC_SAMPLE_RATE BW_RATE_0_39HZ
#endif

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
    {0x2C, ACC_SAMPLE_RATE  }, // BW_RATE register, 100 Hz data rate
    {0x2D, POWER_CTL_MEASURE}, // Power control register, Measure mode
    {0x2E, INT_MAP_NONE     }, // Interrupt map register, Map all interrupts to INT1 pin
    {0x2F, INT_SOURCE_CLEAR }, // Interrupt source register, Clear all interrupts
    {0x31, DATA_FORMAT      }, // Data format register, Full resolution, right justified, +/- 2g range
    {0x38, 0b10000000 + 0x0F}, // FIFO mode, Stream, no triggger, interrupt on 16 samples
    {0,    0                }  // End of the list
};

real_t         ADXL345_lsb_per_g[] = {g2_lsb, g4_lsb, g8_lsb, g16_lsb}; // LSB per g for each range setting
accel_sample_t ADXL345_zero_sample;                                     // Sample to hold the zeroed acceleration data

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
    samples[i].vx = 0;
    samples[i].vy = 0;
    samples[i].vz = 0;
  }

  ADXL345_zero_sample.raw_x = 0;
  ADXL345_zero_sample.raw_y = 0;
  ADXL345_zero_sample.raw_z = 0;

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
 *
 *--------------------------------------------------------------*/
unsigned int ADXL345_read_raw_accel(accel_sample_t *sample,     // Where to save results
                                    bool            zero_offset // TRUE if a zero offset it to be applied
)
{
  unsigned char data[8];

  data[0] = 0x32;                                               // Register address for the X-axis acceleration data
  i2c_write(ADXL345_ADDR, data, 1);                             // Write the register address to the device
  i2c_read(ADXL345_ADDR, data, sizeof(data));                   // Read 6 bytes of acceleration data (X, Y, Z)

  sample->raw_x = (data[1] << 8) | data[0];                     // Combine low and high bytes for X-axis
  sample->raw_y = (data[3] << 8) | data[2];                     // Combine low and high bytes for Y-axis
  sample->raw_z = (data[5] << 8) | data[4];                     // Combine low and high bytes for Z-axis

  if ( zero_offset == true )
  {
    sample->raw_x -= ADXL345_zero_sample.raw_x;                 // Combine low and high bytes for X-axis
    sample->raw_y -= ADXL345_zero_sample.raw_y;                 // Combine low and high bytes for Y-axis
    sample->raw_z -= ADXL345_zero_sample.raw_z;                 // Combine low and high bytes for Z-axi
  }

  /*
   *  Return the number of samples remaining
   */
  return data[7] & 0x1F; // Return FIFO samples remaining
}

/*----------------------------------------------------------------
 *
 * @function: ADXL345_read_FIFO_accel()
 *
 * @brief:    Pull in all of the data waiting in the FIFO
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
 * The func
 *
 *--------------------------------------------------------------*/
unsigned int ADXL345_read_FIFO_accel(void)
{
  static unsigned int next_sample = 0; //  Index to next sample

  unsigned char data[1];
  unsigned int  FIFO_available;        // How many bytes are in the FIFO?
  unsigned int  FIFO_read;             // How many samples are read?
  data[0] = 0x38;                      // FIFO register
  i2c_write(ADXL345_ADDR, data, 1);    // Write the register address to the device
  i2c_read(ADXL345_ADDR, data, 1);     // Read 6 bytes of acceleration data (X, Y, Z)

  FIFO_available = data[0] & 0x1F;     // Up to  32 samples may be waiting
  FIFO_read      = FIFO_available;

  while ( FIFO_available != 0 )
  {
    FIFO_available = ADXL345_read_raw_accel(&samples[next_sample], true);
    next_sample    = (next_sample + 1) % SAMPLE_DEPTH;
  }

  return FIFO_read; // Return number of FIFO samples read
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

accel_sample_t zero_samples;  // Buffer to hold multiple samples for averaging

void ADXL345_find_zero(void)
{
  int            i;
  accel_sample_t ADXL345_raw; // As read from the accelerometer
  accel_sample_t ADXL345_sum;
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "ADXL345_find_zero()");))

  /*
   * Loop and collect samples
   */

  ADXL345_sum.raw_x = 0;                         // Zero out the sum
  ADXL345_sum.raw_y = 0;
  ADXL345_sum.raw_z = 0;

  for ( i = 0; i != NUM_ZERO_SAMPLES; i++ )
  {
    gpio_set_level(STATUS_LED, i & 8);           // Blinik the LED to indicate that we are taking samples
    ADXL345_read_raw_accel(&ADXL345_raw, false); // Take a sample of the raw acceleration data

    ADXL345_sum.raw_x += ADXL345_raw.raw_x;      // Accumulate the X-axis raw acceleration data
    ADXL345_sum.raw_y += ADXL345_raw.raw_y;      // Accumulate the Y-axis raw acceleration data
    ADXL345_sum.raw_z += ADXL345_raw.raw_z;      // Accumulate the Z-axis raw acceleration data

    vTaskDelay(TICK_10ms);                       // Delay between samples
  }

  /*
   * Average the samples to get a more accurate zero level
   */
  ADXL345_zero_sample.raw_x = ADXL345_sum.raw_x / NUM_ZERO_SAMPLES; // Average the X-axis raw acceleration data
  ADXL345_zero_sample.raw_y = ADXL345_sum.raw_y / NUM_ZERO_SAMPLES; // Average the Y-axis raw acceleration data
  ADXL345_zero_sample.raw_z = ADXL345_sum.raw_z / NUM_ZERO_SAMPLES; // Average the Z-axis raw acceleration data

  /*
   *  All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Axis offset - X: %04X, Y: %04X, Z: %04X", ADXL345_zero_sample.raw_x, ADXL345_zero_sample.raw_y,
                                  ADXL345_zero_sample.raw_z);))

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
void ADXL345_convert_to_g(accel_sample_t *sample)
{
  real_t lsb_per_g = ADXL345_lsb_per_g[DATA_FORMAT & 0b00000011]; // Get the LSB per g for the current range setting

  sample->ax = (sample->raw_x) * lsb_per_g;                       // Convert raw X-axis data to g
  if ( F_ABS(sample->ax) < 0.010 )
  {
    sample->ax = 0;
  }

  sample->ay = (sample->raw_y) * lsb_per_g;                       // Convert raw Y-axis data to g
  if ( F_ABS(sample->ay) < 0.010 )
  {
    sample->ay = 0;
  }

  sample->az = (sample->raw_z) * lsb_per_g;                       // Convert raw Z-axis data to g
  if ( F_ABS(sample->az) < 0.010 )
  {
    sample->az = 0;
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

#define G_mm_s2 9806.65f                // Acceleration due to gravity in mm/s^2

void ADXL345_test(void)
{
  static unsigned int next_sample = 0;  // Index to the raw acceleration data
  static unsigned int last_sample = 0;  //  Index to the last input
  real_t              vector_magnitude; // Magnitude of the acceleration vector
  unsigned int        FIFO_samples;     // Number of samples in FIFO
  bool                pause = false;

  while ( 1 )
  {
    if ( pause == false )
    {
      FIFO_samples = ADXL345_read_raw_accel(&samples[next_sample], true); // Read the acceleration dataP
      ADXL345_convert_to_g(&samples[next_sample]);                        // Convert raw data to g

      vector_magnitude = sqrt(SQ(samples[next_sample].ax) + SQ(samples[next_sample].ay) +
                              SQ(samples[next_sample].az));               // Calculate the magnitude of the acceleration vector

      /*
       * Integrate the positition
       */
      samples[next_sample].x =
          (samples[last_sample].x) + (samples[last_sample].vx * TIME_STEP) +
          (samples[next_sample].ax * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the X position using the acceleration data
      samples[next_sample].y =
          (samples[last_sample].y) + (samples[last_sample].vy * TIME_STEP) +
          (samples[next_sample].ay * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Y position using the acceleration data
      samples[next_sample].z =
          (samples[last_sample].z) + (samples[last_sample].vz * TIME_STEP) +
          (samples[next_sample].az * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Z position using the acceleration data
      /*
       * Integrate the velocity
       */
      samples[next_sample].vx = samples[last_sample].vx + (samples[next_sample].ax * G_mm_s2) * TIME_STEP;
      samples[next_sample].vy = samples[last_sample].vy + (samples[next_sample].ay * G_mm_s2) * TIME_STEP;
      samples[next_sample].vz = samples[last_sample].vz + (samples[next_sample].az * G_mm_s2) * TIME_STEP;

      SEND(ALL, sprintf(_xs,
                        "\r\nFIFO: %d, ax: %+.3fg, ay: %+.3fg, az: %+.3fg, |a|: %.3fg  vx: %+.3fmm/s,  "
                        "vy:%+.3fmm/s, vz: %+.3fmm/s   x: %+.3fmm, y: %+.3fmm, z: %+.3fmm,   ",
                        FIFO_samples, samples[next_sample].ax, samples[next_sample].ay, samples[next_sample].az, vector_magnitude,
                        samples[last_sample].vx, samples[last_sample].vy, samples[last_sample].vz, samples[next_sample].x,
                        samples[next_sample].y, samples[next_sample].z);)

      /*
       * Point to the next sample space
       */
      last_sample = next_sample;
      next_sample = (next_sample + 1) % SAMPLE_DEPTH;
    }

    vTaskDelay( TICK_10ms);

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
  unsigned int        FIFO_samples;     // Number of samples in FIFO
  bool                pause = false;

  while ( 1 )
  {
    if ( pause == false )
    {
      FIFO_samples = ADXL345_read_raw_accel(&samples[0], true);            // Read the acceleration data
      ADXL345_convert_to_g(&samples[0]);                                   // Convert raw data to g

      vector_magnitude =
          sqrt(SQ(samples[0].ax) + SQ(samples[0].ay) + SQ(samples[0].az)); // Calculate the magnitude of the acceleration vector

      SEND(ALL, sprintf(_xs, "\r\nFIFO: %d, ax: %+.3fg, ay: %+.3fg, az: %+.3fg, |a|: %.3fg", FIFO_samples, samples[next_sample].ax,
                        samples[next_sample].ay, samples[next_sample].az, vector_magnitude);)
    }

    vTaskDelay(10 * TICK_10ms);

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
