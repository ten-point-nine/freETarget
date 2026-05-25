/******************************************************************************
 *
 * file: BMI270.c
 *
 * Bosch BMI270 3-axis accelerometer driver
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
 *
 *****************************************************************************/
#include "stdio.h"
#include "string.h"
#include "driver\gpio.h"
#include "math.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "nvs_flash.h"

#include "trace.h"
#include "json.h"
#include "diag_tools.h"
#include "gpio.h"
#include "helpers.h"
#include "BMI270.h"
#include "BMI270_define.h"
#include "spi.h"
#include "nonvol.h"
#include "IMU.h"

/*
 * Definitions
 */
#define SQ(x) ((x) * (x))

/*
 *  Typedefs
 */
/*
 * A single sample frame read from the BMI270 from each of the registers.
 * Internally, the target software will use the FIFO format for operations.
 * Any function using the register_raw format will need to convert to the
 * FIFO format before use.
 */
typedef struct                                   // A single raw frame read from registers
{
  uint8_t empty;                                 // Here to force uint16_t x to be on a word boundary
  uint8_t dummy;                                 // Dummy byte as read from the SPI bus
  int16_t x_dotdot;                              // Sample frame directly from BMI270
  int16_t y_dotdot;
  int16_t z_dotdot;
  int16_t rho_dot;
  int16_t theta_dot;
  int16_t phi_dot;                               // Z axis rotation speed
} register_raw_frame_t;                          // Value read from sensor

FIFO_raw_t sample_raw_read[SAMPLE_BUFFER_COUNT]; // Space for 10 seconds of data

/*
 *  Local Functions
 */

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
 * @return:   TRUE if the FIFO is completely full of data
 *
 *----------------------------------------------------------------
 *
 * This function is called if the FIFO watermark is reached.
 *
 * By the time we get here there are AT LEAST WATERMARK bytes of
 * samples in the FIFO.  The function will read one WATERMARKs
 * number of cycles and save them into memory.
 *
 * There may be samples left in the FIFO, but we will get them
 * the next time the watermark interrupt is fired.
 *
 *---------------------------------------------------------------*/
bool BMI270_pull_FIFO(void)
{
  spi_transaction_t transaction;
  static bool       return_value = false; // Return TRUE if the FIFO is completely full of data

  /*
   *  Check to see if we are reading a single sample
   */
  IF(IN_SINGLE)          // Single sample?
  {
    return return_value; // Yes, return and do nothing
  }

  /*
   *  We can read the FIFO
   */
  run_state |= IN_COLLECTION;

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "BMI270_FIFO_read()");))

  /*
   *  Read in the next bunch of samples
   */
  memset(&transaction, 0, sizeof(transaction));         // Clear the transaction structure{"TEST":24}
  transaction.addr      = 0x80 | FIFO_DATA;             // Point to the FIFO read regisetrt
  transaction.tx_buffer = NULL;                         // Transmit buffer not used
  transaction.length    = (sizeof(FIFO_raw_t) - 0) * 8; // Transmit length in bits
  transaction.rx_buffer = &sample_raw_read[index_in.outer].dummy;
  transaction.rxlength  = (sizeof(FIFO_raw_t) - 1) * 8; // Receive length in bits
  transaction.flags     = 0;                            // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);

  trace_FIFO_next(&index_in);
  if ( index_in.outer == index_out.outer )              // Wrapped around the buffer is full
  {
    return_value = true;
  }

  /*
   *  Reset the interrupt status
   */
  memset(&transaction, 0, sizeof(transaction)); // Clear the transaction structure{"TEST":24}
  transaction.addr      = 0x80 | INT_STATUS_1;  // Point to the FIFO read regisetrt
  transaction.tx_buffer = NULL;                 // Transmit buffer not used
  transaction.length    = 3 * 8;                // Transmit length in bits
  transaction.rxlength  = 1 * 8;                // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA; // Read into the four byte pointer
  spi_device_transmit(BMI270_handle, &transaction);

  /*
   *  All done, return
   */
  run_state &= ~IN_COLLECTION;
  return return_value;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_read_raw_accel()
 *
 * @brief:    Read acceleration data from the BMI270
 *
 * @return:   Acceleration in FIFO format
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
 * The function collects information from the BMI270 registers
 * and scrambles it into the FIFO format for use by the rest of the program.\
 *
 *--------------------------------------------------------------*/
