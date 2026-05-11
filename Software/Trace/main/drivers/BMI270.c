/******************************************************************************
 *
 * file: BMI270.c
 *
 * Analog Devices BMI270 3-axis accelerometer driver
 *
 *****************************************************************************
 *
 * This file contains the driver for the BMI270 3-axis accelerometer.  The
 * driver is written to be as generic as possible and should work with any
 * implementation of the BMI270.
 *
 * See: https://www.analog.com/en/products/BMI270.html
 *      https://www.analog.com/media/en/technical-documentation/data-sheets/BMI270.pdf
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
#include "string.h"
#include "driver\gpio.h"
#include "i2c.h"
#include "math.h"
#include "assert.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "trace.h"
#include "board_assembly.h"
#include "diag_tools.h"
#include "gpio.h"
#include "json.h"
#include "helpers.h"
#include "BMI270.h"
#include "spi.h"

/*
 * Definitions
 */

#define g2      0       // Select +/- 2g range
#define g2_lsb  0.0039f // LSB value for +/- 2g range (4 mg/LSB)
#define g4      1       // Select +/- 4g range
#define g4_lsb  0.0078f // LSB value for +/- 4g range (8 mg/LSB)
#define g8      2       // Select +/- 8g range
#define g8_lsb  0.0156f // LSB value for +/- 8g range (16 mg/LSB)
#define g16     3       // Select +/- 16g range
#define g16_lsb 0.0312f // LSB value for +/- 16g range (31.2 mg/LSB)

#define SQ(x) ((x) * (x))

#define CHIP_ID 0x00

/*
 *  Typedefs
 */

/*
 * Function Prototypes
 */

/*
 * Variables
 */
trace_raw_t         BMI270_zero_sample; // Sample to hold the zeroed acceleration data
spi_device_handle_t BMI270_handle;      // Handle for the SPI device

spi_device_interface_config_t BMI270_config = {
    // Configuration for the SPI device
    .command_bits     = 0,                   // No command phase
    .address_bits     = 8,                   //
    .dummy_bits       = 8,                   // No dummy bits
    .mode             = 0,                   // SPI mode 0
    .clock_source     = SPI_CLK_SRC_DEFAULT, // Use default clock source
    .duty_cycle_pos   = 128,                 // 50% duty cycle
    .cs_ena_pretrans  = 0,                   // No pre-transaction CS activation
    .cs_ena_posttrans = 0,                   // No post-transaction CS activation
    .clock_speed_hz   = 400000,              // 400 kHz clock speed
    .input_delay_ns   = 0,                   // No input delay
    .spics_io_num     = BMI270_CS,           // CS pin
    .flags            = 0,                   // No special flags
    .queue_size       = 1,
    .pre_cb           = NULL,                // Callback to be called before a transmission is started.
    .post_cb          = NULL                 // Callback to be called after a transmission has completed.
};

/*----------------------------------------------------------------
 *
 * @function: BMI270_init()
 *
 * @brief:    Initalize the BMI270
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Setup the accelerometer from the table
 *
 *--------------------------------------------------------------*/
