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
 * See:
 *
 * https://www.bosch-sensortec.com/en/products/motion-sensors/imus/bmi270
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/application_notes_1/bst-bmi270-an001.pdf
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
#include "BMI270_define.h"
#include "spi.h"

/*
 * Definitions
 */

#define SQ(x) ((x) * (x))

unsigned int FIFO_raw_in  = 0;
unsigned int FIFO_raw_out = 0;
#define FIFO_RAW_SAMPLE 10
FIFO_raw_read_t sample_raw_read[FIFO_RAW_SAMPLE]; // Allow for 10 reads
/*----------------------------------------------------------------
 *
 * @function: BMI270_init()
 *
 * @brief:    Initaliz              e the BMI270
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
  spi_transaction_t transaction;
  int               i;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_init()");))
  PAUSE("Ready")

  BMI270_spi_config.spics_io_num = BMI270_gpio;

  /*
   * Add the accelerometer to the bus
   */
  if ( spi_bus_add_device(SPI2_HOST, &BMI270_spi_config, &BMI270_handle) != ESP_OK ) // Add the SPI device to the bus
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to add BMI270 device to SPI bus");))
  }

                                                                                     /*
                                                                                      * Read the device ID
                                                                                      */

  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | CHIP_ID;           // Register address to read from
  transaction.length    = 2 * 8;                    // Transmit length in bits
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.rxlength  = 2 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation

  spi_device_transmit(BMI270_handle, &transaction); // Dummy read to put into SPI mode
  PAUSE("CHIP_ID")
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[1] != 0x24 )             // Check the device ID
  {
    DLT(DLT_FATAL, SEND(ALL, sprintf(_xs, "Failed to read BMI270 device ID: 0x%02X", transaction.rx_data[1]);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270 device ID: 0x%02X", transaction.rx_data[1]);))
  }
  vTaskDelay(1);                                    /*
                                                     * Reset the device
                                                     */
#if ( 0 )                                           // Do not enable
  PAUSE("Sending CMD- -0xB6")
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = CMD;                      // Disable the accelerometer before programming the API
  transaction.tx_buffer = 0xB6;                     // Soft Reset
  transaction.length    = 8;                        // Transmit length in bits
  transaction.rxlength  = 0;                        // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;     // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
  vTaskDelay(1);
#endif

  /*
   * Programming the API
   */
  PAUSE("Sending PWR_CONF")
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = PWR_CONF;                                    // Disable the accelerometer before programming the API
  transaction.tx_buffer = 0;                                           // Disable the
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction
  vTaskDelay(1);                                                       // Wait 450us

  PAUSE("sending INIT_CONTROL = 0")
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_CTRL;                                   // Prepare the configuration file for the API programming
  transaction.tx_buffer = 0;                                           // Transmit buffer
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction
  vTaskDelay(1);

  PAUSE("sending INIT_DATA")
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_DATA;                                   // Prepare the configuration file for the API programming
  transaction.tx_buffer = &bmi270_maximum_fifo_config_file;            // Transmit the configuration file for the API programming
  transaction.length    = sizeof(bmi270_maximum_fifo_config_file) * 8; // Transmit length in bits
  transaction.rxlength  = 0;                                           // Indicate that this is a read operation
  transaction.flags     = 0;                                           // Use default flags for a large write operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction
  vTaskDelay(1);

  PAUSE("Sending INIT_CONTROL = 1")
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_CTRL;                                   // Start the processor going
  transaction.tx_buffer = 1;                                           // Complete the API programming
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction
  vTaskDelay(1);

  /*
   *  Make sure the API is properly initialized
   */
  PAUSE("Verifying initialization")
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | INTERNAL_STATUS;   // Read the internal status register to check if the API is properly initialized
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.length    = 2 * 8;                    // Transmit length in bits
  transaction.rxlength  = 2 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
  if ( transaction.rx_data[1] != 0x1 )              // Check the device ID
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initialization failed: 0x%02X", transaction.rx_data[1]);))
  }
  vTaskDelay(1);

  /*
   * Program the registers from the configuration table
   */

  i = 0;
  while ( BMI270_config[i].address != 0x00 )          // Loop through the configuration table until the end is reached
  {
    memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
    transaction.addr      = BMI270_config[i].address; // Register address to write to
    transaction.tx_buffer = BMI270_config[i].value;   // Send the value to be written to the register
    transaction.length    = 1 * 8;                    // Transmit length in bits
    transaction.rxlength  = 0 * 8;                    // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_TXDATA;     // Indicate that this is a read operation
    PAUSE("Sending register value");
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "register 0x%02X: 0x%02X", BMI270_config[i].address, BMI270_config[i].value);))
    spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
    vTaskDelay(2);
    i++;
  }

  /*
   * All done, return
   */
  PAUSE("Finished")
  BMI270_SPI_dump();
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270 initialization successful");))
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_pull_FIFO
 *
 * @brief:    Pull all of the samples out of the FIFO and store them in the sample buffer
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function is called if the FIFO watermark is reached.
 *
 * The function determines how much there is in the FIFO and
 * reads out the contents until there
 *
 *---------------------------------------------------------------*/
