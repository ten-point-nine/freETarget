/******************************************************************************
 *
 * file: ICM45686.c
 *
 * Bosch ICM45686 3-axis accelerometer driver
 *
 *****************************************************************************
 *
 * This file contains the driver for the ICM45686 3-axis accelerometer.  The
 * driver is written to be as generic as possible and should work with any
 * implementation of the ICM45686.
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
#include "ICM45686.h"
#include "ICM45686-define.h"
#include "spi.h"
#include "nonvol.h"
#include "IMU.h"
#include "timer.h"

#if USE_ICM45686

/*
 * Definitions
 */

/*
 *  Typedefs
 */

/*
 *  Variables
 */
time_count_64_t last_FIFO_read;                // Remember when we took the last sample

FIFO_packet_t FIFO_queue[SAMPLE_BUFFER_COUNT]; // Space for 10 seconds of data

/*
 *  Local Functions
 */

/*----------------------------------------------------------------
 *
 * @function: ICM45686_init()
 *
 * @brief:    Initalize the ICM45686
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Setup the accelerometer from the table
 *
 *--------------------------------------------------------------*/
void ICM45686_init(unsigned int ICM45686_gpio)
{
  spi_transaction_t transaction;
  int               i;

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "ICM45686_init()");))
  PAUSE("Ready")

  ICM45686_spi_config.spics_io_num = ICM45686_gpio;

  /*
   * Add the accelerometer to the bus
   */
  if ( spi_bus_add_device(SPI2_HOST, &ICM45686_spi_config, &ICM45686_handle) != ESP_OK ) // Add the SPI device to the bus
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to add ICM45686 device to SPI bus");))
  }

                                                                                         /*
                                                                                          * Read the device ID
                                                                                          */

  memset(&transaction, 0, sizeof(transaction));       // Clear the transaction structure
  transaction.addr      = 0x80 | WHO_AM_I;            // Register address to read from
  transaction.length    = 1 * 8;                      // Transmit length in bits
  transaction.tx_buffer = NULL;                       // Transmit buffer not used
  transaction.rxlength  = 1 * 8;                      // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;       // Indicate that this is a read operation

  spi_device_transmit(ICM45686_handle, &transaction); // Dummy read to put into SPI mode
  PAUSE("CHIP_ID")
  spi_device_transmit(ICM45686_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[0] != WHO_I_SHOULD_BE )    // Check the device ID
  {
    DLT(DLT_FATAL, SEND(CONSOLE, sprintf(_xs, "Failed to read ICM45686 device ID: 0x%02X", transaction.rx_data[0]);))
    return;
  }
  else
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "ICM45686 device ID: 0x%02X", transaction.rx_data[0]);))
  }
  vTaskDelay(1);

  /*
   * Program the registers from the configuration table
   */

  i = 0;
  while ( ICM45686_config[i].address != 0x00 )          // Loop through the configuration table until the end is reached
  {
    memset(&transaction, 0, sizeof(transaction));       // Clear the transaction structure
    transaction.addr      = ICM45686_config[i].address; // Register address to write to
    transaction.tx_buffer = ICM45686_config[i].value;   // Send the value to be written to the register
    transaction.length    = 1 * 8;                      // Transmit length in bits
    transaction.rxlength  = 0 * 8;                      // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_TXDATA;       // Indicate that this is a read operation
    PAUSE("Sending register value");
    DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "register 0x%02X: 0x%02X", ICM45686_config[i].address, ICM45686_config[i].value);))
    spi_device_transmit(ICM45686_handle, &transaction); // Transmit the transaction
    vTaskDelay(2);
    i++;
  }

  /*
   * All done, return
   */
  PAUSE("Finished")
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "ICM45686 initialization successful.  Sample Rate: %d", SAMPLE_RATE);))
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_pull_FIFO
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
bool ICM45686_pull_FIFO(void)
{
  spi_transaction_t      transaction;
  static bool            return_value = false; // Return TRUE if the FIFO is completely full of data
  static time_count_64_t delta_time   = 0;

  /*
   *  Check to see if we need to suspend the logging
   */
  IF((IN_TEST | IN_REDUCTION)) // Single sample?
  {
    return false;              // Yes, return and do nothing
  }

  /*
   *  We can read the FIFO
   */
  last_FIFO_read = run_time_us(); // When was the last sample taken

  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "ICM45686_pull_FIFO(), %'llu, delta: %'llu", last_FIFO_read, last_FIFO_read - delta_time);))
  delta_time = last_FIFO_read;

  /*
   *  Read in the next bunch of samples
   */
  memset(&transaction, 0, sizeof(transaction));      // Clear the transaction structure{"TEST":24}
  transaction.addr      = 0x80 | FIFO_DATA;          // Point to the FIFO read regisetrt
  transaction.tx_buffer = NULL;                      // Transmit buffer not used
  transaction.length    = sizeof(FIFO_packet_t) * 8; // Transmit length in bits
  transaction.rx_buffer = &FIFO_queue[index_in.outer];
  transaction.rxlength  = sizeof(FIFO_packet_t) * 8; // Receive length in bits
  transaction.flags     = 0;                         // Indicate that this is a read operation
  spi_device_transmit(ICM45686_handle, &transaction);
  trace_FIFO_next(&index_in);

  if ( index_in.outer == index_out.outer )           // Wrapped around the buffer is full
  {
    return_value = true;
    run_state &= ~IN_FIFO_FILLING;
  }

  /*
   *  Reset the interrupt status
   */
  memset(&transaction, 0, sizeof(transaction)); // Clear the transaction structure
  transaction.addr      = 0x80 | INT1_STATUS0;  // Point to the FIFO read regisetr
  transaction.tx_buffer = 0xAB;                 // Send a zero
  transaction.length    = 1 * 8;                // Transmit length in bits
  transaction.rxlength  = 1 * 8;                // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA; // Read into the four byte pointer
  spi_device_transmit(ICM45686_handle, &transaction);

  /*
   *  All done, return
   */
  return return_value;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_test_FIFO
 *
 * @brief:    Pull out the FIFO samples for testing
 *
 * @return:   NONE
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
#define TO_16A(x) ((int16_t)(((x)[1] << 8) | (x)[0])) // Convert two bytes to a 16-bit integer

