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

/*
 * BMI270 Register Addresses
 */
#define CHIP_ID         0x00
#define PWR_CONF        0x7C
#define INIT_CTRL       0x59
#define INIT_DATA       0x5E
#define INTERNAL_STATUS 0x21
#define ACCEL_A         0x0C
#define TIMER           0x18

/*
 *  Typedefs
 */
typedef struct
{
  int address; // Register being accessed
  int value;   // Value to be written to the register
} BMI270_config_t;

/*
 * Function Prototypes
 */

/*
 * Variables
 */
trace_raw_t         BMI270_zero_sample; // Sample to hold the zeroed acceleration data
spi_device_handle_t BMI270_handle;      // Handle for the SPI device

spi_device_interface_config_t BMI270_spi_config = {
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

BMI270_config_t BMI270_config[] = {
    //       76543210
    {0x7D, 0b00000110       }, // PWR_CTRL, Enable Gyro and Accel, disable Aux and temperature
    {0x40, 0b01010000 + 0x0b}, // ACC_CONF Performance optimized, Average 4, 800Hz
    {0,    0                },
    {0x41, 0b00000000       }, // ACC_RANGE +/-2g
    {0x42, 0b11010000 + 0x0b}, // GYR_CONF, Performance Optimized, Average 4, 800 samples
    {0x43, 0b00000100       }, // GYR_RANGE, 125 dps
    {0x45, 0b10001000       }, // FIFO_DOWNS FIFO downsampling
    {0x46, 0b00000000       }, // FIFO_WTM_0,  Watermark LSB
    {0x47, 0b10000000       }, // FIFO_WTM_1,  Watermark MSB
    {0x48, 0b00000000       }, // FIFO_CONFIG_0, No timestamp, do not stop if FIFO full
    {0x49, 0b11000000       }, // FIFO_CONNFIG_1, Store Accel and GYro
    {0x53, 0b00001000       }, // INT1_IO_CTRL, INT1 enabled, push pull, active low
    {0x54, 0b00000000       }, // INT2_IO_CTRL, No interrupt on INT2
    {0x55, 0b00000000       }, // INT_LATCH, non-latched
    {0x58, 0b00000010       }, // INT1_MAP, FIFO watermark interrupt mapped to INT1
    {0x59, 0b00000000       }, // INT2_MAP, No interrupts mapped to INT2      {0x70, 0b00000000       }, // NV_CONF, Advanced power up disabled
    {0x7C, 0b00000011       }, // PWR_CONF, Advanced power up enabled
    {0x7D, 0b00000110       }, // PWR_CTRL, Enable Gyro and Accel, disable Aux and temperature
    {0x7E, 0b10110000       }, // CMD, Clear the FIFO
    {0x00, 0x00             }  // End of the configuration file
};

/*
 * @name  Global array that stores the configuration file of BMI270
 *
 * Copyright Bosch
 *
 *
 * See
 *
 * https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270_maximum_fifo.c
 *
 */
const uint8_t bmi270_maximum_fifo_config_file[] = { // 22 x 15 = 330 bytes  -> 2640 bits (13.2ms)
    //     0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21
    0xc8, 0x2e, 0x00, 0x2e, 0x80, 0x2e, 0x1a, 0x00, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, // 0
    0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0xc8, 0x2e, 0x00, 0x2e, 0x90, 0x32, 0x21, 0x2e, 0x59, 0xf5, 0x10, 0x30, 0x21, 0x2e, 0x6a, 0xf5,
    0x1a, 0x24, 0x22, 0x00, 0x80, 0x2e, 0x3b, 0x00, 0xc8, 0x2e, 0x44, 0x47, 0x22, 0x00, 0x37, 0x00, 0xa4, 0x00, 0xff, 0x0f, 0xd1, 0x00,
    0x07, 0xad, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1,
    0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e, 0x00, 0xc1, 0x80, 0x2e,

    0x00, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x24, 0xfc, 0xf5, 0x80, 0x30, 0x40, 0x42, 0x50, 0x50, 0x00, 0x30, 0x12, 0x24, // 4
    0xeb, 0x00, 0x03, 0x30, 0x00, 0x2e, 0xc1, 0x86, 0x5a, 0x0e, 0xfb, 0x2f, 0x21, 0x2e, 0xfc, 0xf5, 0x13, 0x24, 0x63, 0xf5, 0xe0, 0x3c,
    0x48, 0x00, 0x22, 0x30, 0xf7, 0x80, 0xc2, 0x42, 0xe1, 0x7f, 0x3a, 0x25, 0xfc, 0x86, 0xf0, 0x7f, 0x41, 0x33, 0x98, 0x2e, 0xc2, 0xc4,
    0xd6, 0x6f, 0xf1, 0x30, 0xf1, 0x08, 0xc4, 0x6f, 0x11, 0x24, 0xff, 0x03, 0x12, 0x24, 0x00, 0xfc, 0x61, 0x09, 0xa2, 0x08, 0x36, 0xbe,
    0x2a, 0xb9, 0x13, 0x24, 0x38, 0x00, 0x64, 0xbb, 0xd1, 0xbe, 0x94, 0x0a, 0x71, 0x08, 0xd5, 0x42, 0x21, 0xbd, 0x91, 0xbc, 0xd2, 0x42,

    0xc1, 0x42, 0x00, 0xb2, 0xfe, 0x82, 0x05, 0x2f, 0x50, 0x30, 0x21, 0x2e, 0x21, 0xf2, 0x00, 0x2e, 0x00, 0x2e, 0xd0, 0x2e, 0xf0, 0x6f, // 9
    0x02, 0x30, 0x02, 0x42, 0x20, 0x26, 0xe0, 0x6f, 0x02, 0x31, 0x03, 0x40, 0x9a, 0x0a, 0x02, 0x42, 0xf0, 0x37, 0x05, 0x2e, 0x5e, 0xf7,
    0x10, 0x08, 0x12, 0x24, 0x1e, 0xf2, 0x80, 0x42, 0x83, 0x84, 0xf1, 0x7f, 0x0a, 0x25, 0x13, 0x30, 0x83, 0x42, 0x3b, 0x82, 0xf0, 0x6f,
    0x00, 0x2e, 0x00, 0x2e, 0xd0, 0x2e, 0x12, 0x40, 0x52, 0x42, 0x00, 0x2e, 0x12, 0x40, 0x52, 0x42, 0x3e, 0x84, 0x00, 0x40, 0x40, 0x42,
    0x7e, 0x82, 0xe1, 0x7f, 0xf2, 0x7f, 0x98, 0x2e, 0x6a, 0xd6, 0x21, 0x30, 0x23, 0x2e, 0x61, 0xf5, 0xeb, 0x2c, 0xe1, 0x6f};

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
  transaction.length    = 1 * 8;                    // Transmit length in bits
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.rxlength  = 1 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation

  spi_device_transmit(BMI270_handle, &transaction); // Dummy read to put into SPI mode
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[0] != 0x24 )             // Check the device ID
  {
    DLT(DLT_FATAL, SEND(ALL, sprintf(_xs, "Failed to read BMI270 device ID: 0x%02X", transaction.rx_data[0]);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270 device ID: 0x%02X", transaction.rx_data[0]);))
  }

  /*
   * Program the registers from the configuration table
   */
  i = 0;
  printf("here  ");
  vTaskDelay(5 * ONE_SECOND);
  while ( BMI270_config[i].address != 0x00 )          // Loop through the configuration table until the end is reached
  {
    memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
    transaction.addr      = BMI270_config[i].address; // Register address to write to
    transaction.tx_buffer = BMI270_config[i].value;   // Send the value to be written to the register
    transaction.length    = 1 * 8;                    // Transmit length in bits
    transaction.rxlength  = 0 * 8;                    // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_TXDATA;     // Indicate that this is a read operation
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "register 0x%02X: 0x%02X", BMI270_config[i].address, BMI270_config[i].value);))
    spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
    vTaskDelay(2);
    i++;
  }

  /*
   * Programming the API
   */
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = PWR_CONF;                                    // Disable the accelerometer before programming the API
  transaction.tx_buffer = 0;                                           // Disable the
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction

  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_CTRL;                                   // Prepare the configuration file for the API programming
  transaction.tx_buffer = 0;                                           // Transmit buffer
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction

  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_DATA;                                   // Prepare the configuration file for the API programming
  transaction.tx_buffer = &bmi270_maximum_fifo_config_file;            // Transmit the configuration file for the API programming
  transaction.length    = sizeof(bmi270_maximum_fifo_config_file) * 8; // Transmit length in bits
  transaction.rxlength  = 0;                                           // Indicate that this is a read operation
  transaction.flags     = 0;                                           // Use default flags for a large write operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction
  vTaskDelay(1);

  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_CTRL;                                   // Start the processor going
  transaction.tx_buffer = 1;                                           // Complete the API programming
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction


  /*
   *  Make sure the API is properly initialized
   */
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | INTERNAL_STATUS;   // Read the internal status register to check if the API is properly initialized
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.length    = 1 * 8;                    // Transmit length in bits
  transaction.rxlength  = 1 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
  if ( transaction.rx_data[0] != 0x1 )              // Check the device ID
  {
    DLT(DLT_FATAL, SEND(ALL, sprintf(_xs, "Initialization failed: 0x%02X", transaction.rx_data[0]);))
  }

  /*
   * Program the registers from the configuration table
   */
  i = 0;
  printf("\r\nhere \r\n ");
  vTaskDelay(5 * ONE_SECOND);
  while ( BMI270_config[i].address != 0x00 )          // Loop through the configuration table until the end is reached
  {
    memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
    transaction.addr      = BMI270_config[i].address; // Register address to write to
    transaction.tx_buffer = BMI270_config[i].value;   // Send the value to be written to the register
    transaction.length    = 2 * 8;                    // Transmit length in bits
    transaction.rxlength  = 0 * 8;                    // Receive length in bits
    transaction.flags     = SPI_TRANS_USE_TXDATA;     // Indicate that this is a read operation
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "register 0x%02X: 0x%02X", BMI270_config[i].address, BMI270_config[i].value);))
    spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
    vTaskDelay(2);
    i++;
  }

  /*
   * All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270 initialization successful");))
  return;
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_device_ID()
 *
 * @brief:    Read the device ID of the BMI270
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Read the device ID as a single transaction to make sure the
 * SPI is working and the device is responding.
 *
 *--------------------------------------------------------------*/