void BMI270_FIFO_test(void)
{
  BMI270_pull_FIFO();
  printf(_DONE_);
}
unsigned int BMI270_pull_FIFO(void)
{
  spi_transaction_t transaction;
  int               FIFO_length; // How many samples are waiting?

  run_state |= IN_COLLECTION;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_FIFO_read()");))

  while ( 1 )
  {
    /*
     *  Read the FIFO length
     */
    memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
    transaction.addr      = 0x80 | FIFO_LENGTH;       // How many readings are waiting for uys
    transaction.tx_buffer = 0;                        // Transmit buffer not used
    transaction.length    = 4 * 8;                    // Transmit length in bits
    transaction.rxlength  = 3 * 8;                    // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation
    spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

    FIFO_length = (transaction.rx_data[2] << 8) + transaction.rx_data[1];
    printf("%d  ", FIFO_length);
    if ( FIFO_length <= FIFO_READ )
    {
      printf(_DONE_);
      break;
    }

    /*
     *  Read in the next bunch of samples
     */
    memset(&transaction, 0, sizeof(transaction));                // Clear the transaction structure{"TEST":24}
    transaction.addr      = 0x80 | FIFO_DATA;                    // Point to the FIFO read regisetrt
    transaction.tx_buffer = &sample_raw_read[FIFO_raw_in].dummy; // Transmit buffer not used
    transaction.length    = sizeof(FIFO_raw_read_t) * 8;         // Transmit length in bits
    transaction.rxlength  = sizeof(FIFO_raw_read_t) * 8;         // Receive length in bits
    transaction.flags     = 0;                                   // Indicate that this is a read operation
    spi_device_transmit(BMI270_handle, &transaction);

    FIFO_raw_in = (FIFO_raw_in + 1) % FIFO_RAW_SAMPLE;           // Point to the next buffer

    if ( FIFO_raw_in == FIFO_raw_out )                           // Wrapped around and collided with the output
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Finished buffer");))
      break;
    }
  }

  run_state &= ~IN_COLLECTION;

  return FIFO_length;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_get_next_sample
 *
 * @brief:    Pull the next sample out of the circular queue
 *
 * @return:   TRUE if a valid sample has been found.
 *----------------------------------------------------------------
 *
 * The raw data is pulled out of the BMI270 FIFO in fixed chunks
 * each of these chunk is stored in an array.
 *
 * This function keeps track of which cunk is currently in use and
 * where in the chunk the next sample lives.
 *
 * The sample is removed from the array and the pointers updated
 * to point to the next available value;
 *---------------------------------------------------------------*/