char *to_bin(uint8_t value)                           // Convert a byte to a binary string
{
  static char str[9];
  int         i;

  /*
   *  Convert a byte to a binary string
   **/
  for ( i = 0; i != 8; i++ )
  {
    str[7 - i] = (value & (1 << i)) ? '1' : '0';
  }
  str[8] = 0;
  return str;
}

void ICM45686_dump_FIFO(void)
{
  spi_transaction_t transaction;
  int               i;

  memset(&FIFO_queue[0], 0xAB, sizeof(FIFO_packet_t) * 8); // Clear the sample buffer
  memset(&transaction, 0, sizeof(transaction));            // Clear the transaction structure
  transaction.addr      = 0x80 | FIFO_DATA;                // Point to the FIFO read regisetrt
  transaction.tx_buffer = NULL;                            // Transmit buffer not used
  transaction.length    = sizeof(FIFO_packet_t) * 8;       // Transmit length in bits
  transaction.rx_buffer = &FIFO_queue[0];
  transaction.rxlength  = sizeof(FIFO_packet_t) * 8;       // Receive length in bits
  transaction.flags     = 0;                               // Indicate that this is a read operation
  spi_device_transmit(ICM45686_handle, &transaction);

  /*
   * Extract the data
   */
  #if(0)
  for ( i = 0; i != 10; i++ )
  {
    sample.header      = FIFO_queue[0].f[i].buffer[0]; // Header byte for the FIFO frame, not used in this program
    sample.x_dotdot    = (FIFO_queue[0].f[i].buffer[2] << 8) + FIFO_queue[0].f[i].buffer[1];   // Sample frame from ICM_45686
    sample.y_dotdot    = (FIFO_queue[0].f[i].buffer[4] << 8) + FIFO_queue[0].f[i].buffer[3];   // Sample frame from ICM_45686
    sample.z_dotdot    = (FIFO_queue[0].f[i].buffer[6] << 8) + FIFO_queue[0].f[i].buffer[5];   // Sample frame from ICM_45686
    sample.rho_dot     = (FIFO_queue[0].f[i].buffer[8] << 8) + FIFO_queue[0].f[i].buffer[7];   // Sample frame from ICM_45686
    sample.theta_dot   = (FIFO_queue[0].f[i].buffer[10] << 8) + FIFO_queue[0].f[i].buffer[9];  // Sample frame from ICM_45686
    sample.phi_dot     = (FIFO_queue[0].f[i].buffer[12] << 8) + FIFO_queue[0].f[i].buffer[11]; // Z axis rotation speed
    sample.temperature = FIFO_queue[0].f[i].buffer[13];                                        // Temperature data from ICM_45686
    sample.timestamp =
        (FIFO_queue[0].f[i].buffer[15] << 8) + FIFO_queue[0].f[i].buffer[14]; // Timestamp for the sample, not used in this program

    printf("\r\ni:%d H: %s, X..: %04x, Y..: %04x, Z..: %04x, RHO.: %04x, THETA.: %04x, PHI.: %04x, TEMP: %02x, TS: %04x", i,
           to_bin(sample.header & 0xff), (sample.x_dotdot & 0xffff), (sample.y_dotdot & 0xffff), (sample.z_dotdot & 0xffff),
           (sample.rho_dot & 0xffff), (sample.theta_dot & 0xffff), (sample.phi_dot & 0xffff), (sample.temperature & 0xff),
           (sample.timestamp & 0xffff));
  }
#endif 
  /*
   *  Display the leaving state
   */
  for ( i = 0; i != 10; i++ )
  {
    memset(&transaction, 0, sizeof(transaction)); // Clear the transaction structure
    transaction.addr      = 0x80 | FIFO_COUNT_0;  // Point to the FIFO read regisetrt
    transaction.tx_buffer = NULL;                 // Transmit buffer not used
    transaction.length    = 2 * 8;                // Transmit length in bits
    transaction.rx_buffer = NULL;
    transaction.rxlength  = 2 * 8;                // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_RXDATA; // Indicate that this is a read operation
    spi_device_transmit(ICM45686_handle, &transaction);
    printf("\r\nFIFO_COUNT_TOTAL: %d", (transaction.rx_data[1] << 8) | transaction.rx_data[0]);

    memset(&transaction, 0, sizeof(transaction)); // Clear the transaction structure
    transaction.addr      = 0x80 | INT1_STATUS0;  // Point to the FIFO read regisetrt
    transaction.tx_buffer = NULL;                 // Transmit buffer not used
    transaction.length    = 1 * 8;                // Transmit length in bits
    transaction.rx_buffer = NULL;
    transaction.rxlength  = 1 * 8;                // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_RXDATA; // Indicate that this is a read operation
    spi_device_transmit(ICM45686_handle, &transaction);
    printf("\r\nINT1_STATUS0: %02x", transaction.rx_data[0]);
    printf("\r\nIMU_INTERRUPT: %s", gpio_get_level(IMU_INTERRUPT) == 0 ? "ACTIVE" : "INACTIVE");
    printf("\r\n");
    vTaskDelay(1);
  }
  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_find_index_out
 *
 * @brief:    Find the place corresponding to the shot sample
 *
 * @return:   TRUE if the data is valid
 *            Starting point of the trace
 *
 *----------------------------------------------------------------
 *
 * The FIFO is a freerunning queue that is updated perfiodically
 * (SAMPLE_RATE).
 *
 * The entry to the queue is index_in and the output is index_out
 * The function picks up the current index_in and subtracts the
 * entries needed to go backwards in time to the desired shot
 * time (index_out).
 *
 * Sample Calculations:
 *
 * shot:           99,627,789
 * last_FIFO_read:101,515,835
 *                -----------
 *                  1,888,046
 *
 * time_delay_s:  1.888,046 seconds
 * sample_delay:  1,510 samples
 *
 * index_in: (6, 0)
 * index_out:(2, 90)
 *
 * 2  -----  3  -----  4  ----- 5  -----  ( 6 not yet used)
 *  90 | 310     400        400      400
 *     |       1510 back in time          |
 *
 *---------------------------------------------------------------*/
bool ICM45686_find_index_out(time_count_64_t shot) // Time shot occured
{
  real_t       time_delay_s;                       // Time shot occured in micro seconds
  unsigned int sample_delay;

  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "ICM45686_find_index_out(%'llu)", shot);))

  if ( shot == 0 )                                 // No shot time, just start at the current point in the FIFO
  {
    index_out.inner = 0;
    index_out.outer = 0;
    return false;
  }

  /*
   * Calculate how much to go backwards in time
   */
  time_delay_s = (real_t)(last_FIFO_read - shot) / (1000000.0); // Time in microseconds (ago)
  sample_delay = time_delay_s * SAMPLE_RATE;                    // This is how many samples behind

  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "last_FIFO_read: %'llu shot: %'llu) => time_delay_s %7.4f   sample_delay:%u  %u",
                                       last_FIFO_read, shot, time_delay_s, sample_delay, sample_delay / RAW_FRAME_COUNT);))

  /*
   * Figure out what indexes to use
   */
  index_out.outer = (index_in.outer - (sample_delay / RAW_FRAME_COUNT) - 1); // Go backwards in time
  if ( index_out.outer < 0 )                                                 // Gone negative, wrap around
  {
    index_out.outer += SAMPLE_BUFFER_COUNT;
  }

  index_out.inner = RAW_FRAME_COUNT - (sample_delay % RAW_FRAME_COUNT);      // Residiue of the timer index;

  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "in:%d %d = out:%d %d", index_in.outer, index_in.inner, index_out.outer, index_out.inner);))

  /*
   * All done, return
   */
  return true;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_read_raw_accel()
 *
 * @brief:    Read acceleration data from the ICM45686
 *
 * @return:   Acceleration in FIFO format
 *
 *----------------------------------------------------------------
 *
 * The Acceleration data is read from the ICM45686 in 6 bytes,
 * with the X, Y, and Z axis data each consisting of a low byte
 * followed by a high byte. The raw acceleration data is stored
 * in the provided sample structure.
 *
 * IMPORTANT
 *
 * The ICM45686 brings the acceleration in as LSB and MSB.
 * ie, the bytes need to be swapped before use.
 *
 * The function collects information from the ICM45686 registers
 * and scrambles it into the FIFO format for use by the rest of the program.\
 *
 *--------------------------------------------------------------*/