void BMI270_init(unsigned int BMI270_gpio)
{
  esp_err_t         ret;
  spi_transaction_t transaction;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_init()");))

  BMI270_config.spics_io_num = BMI270_gpio;

  /*
   * Add the accelerometer to the bus
   */
  if ( spi_bus_add_device(SPI2_HOST, &BMI270_config, &BMI270_handle) != ESP_OK ) // Add the SPI device to the bus
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to add BMI270 device to SPI bus");))
  }

  /*
   * Read the device ID
   */
  memset(&transaction, 0, sizeof(transaction));           // Clear the transaction structure
  transaction.addr      = 0x80 | CHIP_ID;                 // Register address to read from
  transaction.length    = 8;                              // Transmit length in bits
  transaction.tx_buffer = NULL;                           // Transmit buffer not used
  transaction.rxlength  = 1 * 8;                          // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;           // Indicate that this is a read operation

  ret = spi_device_transmit(BMI270_handle, &transaction); // Dummy read to put into SPI mode
  ret = spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Device ID: 0x%02X 0x%02X", transaction.rx_data[0], transaction.rx_data[1]);))

  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_FIFO_read
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
void BMI270_FIFO_read(void)
{
  run_state |= IN_COLLECTION;

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "BMI270_FIFO_read()");))

  run_state &= ~IN_COLLECTION;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_read_accel()
 *
 * @brief:    Read acceleration data from the BMI270
 *
 * @return:   Number of bytes remaining in the FIFO
 *
 *----------------------------------------------------------------
 *
 * The Acceleration data is read from the BMI270 in 6 bytes,
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
unsigned int BMI270_read_raw_accel(trace_raw_t *sample,     // Where to save results
                                   bool         zero_offset // TRUE if a zero offset it to be applied
)
{
  int samples_remaining;                                    // Number of samples remaining in the FIFO

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
    sample->x -= BMI270_zero_sample.x;            // Add the zero offset to the raw data
    sample->y -= BMI270_zero_sample.y;            // to remove the DC bias for a level sensor
    sample->z -= BMI270_zero_sample.z;
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
 * @function: BMI270_zero()
 *
 * @brief:    Determine the resting g levels for the BMI270
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

void BMI270_find_zero(void)
{
  unsigned int i;          // Loop counter
  trace_raw_t  BMI270_raw; // As read from the accelerometer
  trace_raw_t  BMI270_sum;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_find_zero()");))

  /*
   * Loop and collect samples
   */

  BMI270_sum.x = 0; // Zero out the sum
  BMI270_sum.y = 0;
  BMI270_sum.z = 0;

  sample_out = BMI270_find_sample_out(NUM_ZERO_SAMPLES);
  i          = 0;
  while ( i != NUM_ZERO_SAMPLES )
  {
    BMI270_read_raw_accel(&BMI270_raw, false); // Take a sample of the raw acceleration data
    BMI270_sum.x += BMI270_raw.x;              // Accumulate the X-axis raw acceleration data
    BMI270_sum.y += BMI270_raw.y;              // Accumulate the Y-axis raw acceleration data
    BMI270_sum.z += BMI270_raw.z;              // Accumulate the Z-axis raw acceleration data
    i++;
    printf("Sample %d: X: %04X, Y: %04X, Z: %04X\r\n", i, BMI270_raw.x, BMI270_raw.y, BMI270_raw.z);
  }

  /*
   * Average the samples to get a more accurate zero level
   */
  BMI270_zero_sample.x = BMI270_sum.x / NUM_ZERO_SAMPLES; // Average the X-axis raw acceleration data
  BMI270_zero_sample.y = BMI270_sum.y / NUM_ZERO_SAMPLES; // Average the Y-axis raw acceleration data
  BMI270_zero_sample.z = BMI270_sum.z / NUM_ZERO_SAMPLES; // Average the Z-axis raw acceleration data

  /*
   *  All done, return
   */
  DLT(DLT_INFO,
      SEND(ALL, sprintf(_xs, "Axis offset - X: %04X, Y: %04X, Z: %04X", BMI270_zero_sample.x, BMI270_zero_sample.y, BMI270_zero_sample.z);))

  set_status_LED(LED_READY); // Indicate that we are ready
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_convert_to_g()
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
void BMI270_convert_to_g(trace_raw_t *sample, trace_point_t *actual)
{
  real_t lsb_per_g = 1.0;               //  BMI270_lsb_per_g[DATA_FORMAT & 0b00000011]; // Get the LSB per g for the current range setting

  actual->ax = (sample->x) * lsb_per_g; // Convert raw X-axis data to g
  if ( F_ABS(actual->x) < 0.010 )
  {
    actual->ax = 0;
  }

  actual->ay = (sample->y) * lsb_per_g; // Convert raw Y-axis data to g
  if ( F_ABS(actual->ay) < 0.010 )
  {
    actual->ay = 0;
  }

  actual->az = (sample->z) * lsb_per_g; // Convert raw Z-axis data to g
  if ( F_ABS(actual->az) < 0.010 )
  {
    actual->az = 0;
  }
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_test()
 *
 * @brief:    Test the BMI270
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Poll the BMI270 and print out the acceleration data
 *
 *--------------------------------------------------------------*/
#define TIME_STEP    (0.010)
#define TIME_STEP_SQ (TIME_STEP * TIME_STEP)
#define G_mm_s2      9806.65

void BMI270_test(void)
{
  real_t        vector_magnitude;                          // Magnitude of the acceleration vector
  trace_raw_t   previous_raw;                              // Previous sample
  trace_raw_t   present_raw;                               // Present sample
  trace_point_t previous;                                  // Previous sample converted to g
  trace_point_t present;                                   // Present sample converted to g

  BMI270_find_sample_out(NUM_ZERO_SAMPLES);                // Start at the point where we took the zero samples to get the most recent data

  BMI270_read_raw_accel(&previous_raw, true);

  while ( BMI270_read_raw_accel(&present_raw, true) != 0 ) // Read the acceleration dataP )
  {
    BMI270_convert_to_g(&present_raw, &present);           // Convert raw data to g

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
  }

  /*
   * All done
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_oscilliscope()
 *
 * @brief:    Create a real time oscilliscope for the accelerometer
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Poll the BMI270 and print out the acceleration data
 *
 *--------------------------------------------------------------*/
void BMI270_oscilliscope(void)
{
  static unsigned int next_sample = 0;  // Index to the raw acceleration data
  real_t              vector_magnitude; // Magnitude of the acceleration vector
  bool                pause = false;

  while ( 1 )
  {
    if ( pause == false )
    {
      while ( BMI270_read_raw_accel(&samples[0], false) != 0 )
      {
        BMI270_convert_to_g(&samples[0], &present);                                // Convert raw data to g

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

/*----------------------------------------------------------------
 *
 * @function: BMI270_find_sample_in()
 *
 * @brief:    Find the index of a sample based on a number of samples back from the current sample
 *
 * @return:   index of the sample in the sample buffer
 *
 *----------------------------------------------------------------
 *
 * Samples are taken continiously so the pointer sample_out
 * may correspond to some point in time far back in the past.
 *
 * This function works out sample_out relative to sample_in
 * to find a find a starting point correspondin to an interval.
 *
 *--------------------------------------------------------------*/
unsigned int BMI270_find_sample_out(unsigned int sample_count) // Number of samples to look back
{
  sample_out = sample_in - sample_count;                       // Calculate the index of the sample that corresponds to the desired duration
  if ( sample_out < 0 )                                        // If the index is negative, adjust for wrap-around
  {
    sample_out += SAMPLE_DEPTH;                                // Adjust for wrap-around if the index is negative
  }

  return sample_out;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_device_id()
 *
 * @brief:    Read the device ID from the BMI270
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Samples are taken continiously so the pointer sample_out
 * may correspond to some point in time far back in the past.
 *
 * This function works out sample_out relative to sample_in
 * to find a find a starting point correspondin to an interval.
 *
 *--------------------------------------------------------------*/
void BMI270_device_id(void)
{
  esp_err_t         ret;
  spi_transaction_t transaction;
  /*
   * Read the device ID
   */

  while ( 1 )
  {
    memset(&transaction, 0, sizeof(transaction));           // Clear the transaction structure
    transaction.addr     = 0x80 | CHIP_ID;                         // Register address to read from
    transaction.length   = 1 * 8;                           // Transmit length in bits
    transaction.rxlength = 1 * 8;                           // Receive length in bits
    transaction.flags    = SPI_TRANS_USE_RXDATA;            // Indicate that this is a read operation

    ret = spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Device ID: 0x%02X 0x%02X", transaction.rx_data[0], transaction.rx_data[1]);))
    vTaskDelay(100);

    if ( check_for_exit() == '!' )                          // Check for an exit command on the serial port
    {
      break;
    }
  }

  /*
   * All done
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