bool BMI270_get_next_raw_sample(raw_frame_t *sample)
{
  static unsigned int inner_index = 0;

  /*
   * Check to see if we have wrapped around
   */
  if ( (inner_index == 0)                 // Starting a new chunk
       && (FIFO_raw_out == FIFO_raw_in) ) // Wrapped around and no new data
  {
    return false;                         // This sample is not valid
  }

                                          /*
                                           * Pull the next sample out of the chunk
                                           */
  *sample = sample_raw_read[FIFO_raw_out].f[inner_index];

  inner_index = (inner_index + 1) % FIFO_READ; // Next sample

  if ( inner_index == 0 )                      // Next chunk
  {
    FIFO_raw_out = (FIFO_raw_out + 1) % FIFO_RAW_SAMPLE;
  }

  /*
   * All done, show we have valid data
   */
  return true;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_read_raw_accel()
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
 * IMPORTANT
 *
 * The BMI270 brings the acceleration in as LSB and MSB.
 * ie, the bytes need to be swapped before use.
 *
 *--------------------------------------------------------------*/
void BMI270_read_raw_accel(trace_raw_t *sample_as_read) // TRUE if a zero offset it to be applied
{
  esp_err_t         ret;
  spi_transaction_t transaction;
  trace_raw_t       filler;

  memset(&filler, 0xff, sizeof(trace_raw_t));           // Clear the sample structure
  memset(&transaction, 0x00, sizeof(transaction));      // Clear the transaction structure
  transaction.addr      = 0x80 | ACCEL_X; // Start at Accel Acceleration Data Low register and read all 6 bytes in one transaction
  transaction.length    = (sizeof(trace_raw_t) + 1) * 8;  // Transmit length in bits
  transaction.tx_buffer = &filler;                        // Send dummy data to read the acceleration data
  transaction.rxlength  = sizeof(trace_raw_t) * 8;        //
  transaction.rx_buffer = &sample_as_read->dummy;         // Receive buffer to store the raw acceleration data
  transaction.flags     = 0;

  ret = spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
  if ( ret != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to read sensor");))
  }

  DLT(DLT_DEBUG,
      SEND(ALL, sprintf(_xs, "\r\n %d %p %p x_dot: %04X   y_dor: %04X   z_dot: %04X  ", sizeof(trace_raw_t), &sample_as_read->dummy,
                        &sample_as_read->f.x_dotdot, sample_as_read->f.x_dotdot, sample_as_read->f.y_dotdot, sample_as_read->f.z_dotdot);))

  return; //}
#if ( 0 )
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
#endif
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_find_zero()
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
  int32_t      x_dotdot;   // 32 bit acceleration to prevent overflow
  int32_t      y_dotdot;   // while summing
  int32_t      z_dotdot;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_find_zero()");))

  /*
   * Loop and collect samples
   */

  x_dotdot = 0; // Zero out the sum
  y_dotdot = 0;
  z_dotdot = 0;

                // sample_out = BMI270_find_sample_out(NUM_ZERO_SAMPLES);
  i = 0;
  while ( i != NUM_ZERO_SAMPLES )
  {
    BMI270_read_raw_accel(&BMI270_raw); // Take a sample of the raw acceleration data
    x_dotdot += BMI270_raw.f.x_dotdot;  // Accumulate the X-axis raw acceleration data
    y_dotdot += BMI270_raw.f.y_dotdot;  // Accumulate the Y-axis raw acceleration data
    z_dotdot += BMI270_raw.f.z_dotdot;  // Accumulate the Z-axis raw acceleration data
    i++;
  }

  /*
   * Average the samples to get a more accurate zero level
   */
  BMI270_zero_sample.f.x_dotdot = x_dotdot / NUM_ZERO_SAMPLES; // Average the X-axis raw acceleration data
  BMI270_zero_sample.f.y_dotdot = y_dotdot / NUM_ZERO_SAMPLES; // Average the Y-axis raw acceleration data
  BMI270_zero_sample.f.z_dotdot = z_dotdot / NUM_ZERO_SAMPLES; // Average the Z-axis raw acceleration data

  /*
   *  All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Axis offset - X: %04X, Y: %04X, Z: %04X", BMI270_zero_sample.f.x_dotdot,
                                  BMI270_zero_sample.f.y_dotdot, BMI270_zero_sample.f.z_dotdot);))

  set_status_LED(LED_READY); // Indicate that we are ready
  SEND(ALL, sprintf(_xs, _DONE_);)

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
 * The fudge factor 2* is because of the scaling inside of the
 * register.
 *
 *--------------------------------------------------------------*/
#define ACCEL_DEAD_BAND 0.0
#define GYRO_DEAD_BAND  0.0
#define CAL_SCALE       1.0
void BMI270_convert_to_g(trace_raw_t *sample, trace_point_t *actual)
{
  actual->x_dotdot = CAL_SCALE * ((real_t)sample->f.x_dotdot) * G_PER_LSB; // Convert raw X-axis data to g
  if ( F_ABS(actual->x) < ACCEL_DEAD_BAND )
  {
    actual->x_dotdot = 0;
  }

  actual->y_dotdot = CAL_SCALE * ((real_t)sample->f.y_dotdot) * G_PER_LSB; // Convert raw Y-axis data to g
  if ( F_ABS(actual->y_dotdot) < ACCEL_DEAD_BAND )
  {
    actual->y_dotdot = 0;
  }

  actual->z_dotdot = CAL_SCALE * ((real_t)sample->f.z_dotdot) * G_PER_LSB; // Convert raw Z-axis data to g
  if ( F_ABS(actual->z_dotdot) < ACCEL_DEAD_BAND )
  {
    actual->z_dotdot = 0;
  }

  actual->rho_dot = ((real_t)sample->f.rho_dot) * GYRO_PER_LSB;            // Convert raw X-axis data to g
  if ( F_ABS(actual->rho_dot) < GYRO_DEAD_BAND )
  {
    actual->rho_dot = 0;
  }

  actual->theta_dot = ((real_t)sample->f.theta_dot) * GYRO_PER_LSB;        // Convert raw X-axis data to g
  if ( F_ABS(actual->theta_dot) < GYRO_DEAD_BAND )
  {
    actual->theta_dot = 0;
  }

  actual->phi_dot = ((real_t)sample->f.phi_dot) * GYRO_PER_LSB;            // Convert raw X-axis data to g
  if ( F_ABS(actual->phi_dot) < GYRO_DEAD_BAND )
  {
    actual->phi_dot = 0;
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

  real_t        vector_magnitude;           // Magnitude of the acceleration vector
  trace_raw_t   previous_raw;               // Previous sample
  trace_raw_t   present_raw;                // Present sample
  trace_point_t previous;                   // Previous sample converted to g
  trace_point_t present;                    // Present sample converted to g

  BMI270_find_sample_out(NUM_ZERO_SAMPLES); // Start at the point where we took the zero samples to get the most recent data

  BMI270_read_raw_accel(&previous_raw);

  while ( 1 )
  {

    BMI270_read_raw_accel(&present_raw);                                          // Read the acceleration dataP )

    BMI270_convert_to_g(&present_raw, &present);                                  // Convert raw data to g

    vector_magnitude =
        sqrt(SQ(present.x_dotdot) + SQ(present.y_dotdot) + SQ(present.z_dotdot)); // Calculate the magnitude of the acceleration vector

    /*
     * Integrate the positition
     */
    present.x = (previous.x) + (previous.x_dot * TIME_STEP) +
                (present.x_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the X position using the acceleration data
    present.y = (previous.y) + (previous.y_dot * TIME_STEP) +
                (present.y_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Y position using the acceleration data
    present.z = (previous.z) + (previous.z_dot * TIME_STEP) +
                (present.z_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Z position using the acceleration data
    /*{"TR}"}
     * Integrate the velocity
     */
    present.x_dot = previous.x_dot + (present.x_dotdot * G_mm_s2) * TIME_STEP;
    present.y_dot = previous.y_dot + (present.y_dotdot * G_mm_s2) * TIME_STEP;
    present.z_dot = previous.y_dot + (present.z_dotdot * G_mm_s2) * TIME_STEP;

    previous = present;

    SEND(ALL, sprintf(_xs,
                      "\r\nx..: %+.3fg, y..: %+.3fg, z..: %+.3fg, |a|: %.3fg  x.: %+.3fmm/s,  "
                      "y.:%+.3fmm/s, z.: %+.3fmm/s   x: %+.3fmm, y: %+.3fmm, z: %+.3fmm,   ",
                      present.x_dotdot, present.y_dotdot, present.z_dotdot, vector_magnitude, present.x_dot, present.y_dot, present.z_dot,
                      present.x, present.y, present.z);)
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
  trace_point_t       trace_value;
  trace_raw_t         sample;

  while ( 1 )
  {
    if ( pause == false )
    {
      BMI270_read_raw_accel(&sample);
      BMI270_convert_to_g(&sample, &trace_value);        // Convert raw data to g

      vector_magnitude = sqrt(SQ(trace_value.x_dotdot) + SQ(trace_value.y_dotdot) +
                              SQ(trace_value.z_dotdot)); // Calculate the magnitude of the acceleration

      SEND(ALL, sprintf(_xs, "\r\n\r|a|: %6.4f,   ax: %6.4f, ay: %6.4f,  az: %6.4f,    rho_dot: %6.4f, theta_dot: %6.4f, phi_dot: %6.4f",
                        vector_magnitude, trace_value.x_dotdot, trace_value.y_dotdot, trace_value.z_dotdot, trace_value.rho_dot,
                        trace_value.theta_dot, trace_value.phi_dot);)
      vTaskDelay(ONE_SECOND / 2);
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
 * @function: BMI270_dump()
 *
 * @brief:    Dump the contents of the BMI270 registers
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * This function allows the user to enter a register address and a
 * value to write to that register, then reads back the value from the
 * register to verify that the SPI communication is working correctly.
 *
 *--------------------------------------------------------------*/
#define HEADER "\r\n\n          0    1    2    3      4    5    6    7      8    9    A    B      C    D    E    F"

void BMI270_SPI_dump(void)
{
  int               address;
  uint8_t           registers[128];
  spi_transaction_t transaction;

  /*
   * Wait here if we are collecting data
   */
  while ( run_state & IN_COLLECTION )
  {
    vTaskDelay(1);
  }

  /*
   * Read and print the values of all registers
   */
  for ( address = 0x00; address <= 0x7F; address++ )            // Loop through all the registers from 0x00 to 0x7F
  {

    memset(&transaction, 0, sizeof(transaction));               // Clear the transaction structure
    transaction.addr      = 0x80 | address;                     // Register address to read from
    transaction.length    = 2 * 8;                              // Transmit length in bits
    transaction.tx_buffer = NULL;                               // Transmit buffer not used
    transaction.rxlength  = 2 * 8;                              // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_RXDATA;               // Indicate that this is a read operation

    spi_device_transmit(BMI270_handle, &transaction);           // Transmit the transaction

    if ( (address % 0x40) == 0x00 )
    {
      SEND(ALL, sprintf(_xs, HEADER);)                          // Print the register address at the start of each line
    }
    if ( (address & 0x0F) == 0x00 )
    {
      SEND(ALL, sprintf(_xs, "\n0x%02X: ", address);)           // Print the register address at the start of each line
    }
    if ( (address % 4) == 0x00 )
    {
      SEND(ALL, sprintf(_xs, "  ");)
    }
    SEND(ALL, sprintf(_xs, "0x%02X ", transaction.rx_data[1]);) // Print the value read from the register}
    registers[address] = transaction.rx_data[1];
  }

                                                                /*
                                                                 *  Display known values
                                                                 */
  SEND(ALL, sprintf(_xs, "\r\n\r\n");)
  SEND(ALL, sprintf(_xs, "\r\n0x18: Sensor time: %d", (registers[0x1A] << 16) | (registers[0x19] << 8) | registers[18]);)
  SEND(ALL, sprintf(_xs, "\r\n0x22: Temperature: %4.2f", (23.0 + (1 / 512.0) * ((registers[0x23] << 8) + registers[0x22])));)
  SEND(ALL, sprintf(_xs, "\r\n0x24: FIFO length: %d", ((registers[0x25] << 8) + registers[0x24]));)
  SEND(ALL, sprintf(_xs, "\r\n0x0C: ACC X: %04X", ((registers[0x0D] << 8) + registers[0x0C]));)
  SEND(ALL, sprintf(_xs, "\r\n0x0E: ACC Y: %04X", ((registers[0x0F] << 8) + registers[0x0E]));)
  SEND(ALL, sprintf(_xs, "\r\n0x10: ACC Z: %04X", ((registers[0x11] << 8) + registers[0x10]));)
  SEND(ALL, sprintf(_xs, "\r\n0x12: GYRO X: %04X", ((registers[0x13] << 8) + registers[0x12]));)
  SEND(ALL, sprintf(_xs, "\r\n0x14: GYRO Y: %04X", ((registers[0x15] << 8) + registers[0x14]));)
  SEND(ALL, sprintf(_xs, "\r\n0x16: GYRO Z: %04X", ((registers[0x17] << 8) + registers[0x16]));)
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