void ICM45686_read_raw_accel(register_single_t *sample) // Returned values
{
  spi_transaction_t    transaction;
  register_raw_frame_t raw_frame;

  /*
   * Prepare and read a single sample directly from the ICM45686
   */
  memset(&transaction, 0x00, sizeof(transaction));            // Clear the transaction structure
  transaction.addr      = 0x80 | ACCEL_DATA_X1_UI;            // Start at Accel Acceleration Data and read all 6 bytes in one transaction
  transaction.length    = (sizeof(register_raw_frame_t)) * 8; // Transmit length in bits (less the empty)
  transaction.tx_buffer = NULL;                               // Send dummy data to read the acceleration data
  transaction.rxlength  = (sizeof(register_raw_frame_t)) * 8; // Don't count the empty
  transaction.rx_buffer = &raw_frame;                         // Receive buffer to store the raw acceleration data
  transaction.flags     = 0;
  spi_device_transmit(ICM45686_handle, &transaction);         // Transmit the transaction

  DLT(DLT_DEBUG,
      SEND(CONSOLE,
           sprintf(_xs, "raw  x_..: 0X%04X   y_..: 0X%04X   z_..: 0X%04X   rho_.: 0X%04X   theta_.: 0X%04X   phi_.: 0X%04X",
                   raw_frame.x_dotdot, raw_frame.y_dotdot, raw_frame.z_dotdot, raw_frame.rho_dot, raw_frame.theta_dot, raw_frame.phi_dot);))

  /*
   *  Scramble the register values to FIFO position
   */
  sample->x_dotdot  = raw_frame.x_dotdot;
  sample->y_dotdot  = raw_frame.y_dotdot;
  sample->z_dotdot  = raw_frame.z_dotdot;
  sample->rho_dot   = raw_frame.rho_dot;
  sample->theta_dot = raw_frame.theta_dot;
  sample->phi_dot   = raw_frame.phi_dot;

/*
 *  All done
 */
#endif
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_read_temperature()
 *
 * @brief:    Read temperature data from the ICM45686
 *
 * @return:   Temperature in FIFO format
 *
 *----------------------------------------------------------------
 *
 * The Temperature data is read from the ICM45686 in 2 bytes,
 * with the temperature data consisting of a high byte followed by a low byte.
 *
 * {"TEST":43}
 *
 *--------------------------------------------------------------*/
void ICM45686_read_temperature(void) // Returned values
{
  spi_transaction_t transaction;
  real_t            temp_raw;        // Signed raw temperature value from the sensor
  real_t            temp_celsius;

  /*
   * Prepare and read a single sample directly from the ICM45686
   */
  memset(&transaction, 0, sizeof(transaction));                              // Clear the transaction structure
  transaction.addr      = 0x80 | TEMP_DATA1_UI;                              // Register address to read from
  transaction.length    = 3 * 8;                                             // Transmit length in bits
  transaction.tx_buffer = NULL;                                              // Transmit buffer not used
  transaction.rxlength  = 3 * 8;                                             // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;                              // Indicate that this is a read operation

  spi_device_transmit(ICM45686_handle, &transaction);                        // Transmit the transaction
  SEND(CONSOLE, sprintf(_xs, "Temperature: 0x%02X %02X %02X", transaction.rx_data[0], transaction.rx_data[1], transaction.rx_data[2]);)
  temp_raw =
      -(((transaction.rx_data[1] << 8) | transaction.rx_data[0]) - 32768.0); // Combine the two bytes into a single raw temperature value
  temp_celsius = (temp_raw / 128.0) + 25.0; // Convert the raw temperature value to degrees Celsius using the formula from the datasheet

  SEND(CONSOLE, sprintf(_xs, "\r\nTemperature: %6.2f (%6.2f °C)", temp_raw, temp_celsius);)
  /*
   *  All done
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_find_zero()
 *
 * @brief:    Determine the resting g levels for the ICM45686
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
 * The function computes the AVERAGE acceleration value which
 * must later be SUBRACTED from the raw sensor value
 *
 *--------------------------------------------------------------*/
#define NUM_ZERO_SAMPLES 200

void ICM45686_find_zero(bool ask_for_confirm) // Ask for save confirmation)
{
  unsigned int      i;                        // Loop counter
  register_single_t ICM45686_register_raw;    // Read in the order the FIFO returns data

  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "ICM45686_find_zero()");))

  run_state |= IN_TEST;

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
    ICM45686_read_raw_accel(&ICM45686_register_raw);        // Take a sample of the raw acceleration data
    json_x_dotdot_offset += ICM45686_register_raw.x_dotdot; // Accumulate the X-axis raw acceleration data
    json_y_dotdot_offset += ICM45686_register_raw.y_dotdot; // Accumulate the Y-axis raw acceleration data
    json_z_dotdot_offset += ICM45686_register_raw.z_dotdot; // Accumulate the Z-axis raw acceleration data
    json_rho_dot_offset += ICM45686_register_raw.rho_dot;
    json_theta_dot_offset += ICM45686_register_raw.theta_dot;
    json_phi_dot_offset += ICM45686_register_raw.phi_dot;
    vTaskDelay(2);
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
  DLT(DLT_INFO,
      SEND(CONSOLE, sprintf(_xs, "Zero - X_..: 0X%04X  Y_..: 0X%04X  Z_..: 0X%04X   rho_.: 0X%04X   theta_.: 0X%04X  phi_.: 0X%04X ",
                            (json_x_dotdot_offset & 0xffff), (json_y_dotdot_offset & 0xffff), (json_z_dotdot_offset & 0xffff),
                            (json_rho_dot_offset & 0xffff), (json_theta_dot_offset & 0xffff), (json_phi_dot_offset & 0xffff));))

  if ( ask_for_confirm == true )
  {
    if ( prompt_for_confirm("Commit settings?") == true )
    {
      SEND(CONSOLE, sprintf(_xs, "\r\nZero offset saved");)
      nvs_set_i32(my_handle, NONVOL_X_DOTDOT_OFFSET, json_x_dotdot_offset); // Save the value
      nvs_set_i32(my_handle, NONVOL_Y_DOTDOT_OFFSET, json_y_dotdot_offset);
      nvs_set_i32(my_handle, NONVOL_Z_DOTDOT_OFFSET, json_z_dotdot_offset);
      nvs_set_i32(my_handle, NONVOL_RHO_DOT_OFFSET, json_rho_dot_offset);
      nvs_set_i32(my_handle, NONVOL_THETA_DOT_OFFSET, json_theta_dot_offset);
      nvs_set_i32(my_handle, NONVOL_PHI_DOT_OFFSET, json_phi_dot_offset);
    }
    else
    {
      SEND(CONSOLE, sprintf(_xs, "\r\nZero offset removed");)
      nvs_set_i32(my_handle, NONVOL_X_DOTDOT_OFFSET, 0); // Save the value
      nvs_set_i32(my_handle, NONVOL_Y_DOTDOT_OFFSET, 0);
      nvs_set_i32(my_handle, NONVOL_Z_DOTDOT_OFFSET, 0);
      nvs_set_i32(my_handle, NONVOL_RHO_DOT_OFFSET, 0);
      nvs_set_i32(my_handle, NONVOL_THETA_DOT_OFFSET, 0);
      nvs_set_i32(my_handle, NONVOL_PHI_DOT_OFFSET, 0);
    }
  }
  else
  {
    SEND(CONSOLE, sprintf(_xs, "\r\nZero offset saved");)
    nvs_set_i32(my_handle, NONVOL_X_DOTDOT_OFFSET, json_x_dotdot_offset); // Save the value
    nvs_set_i32(my_handle, NONVOL_Y_DOTDOT_OFFSET, json_y_dotdot_offset);
    nvs_set_i32(my_handle, NONVOL_Z_DOTDOT_OFFSET, json_z_dotdot_offset);
    nvs_set_i32(my_handle, NONVOL_RHO_DOT_OFFSET, json_rho_dot_offset);
    nvs_set_i32(my_handle, NONVOL_THETA_DOT_OFFSET, json_theta_dot_offset);
    nvs_set_i32(my_handle, NONVOL_PHI_DOT_OFFSET, json_phi_dot_offset);
  }

  /*
   *  All done, return
   */
  run_state &= ~IN_TEST;
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_convert_to_g()
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
#define ACCEL_DEAD_BAND 0.005
#define GYRO_DEAD_BAND  0.0001
#define CAL_SCALE       1.0
#define SWAP_ENDIAN(x)  (((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF))