void BMI270_read_raw_accel(FIFO_raw_frame_t *sample) // Returned values
{
  spi_transaction_t    transaction;
  register_raw_frame_t raw_frame;

  run_state |= IN_SINGLE;

  /*
   * Prepare and read a single sample directly from the BMI270
   */
  memset(&transaction, 0x00, sizeof(transaction));            // Clear the transaction structure
  transaction.addr      = 0x80 | ACCEL_X;                     // Start at Accel Acceleration Data and read all 6 bytes in one transaction
  transaction.length    = (sizeof(register_raw_frame_t)) * 8; // Transmit length in bits (less the empty)
  transaction.tx_buffer = NULL;                               // Send dummy data to read the acceleration data
  transaction.rxlength  = (sizeof(register_raw_frame_t)) * 8; // Don't count the empty
  transaction.rx_buffer = &raw_frame.dummy;                   // Receive buffer to store the raw acceleration data
  transaction.flags     = 0;
  spi_device_transmit(BMI270_handle, &transaction);           // Transmit the transaction

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "raw  x_..: 0X%04X   y_..: 0X%04X   z_..: 0X%04X   rho_.: 0X%04X   theta_.: 0X%04X   phi_.: 0X%04X",
                                   raw_frame.x_dotdot, raw_frame.y_dotdot, raw_frame.z_dotdot, raw_frame.rho_dot, raw_frame.theta_dot,
                                   raw_frame.phi_dot);))

  /*
   *  Scramble the register values to FIFO position
   */
  sample->x_dotdot  = raw_frame.x_dotdot;
  sample->y_dotdot  = raw_frame.y_dotdot;
  sample->z_dotdot  = raw_frame.z_dotdot;
  sample->rho_dot   = raw_frame.rho_dot;
  sample->theta_dot = raw_frame.theta_dot;
  sample->phi_dot   = raw_frame.phi_dot;

  run_state &= ~IN_SINGLE;

  return;
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
 * so to get the vector acceleration of the device, we need to
 * zero the data by taking a sample when the device is stationary
 * and subtracting that from future samples.
 *
 *--------------------------------------------------------------*/
#define NUM_ZERO_SAMPLES 100