unsigned int BMI270_device_ID(void)
{
  spi_transaction_t transaction;

  /*
   * Read the device ID
   */
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | CHIP_ID;           // Register address to read from
  transaction.length    = 1 * 8;                    // Transmit length in bits
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.rxlength  = 1 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation

  spi_device_transmit(BMI270_handle, &transaction); // Dummy read to put into SPI mode
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[0] != 0x24 )             // Check the device ID
  {
    SEND(ALL, sprintf(_xs, "Failed to read BMI270 device ID: 0x%02X", transaction.rx_data[0]);)
  }
  else
  {
    SEND(ALL, sprintf(_xs, "BMI270 device ID: 0x%02X", transaction.rx_data[0]);)
  }

  /*
   * All done, return
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return transaction.rx_data[0]; // Return the device ID
}

/*----------------------------------------------------------------
 *
 * @function: BMI270_device_status()
 *
 * @brief:    Read the device status of the BMI270
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Read the device status as a single transaction to make sure the
 * SPI is working and the device is responding.
 *
 *--------------------------------------------------------------*/
unsigned int BMI270_device_status(void)
{
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | INTERNAL_STATUS;   // Register address to read from
  transaction.length    = 1 * 8;                    // Transmit length in bits
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.rxlength  = 1 * 8;                    // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation

  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[0] != 0x1 )              // Check the device ID
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Initialization failed: 0x%02X", transaction.rx_data[0]);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initialization successful: 0x%02X", transaction.rx_data[0]);))
  }
  /*
   * All done, return
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return transaction.rx_data[0]; // Return the device ID
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
void BMI270_read_raw_accel(trace_raw_t *sample)    // TRUE if a zero offset it to be applied
{
  esp_err_t         ret;
  spi_transaction_t transaction;
  trace_raw_t       filler;

  memset(&filler, 0xff, sizeof(trace_raw_t));      // Clear the sample structure
  memset(&transaction, 0x00, sizeof(transaction)); // Clear the transaction structure
  transaction.addr      = 0x80 | 0x18;             // Start at Accel Acceleration Data Low register and read all 6 bytes in one transaction
  transaction.length    = (sizeof(trace_raw_t)) * 8;      // Transmit length in bits
  transaction.tx_buffer = &filler;                        // Send dummy data to read the acceleration data
  transaction.rxlength  = (sizeof(trace_raw_t)) * 8;      //
  transaction.rx_buffer = sample;                         // Receive buffer to store the raw acceleration data
  transaction.flags     = 0;

  ret = spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
  if ( ret != ESP_OK )
  {
    printf("error");
  }
  return;                                                 //}
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
    BMI270_read_raw_accel(&BMI270_raw); // Take a sample of the raw acceleration data
    BMI270_sum.x += BMI270_raw.x;       // Accumulate the X-axis raw acceleration data
    BMI270_sum.y += BMI270_raw.y;       // Accumulate the Y-axis raw acceleration data
    BMI270_sum.z += BMI270_raw.z;       // Accumulate the Z-axis raw acceleration data
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
  real_t        vector_magnitude;                    // Magnitude of the acceleration vector
  trace_raw_t   previous_raw;                        // Previous sample
  trace_raw_t   present_raw;                         // Present sample
  trace_point_t previous;                            // Previous sample converted to g
  trace_point_t present;                             // Present sample converted to g
#if ( 0 )
  BMI270_find_sample_out(NUM_ZERO_SAMPLES);          // Start at the point where we took the zero samples to get the most recent data

  BMI270_read_raw_accel(&previous_raw);

  while ( BMI270_read_raw_accel(&present_raw) != 0 ) // Read the acceleration dataP )
  {
    BMI270_convert_to_g(&present_raw, &present);     // Convert raw data to g

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
#endif
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
      BMI270_read_raw_accel(&samples[0]);
      {

        //        BMI270_convert_to_g(&samples[0], &present);                                // Convert raw data to g

        //        vector_magnitude = sqrt(SQ(present.ax) + SQ(present.ay) + SQ(present.az)); // Calculate the magnitude of the acceleration
        //        vector

        SEND(ALL, sprintf(_xs, "\r\n\rlength: %d, ax: 0X%04X, ay: 0X%04X  az: 0X%04X, rho: 0X%04X, theta: 0X%04X, phi: 0X%04X",
                          sizeof(trace_raw_t), samples[0].x, samples[0].y, samples[0].z, samples[0].rho, samples[0].theta, samples[0].phi);)
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
 * @function: BMI270_SPI_test()
 *
 * @brief:    Send a command to the BMI270 and read the response to test the SPI communication
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
void BMI270_SPI_test(void)
{
  int address;
  int value;

  address = get_hex("Enter the register address to read (in hex): ");
  value   = get_hex("Enter the value to write to the register (in hex): ");

  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = address;                  // Register address to write to
  transaction.tx_buffer = &value;                   // Send the value to be written to the register
  transaction.length    = 8;                        // Transmit length in bits
  transaction.rxlength  = 0;                        // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;     // Indicate that this is a write operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  vTaskDelay(5 * ONE_SECOND);                       // Wait for the write to complete
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | address;           // Register address to read from
  transaction.length    = 8;                        // Transmit length in bits
  transaction.tx_buffer = NULL;                     // Transmit buffer not used
  transaction.rxlength  = 8;                        // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_RXDATA;     // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  if ( transaction.rx_data[0] != value )            // Check the value read back from the register
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "SPI test failed: wrote 0x%02X but read back 0x%02X", value, transaction.rx_data[0]);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "SPI test successful: wrote 0x%02X and read back 0x%02X", value, transaction.rx_data[0]);))
  }

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
  int address;

  /*
   * Read and print the values of all registers
   */
  for ( address = 0x00; address <= 0x7F; address++ )            // Loop through all the registers from 0x00 to 0x7F
  {
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));               // Clear the transaction structure
    transaction.addr      = 0x80 | address;                     // Register address to read from
    transaction.length    = 8;                                  // Transmit length in bits
    transaction.tx_buffer = NULL;                               // Transmit buffer not used
    transaction.rxlength  = 8;                                  // Receive length in bits
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
    SEND(ALL, sprintf(_xs, "0x%02X ", transaction.rx_data[0]);) // Print the value read from the register}
  }

  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