void ICM45686_convert_to_g(register_single_t *sample, // 16 bit numbers read from BICM45686
                           trace_vector_t    *vector  // Working values
)
{
  /*
   *  Swap the endians
   */
  /*
  sample->x_dotdot  = SWAP_ENDIAN(sample->x_dotdot);
  sample->y_dotdot  = SWAP_ENDIAN(sample->y_dotdot);
  sample->z_dotdot  = SWAP_ENDIAN(sample->z_dotdot);
  sample->rho_dot   = SWAP_ENDIAN(sample->rho_dot);
  sample->theta_dot = SWAP_ENDIAN(sample->theta_dot);
  sample->phi_dot   = SWAP_ENDIAN(sample->phi_dot);
  */
  vector->x_dotdot = CAL_SCALE * (real_t)(sample->x_dotdot - json_x_dotdot_offset) * G_PER_LSB; // Convert raw X-axis data to g
  if ( F_ABS(vector->x_dotdot) < ACCEL_DEAD_BAND )
  {
    vector->x_dotdot = 0;
  }

  vector->y_dotdot = CAL_SCALE * (real_t)(sample->y_dotdot - json_y_dotdot_offset) * G_PER_LSB; // Convert raw Y-axis data to g
  if ( F_ABS(vector->y_dotdot) < ACCEL_DEAD_BAND )
  {
    vector->y_dotdot = 0;
  }

  vector->z_dotdot = CAL_SCALE * (real_t)(sample->z_dotdot - json_z_dotdot_offset) * G_PER_LSB; // Convert raw Z-axis data to g
  if ( F_ABS(vector->z_dotdot) < ACCEL_DEAD_BAND )
  {
    vector->z_dotdot = 0;
  }

  vector->rho_dot = (real_t)(sample->rho_dot - json_rho_dot_offset) * GYRO_PER_LSB;             // Convert raw X-axis data to g
  if ( F_ABS(vector->rho_dot) < GYRO_DEAD_BAND )
  {
    vector->rho_dot = 0;
  }

  vector->theta_dot = (real_t)(sample->theta_dot - json_theta_dot_offset) * GYRO_PER_LSB;       // Convert raw X-axis data to g
  if ( F_ABS(vector->theta_dot) < GYRO_DEAD_BAND )
  {
    vector->theta_dot = 0;
  }

  vector->phi_dot = (real_t)(sample->phi_dot - json_phi_dot_offset) * GYRO_PER_LSB;             // Convert raw X-axis data to g
  if ( F_ABS(vector->phi_dot) < GYRO_DEAD_BAND )
  {
    vector->phi_dot = 0;
  }
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_oscilliscope()
 *
 * @brief:    Create a real time oscilliscope for the accelerometer
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Poll the ICM45686 and print out the acceleration data
 *
 *--------------------------------------------------------------*/
void ICM45686_oscilliscope(void)
{
  real_t            vector_magnitude; // Magnitude of the acceleration vector
  trace_vector_t    trace_vector;
  register_single_t sample;
  bool              pause = false;

  memset(&trace_vector, 0, sizeof(trace_vector));
  while ( 1 )
  {
    if ( pause == false )
    {
      ICM45686_read_raw_accel(&sample);
      ICM45686_convert_to_g(&sample, &trace_vector);      // Convert raw data to g

      vector_magnitude = sqrt(SQ(trace_vector.x_dotdot) + SQ(trace_vector.y_dotdot) +
                              SQ(trace_vector.z_dotdot)); // Calculate the magnitude of the acceleration

      SEND(CONSOLE, sprintf(_xs,
                            "\r\n\rrx: 0x%04X  ry: 0x%04X  rz: 0x%04X   rr: 0x%04X  rt: 0x%04X  rp: 0x%04X   |a|: %6.4f,   ax: %+6.4f, ay: "
                            "%+6.4f,  az: %+6.4f,    "
                            "rho_dot: %+6.4f,  theta_dot: %+6.4f, phi_dot: %+6.4f",
                            (sample.x_dotdot & 0x0000ffff), (sample.y_dotdot & 0x0000ffff), (sample.z_dotdot & 0x0000ffff),
                            (sample.rho_dot & 0x0000ffff), (sample.theta_dot & 0x0000ffff), (sample.phi_dot & 0x0000ffff), vector_magnitude,
                            trace_vector.x_dotdot, trace_vector.y_dotdot, trace_vector.z_dotdot, trace_vector.rho_dot,
                            trace_vector.theta_dot, trace_vector.phi_dot);)

      vTaskDelay(ONE_SECOND);
    }

    if ( serial_available(CONSOLE) != 0 )
    {
      char ch = serial_getch(CONSOLE);
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
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ICM45686_dump()
 *
 * @brief:    Dump the contents of the ICM45686 registers
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
#define TO_16(x)                                                                                                                           \
  (((registers[(x) + 1] << 8) |                                                                                                            \
    registers[(x)]))                // Convert two bytes to a 16 bit value, with the first byte as the MSB and the second byte as the LSB
void ICM45686_SPI_dump(void)
{
  int               address;        // Display address
  uint8_t           registers[128]; // Copy of registers
  spi_transaction_t transaction;

                                    /*
                                     * Read and print the values of all registers
                                     */
  memset(&registers, 0xAB, sizeof(registers));                   // Clear the registers array
  memset(&transaction, 0, sizeof(transaction));                  // Clear the transaction structure

  for ( address = 0; address != 0x80; address++ )
  {
    transaction.addr      = 0x80 | address;                      // Register address to read from
    transaction.length    = (1) * 8;                             // Transmit length in bits
    transaction.tx_buffer = NULL;                                // Transmit buffer not used
    transaction.rxlength  = (1) * 8;                             // Receive length in bits
    transaction.rx_buffer = NULL;                                // Use the pointer as the destination for the read data
    transaction.flags     = SPI_TRANS_USE_RXDATA;                // Indicate that this is a read operation;
    spi_device_transmit(ICM45686_handle, &transaction);          // Transmit the transaction

    registers[address] = transaction.rx_data[0];
  }

  for ( address = 0x00; address < sizeof(registers); address++ ) // Loop through all the registers from 0x00 to 0x7F
  {
    if ( (address % 0x40) == 0x00 )
    {
      SEND(CONSOLE, sprintf(_xs, HEADER);)                       // Print the register address at the start of each line
    }
    if ( (address & 0x0F) == 0x00 )
    {
      SEND(CONSOLE, sprintf(_xs, "\n0x%02X: ", address);)        // Print the register address at the start of each line
    }
    if ( (address % 4) == 0x00 )
    {
      SEND(CONSOLE, sprintf(_xs, "  ");)
    }
    SEND(CONSOLE, sprintf(_xs, "0x%02X ", registers[address]);)  // Print the value read from the register
  }

                                                                 /*
                                                                  *  Display known values
                                                                  */
  SEND(CONSOLE, sprintf(_xs, "\r\n");)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x22: Temperature: %4.2f", ((real_t)TO_16(TEMP_DATA1_UI)) / 128.0 + 25.0);)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x24: FIFO length: %d", ((registers[0x25] << 8) + registers[0x24]));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x0C: ACC X: %04X", TO_16(ACCEL_DATA_X1_UI));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x0E: ACC Y: %04X", TO_16(ACCEL_DATA_X1_UI + 2));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x10: ACC Z: %04X", TO_16(ACCEL_DATA_X1_UI + 4));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x12: GYRO X: %04X", TO_16(ACCEL_DATA_X1_UI + 6));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x14: GYRO Y: %04X", TO_16(ACCEL_DATA_X1_UI + 8));)
  SEND(CONSOLE, sprintf(_xs, "\r\n0x16: GYRO Z: %04X", TO_16(ACCEL_DATA_X1_UI + 10));)

/*
 *  Display the contents of the FIFO loop
 */
#if ( 0 )
  SEND(CONSOLE, sprintf(_xs, "\r\n");)
  for ( i = 0; i != SAMPLE_BUFFER_COUNT; i++ )
  {
    SEND(CONSOLE, sprintf(_xs, "\r\nBuffer: %d   ", i);)
    SEND(CONSOLE, sprintf(_xs, "Header: %02X   ", FIFO_queue[i].f[0].header);)
    SEND(CONSOLE, sprintf(_xs, "x_dotdot: %04X   y_dotdot: %04X   z_dotdot:%04X    ", FIFO_queue[i].f[0].x_dotdot,
                          FIFO_queue[i].f[0].y_dotdot, FIFO_queue[i].f[0].z_dotdot);)
    SEND(CONSOLE, sprintf(_xs, "rho_dot: %04X   theta_dot: %04X    phi_dot: %04X", FIFO_queue[i].f[0].rho_dot, FIFO_queue[i].f[0].theta_dot,
                          FIFO_queue[i].f[0].phi_dot);)
    SEND(CONSOLE, sprintf(_xs, "temperature: %02X   timestamp: %04X", FIFO_queue[i].f[0].temperature, FIFO_queue[i].f[0].timestamp);)
  }
#endif
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}