void BMI270_find_zero(void)
{
  unsigned int     i;               // Loop counter
  FIFO_raw_frame_t BMI270_FIFO_raw; // Read in the order the FIFO returns data

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "BMI270_find_zero()");))

  /*
   *  Clear the current offset
   */
  json_x_dotdot_offset  = 0;
  json_y_dotdot_offset  = 0;
  json_z_dotdot_offset  = 0;
  json_rho_dot_offset   = 0;
  json_theta_dot_offset = 0;
  json_phi_dot_offset   = 0;

  /*
   *  Read the registers to find the average
   */
  for ( i = 0; i != NUM_ZERO_SAMPLES; i++ )
  {
    BMI270_read_raw_accel(&BMI270_FIFO_raw);          // Take a sample of the raw acceleration data
    json_x_dotdot_offset -= BMI270_FIFO_raw.x_dotdot; // Accumulate the X-axis raw acceleration data
    json_y_dotdot_offset -= BMI270_FIFO_raw.y_dotdot; // Accumulate the Y-axis raw acceleration data
    json_z_dotdot_offset -= BMI270_FIFO_raw.z_dotdot; // Accumulate the Z-axis raw acceleration data
    json_rho_dot_offset -= BMI270_FIFO_raw.rho_dot;
    json_theta_dot_offset -= BMI270_FIFO_raw.theta_dot;
    json_phi_dot_offset -= BMI270_FIFO_raw.phi_dot;
    vTaskDelay(1);
  }

  /*
   * Average the samples to get a more accurate zero level
   */
  json_x_dotdot_offset /= NUM_ZERO_SAMPLES;
  json_y_dotdot_offset /= NUM_ZERO_SAMPLES;
  json_z_dotdot_offset /= NUM_ZERO_SAMPLES;
  json_rho_dot_offset /= NUM_ZERO_SAMPLES;
  json_theta_dot_offset /= NUM_ZERO_SAMPLES;
  json_phi_dot_offset /= NUM_ZERO_SAMPLES;

  /*
   * Put the results in NONVOL
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Zero - X_..: 0X%04X  Y_..: 0X%04X  Z_..: 0X%04X   rho_.: 0X%04X   theta_.: 0X%04X  phi_.: 0X%04X ",
                                  json_x_dotdot_offset, json_y_dotdot_offset, json_z_dotdot_offset, json_rho_dot_offset,
                                  json_rho_dot_offset, json_rho_dot_offset);))

  SEND(ALL, sprintf(_xs, "\r\nCommit settings.");)
  if ( prompt_for_confirm() == true )
  {
    nvs_set_i32(my_handle, NONVOL_X_DOTDOT_OFFSET, json_x_dotdot_offset); // Save the value
    nvs_set_i32(my_handle, NONVOL_Y_DOTDOT_OFFSET, json_y_dotdot_offset);
    nvs_set_i32(my_handle, NONVOL_Z_DOTDOT_OFFSET, json_z_dotdot_offset);
    nvs_set_i32(my_handle, NONVOL_RHO_DOT_OFFSET, json_rho_dot_offset);
    nvs_set_i32(my_handle, NONVOL_THETA_DOT_OFFSET, json_theta_dot_offset);
    nvs_set_i32(my_handle, NONVOL_PHI_DOT_OFFSET, json_phi_dot_offset);
  }
  else
  {
    SEND(ALL, sprintf(_xs, "Settings not changed");)
  }

  /*
   *  All done, return
   */
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
void BMI270_convert_to_g(FIFO_raw_frame_t *sample,                                              // 16 bit numbers read from BBMI270
                         trace_vector_t   *vector                                               // Working values
)
{
  vector->x_dotdot = CAL_SCALE * (real_t)(sample->x_dotdot + json_x_dotdot_offset) * G_PER_LSB; // Convert raw X-axis data to g
  if ( F_ABS(vector->x) < ACCEL_DEAD_BAND )
  {
    vector->x_dotdot = 0;
  }

  vector->y_dotdot = CAL_SCALE * (real_t)(sample->y_dotdot + json_y_dotdot_offset) * G_PER_LSB; // Convert raw Y-axis data to g
  if ( F_ABS(vector->y_dotdot) < ACCEL_DEAD_BAND )
  {
    vector->y_dotdot = 0;
  }

  vector->z_dotdot = CAL_SCALE * (real_t)(sample->z_dotdot + json_z_dotdot_offset) * G_PER_LSB; // Convert raw Z-axis data to g
  if ( F_ABS(vector->z_dotdot) < ACCEL_DEAD_BAND )
  {
    vector->z_dotdot = 0;
  }

  vector->rho_dot = (real_t)(sample->rho_dot + json_rho_dot_offset) * GYRO_PER_LSB;             // Convert raw X-axis data to g
  if ( F_ABS(vector->rho_dot) < GYRO_DEAD_BAND )
  {
    vector->rho_dot = 0;
  }

  vector->theta_dot = (real_t)(sample->theta_dot + json_theta_dot_offset) * GYRO_PER_LSB;       // Convert raw X-axis data to g
  if ( F_ABS(vector->theta_dot) < GYRO_DEAD_BAND )
  {
    vector->theta_dot = 0;
  }

  vector->phi_dot = (real_t)(sample->phi_dot + json_phi_dot_offset) * GYRO_PER_LSB;             // Convert raw X-axis data to g
  if ( F_ABS(vector->phi_dot) < GYRO_DEAD_BAND )
  {
    vector->phi_dot = 0;
  }

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
  real_t           vector_magnitude; // Magnitude of the acceleration vector
  trace_vector_t   trace_vector;
  FIFO_raw_frame_t sample;
  bool             pause = false;

  while ( 1 )
  {
    if ( pause == false )
    {
      BMI270_read_raw_accel(&sample);
      BMI270_convert_to_g(&sample, &trace_vector);        // Convert raw data to g

      vector_magnitude = sqrt(SQ(trace_vector.x_dotdot) + SQ(trace_vector.y_dotdot) +
                              SQ(trace_vector.z_dotdot)); // Calculate the magnitude of the acceleration
      if ( (is_trace & DLT_DEBUG) == 0 )
      {
        SEND(ALL, sprintf(_xs, "\r\n\r|a|: %6.4f,   ax: %6.4f, ay: %6.4f,  az: %6.4f,    rho_dot: %6.4f, theta_dot: %6.4f, phi_dot: %6.4f",
                          vector_magnitude, trace_vector.x_dotdot, trace_vector.y_dotdot, trace_vector.z_dotdot, trace_vector.rho_dot,
                          trace_vector.theta_dot, trace_vector.phi_dot);)
      }
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
  int               address;        // Display address
  int               i;              // Index
  uint8_t           registers[128]; // Copy of registers
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
  SEND(ALL, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "\r\n0x18: Sensor time: %d", (registers[0x1A] << 16) | (registers[0x19] << 8) | registers[18]);)
  SEND(ALL, sprintf(_xs, "\r\n0x22: Temperature: %4.2f", (23.0 + (1 / 512.0) * ((registers[0x23] << 8) + registers[0x22])));)
  SEND(ALL, sprintf(_xs, "\r\n0x24: FIFO length: %d", ((registers[0x25] << 8) + registers[0x24]));)
  SEND(ALL, sprintf(_xs, "\r\n0x0C: ACC X: %04X", ((registers[0x0D] << 8) | registers[0x0C]));)
  SEND(ALL, sprintf(_xs, "\r\n0x0E: ACC Y: %04X", ((registers[0x0F] << 8) | registers[0x0E]));)
  SEND(ALL, sprintf(_xs, "\r\n0x10: ACC Z: %04X", ((registers[0x11] << 8) | registers[0x10]));)
  SEND(ALL, sprintf(_xs, "\r\n0x12: GYRO X: %04X", ((registers[0x13] << 8) | registers[0x12]));)
  SEND(ALL, sprintf(_xs, "\r\n0x14: GYRO Y: %04X", ((registers[0x15] << 8) | registers[0x14]));)
  SEND(ALL, sprintf(_xs, "\r\n0x16: GYRO Z: %04X", ((registers[0x17] << 8) | registers[0x16]));)

  /*
   *  Display the contents of the FIFO loop
   */
  SEND(ALL, sprintf(_xs, "\r\n");)
  for ( i = 0; i != SAMPLE_BUFFER_COUNT; i++ )
  {
    SEND(ALL, sprintf(_xs, "\r\nBuffer: %d   ", i);)
    SEND(ALL, sprintf(_xs, "x_dotdot: %04X   y_dotdot: %04X   z_dotdot:%04X    ", sample_raw_read[i].f[0].x_dotdot,
                      sample_raw_read[i].f[0].y_dotdot, sample_raw_read[i].f[0].z_dotdot);)
    SEND(ALL, sprintf(_xs, "rho_dot: %04X   theta_dot: %04X    phi_dot: %04X", sample_raw_read[i].f[0].rho_dot,
                      sample_raw_read[i].f[0].theta_dot, sample_raw_read[i].f[0].phi_dot);)
  }
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
