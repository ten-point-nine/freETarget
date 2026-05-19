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

#define G2        (0)                                        // Select +/- 2g range
#define G2_RANGE  (4.0)
#define G4        1                                          // Select +/- 4g range
#define G4_RANGE  (8.0)
#define G8        2                                          // Select +/- 8g range
#define G8_RANGE  (16.0)
#define G16       3                                          // Select +/- 16g range
#define G16_RANGE (32.0)

#define G_RANGE   G2
#define G_PER_LSB (G2_RANGE / 65535.0)                       // g per LSB for the selected range

#define GYRO_2000       0                                    // Select +/- 2000 degrees per second
#define GYRO_RANGE_2000 (4000)                               //
#define GYRO_1000       1                                    // Select +/- 1000 degrees per second
#define GYRO_RANGE_1000 (2000)                               //
#define GYRO_500        2                                    // Select +/- 500 degrees per second
#define GYRO_RANGE_500  (1000)                               //
#define GYRO_250        3                                    // Select +/- 250 degrees per second
#define GYRO_RANGE_259  (500)                                //
#define GYRO_125        4                                    // Select +/- 125 degrees per second
#define GYRO_RANGE_125  (250)                                //

#define GYRO_RANGE   GYRO_125
#define GYRO_PER_LSB (GYRO_RANGE_125 / 65535.0 * PI / 180.0) // in Radians per LSB

#define SQ(x) ((x) * (x))

/*
 * BMI270 Register Addresses
 */
#define CHIP_ID         0x00
#define ACCEL_X         0x0C           // Acceleration X, Y, Z  Gyro X, Y, Z
#define SENSORTIME_0    0x18           // Sensor time register
#define INTERNAL_STATUS 0x21
#define FIFO_LENGTH_0   0x24           // LSB of FIFO length
#define FIFO_LENGTH_1   0x25           // MSB of FIFO length
#define FIFO_DATA       0x26           // FIFO data register

#define ACC_CONF        0x40           // Acceleration Configuration
#define acc_odr         (0x09 << 0)    // (odr_200) Output data rate 200
#define acc_bwp         (0x02 << 4)    // (norm_avg4) Average four samples
#define acc_filter_perf (0x01 << 7)    // (hp) Optimized for parformance

#define ACC_RANGE 0x41                 // Acceleration range
#define acc_range 0x00                 // +/- 2g

#define GYR_CONF        0x42           // Gyro Configuration
#define gyr_odr         0x09           // (odr_200)
#define gyr_bwp         (0x2 << 4)     // (norm) Bandwidth coefficient
#define gyr_noise_perf  (1 << 6)       // (hp) performance optiminzed
#define gyr_filter_perf (1 << 7)       // (hp) performance optimized

#define GYR_RANGE 0x43                 // Gyro Range
#define gyr_range 0x03                 // (range_250) +/- 250 dps
#define ois_range (0x00 << 3)          // (range_250) Full scale resolution +/- 250 dps

#define FIFO_DOWNS         0x45        // FIFO downsampling
#define gyr_fifo_downs     (0x00 << 0) // Downsampling for gyro, 2**gyr_fifo_downs
#define gyr_fifo_filt_data (0x01 << 3) // Filtered data
#define acc_fifo_downs     (0x00 << 4) // Downsampling for accel, 2**acc_fifo_downs
#define acc_fifo_filt_data (0x01 << 7) // Filtered data

#define FIFO_WTM_0 0x46                // FIFO Watermark lsb
#define FIFO_WTM_1 0x47                // FIFO Watermark msb
#define watermark  100                 // Interrupt after 400 samples

#define FIFO_CONFIG_0     0x48         // FIFO Frame Configuration
#define fifo_stop_on_full (0x01 << 0)  // (disable) do not stop on full
#define fifo_time_en      (0x00 << 1)  // (disable) do not return sensor time frame

#define FIFO_CONFIG_1     0x49         // FIFO Frame Content Configuration
#define fifo_tag_int_1_en 0x00         // (int_edge) enable tag on rising edge of int 12 pin
#define fifo_tag_int_2_en (0x00 << 2)  // (int_edge) enable tag on rising edge of int 2 pin
#define fifo_header_en    (0x00 << 4)  // (disable) no heder is stored
#define fifo_aux_en       (0x00 << 5)  // (disable) no Auxilary sensor data is stored
#define fifo_acc_en       (0x01 << 6)  // (enable) Accelerometer data is stored
#define fifo_gyr_en       (0x01 << 7)  // (enable) Gryoscope data is stored

#define INT_1_IO_CTRL 0x53             // Interrupt 1 configuration
#define lvl           (0x00 << 1)      // (active_low)
#define od            (0x00 << 2)      // (od) push pull
#define output_en     (0x01 << 3)      // (on)
#define input_en      (0x00 << 4)      // (off)

#define INT_2_IO_CTRL  0x54            // Interrupt 2 configuration
#define int_2_not_used 0x00            // disable everything

#define INT_LATCH 0x55                 // Interupt latch
#define int_latch 0x01                 // Permanent

#define INT_MAP_DATA 0x58              // Data interupt mapping for both INT pins
#define ffull_int1   (0x01 << 0)       // FIFO full to int 1
#define fwm_int1     (0x01 << 1)       // FIFO watermark to int 1
#define drdy_int1    (0x01 << 2)       // Data ready to int 1
#define err_int1     (0x01 << 3)       // Error interrupt to int 1
#define ffull_int2   (0x01 << 4)       // FIFO full to int 2
#define fwm_int2     (0x01 << 5)       // FIFO watermark to int 2
#define drdy_int2    (0x01 << 6)       // Data ready to int 2
#define err_int2     (0x01 << 7)       // Error interrupt to int 2

#define PWR_CONF          0x7C         // Power mode configuration
#define adv_power_save    0x00         // Advanced powerf save disabled
#define fifo_self_wake_up (0x01 << 1)  // (fsw_on) FIFO enabled in low power mode
#define fup_en            (0x01 << 2)  // (fup_on) Fast power up enabled

#define PWR_CTRL 0x7D                  // Power mode control register
#define aux_en   (0x00 << 0)           // (aux_off) disable aux sensor
#define gyr_en   (0x01 << 1)           // (gyr_on) gyro enabled
#define acc_en   (0x01 << 2)           // (acc_on) Acclerometer enabled
#define temp_en  (0x00 << 3)           // (temp_off) Temperature disabled

#define CMD      0x7E                  // Command register
#define fifo_cmd (0x15)                // (fifo_flush) Clear FIFO contrent

#define INIT_CTRL 0x59
#define INIT_DATA 0x5E

#define FIFO_LENGTH 0x24
#define FIFO_DATA   0x26

#define g_RANGE   g2 // Use 2G for the range
#define g_PER_LSB ((g2_RANGE) / 65565.0)

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
#define DEBUG                                                                                                                              \
  printf("\r\nrx_data: %02X %02X %02X %02X", transaction.rx_data[0], transaction.rx_data[1], transaction.rx_data[2],                       \
         transaction.rx_data[3]);

/*
 * Variables
 */
static trace_raw_t         BMI270_zero_sample; // Sample to hold the zeroed acceleration data
static spi_device_handle_t BMI270_handle;      // Handle for the SPI device

static spi_device_interface_config_t BMI270_spi_config = {
    // Configuration for the SPI device
    .command_bits     = 0,                                            // No command phase
    .address_bits     = 8,                                            //
    .dummy_bits       = 0,                                            // No dummy bits
    .mode             = 0,                                            // SPI mode 0
    .clock_source     = SPI_CLK_SRC_DEFAULT,                          // Use default clock source
    .duty_cycle_pos   = 128,                                          // 50% duty cycle
    .cs_ena_pretrans  = 0,                                            // No pre-transaction CS activation
    .cs_ena_posttrans = 0,                                            // No post-transaction CS activation
    .clock_speed_hz   = 2 * 1000 * 1000,                              // 2 MHz clock speed (do not set higher than 2 MHz)
    .input_delay_ns   = 0,                                            // No input delay
    .spics_io_num     = BMI270_CS,                                    // CS pin
    .flags            = SPI_DEVICE_NO_DUMMY,                          // No special flags
    .queue_size       = 1,
    .pre_cb           = NULL,                                         // Callback to be called before a transmission is started.
    .post_cb          = NULL                                          // Callback to be called after a transmission has completed.
};

static BMI270_config_t BMI270_config[] = {
    {ACC_CONF,      acc_odr + acc_bwp + acc_filter_perf                                      },
    {ACC_RANGE,     acc_range                                                                }, // ACC_RANGE +/-2g
    {GYR_CONF,      gyr_odr + gyr_bwp + gyr_noise_perf + gyr_filter_perf                     }, //  GYR_CONF, Performance Optimized, Average 4, 800 samples
    {GYR_RANGE,     gyr_range + ois_range                                                    }, // GYR_RANGE, 125 dps
    {FIFO_DOWNS,    gyr_fifo_downs + gyr_fifo_filt_data + acc_fifo_downs + acc_fifo_filt_data}, // FIFO_DOWNS FIFO downsampling
    {FIFO_WTM_0,    watermark & 0x00ff                                                       }, // FIFO Watermark lsb
    {FIFO_WTM_1,    (watermark >> 8) & 0x00ff                                                }, // FIFO Watermark msb
    {FIFO_CONFIG_0, fifo_stop_on_full + fifo_time_en                                         }, // FIFO_CONFIG_0, No timestamp, do not stop if FIFO full
    {FIFO_CONFIG_1, fifo_tag_int_1_en + fifo_tag_int_2_en + fifo_header_en + fifo_aux_en + fifo_acc_en +
                        fifo_gyr_en                                       }, // FIFO_CONFIG_1, Store gyro and accel data
    {INT_1_IO_CTRL, lvl + od + output_en + input_en                                          }, // INT_1_IO_CTRL Interrupt 1 used
    {INT_2_IO_CTRL, int_2_not_used                                                           }, // INT_2_IO_CTRL, not used
    {INT_LATCH,     int_latch                                                                }, // INT_LATCH, latched
    {INT_MAP_DATA,  fwm_int1                                                                 }, // INT1_MAP_DATA, FIFO watermark interrupt mapped to INT1
    {PWR_CTRL,      aux_en + gyr_en + acc_en + temp_en                                       }, // PWR_CTRL, Enable Gyro and Accel, disable Aux and temperature
    {CMD,           fifo_cmd                                                                 }, // (fifo_flush) Clear FIFO contrent
    {PWR_CONF,      adv_power_save + fifo_self_wake_up +
                   fup_en                                                      }, //    adv_power_save + fifo_self_wake_up + fup_en         }, // (fup_on) Fast power up enabled
    {0x00,          0x00                                                                     }  // End of the configuration file
};

trace_FIFO_read_t sample_read[10]; // Allow for 10 reads

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

  PAUSE("sending INIT_CONTROL = 0")
  memset(&transaction, 0, sizeof(transaction));                        // Clear the transaction structure
  transaction.addr      = INIT_CTRL;                                   // Prepare the configuration file for the API programming
  transaction.tx_buffer = 0;                                           // Transmit buffer
  transaction.length    = 8;                                           // Transmit length in bits
  transaction.rxlength  = 0;                                           // Receive length in bits
  transaction.flags     = SPI_TRANS_USE_TXDATA;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction);                    // Transmit the transaction

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
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "register 0x%02X: 0x%02X", BMI270_config[i].address, BMI270_config[i].value);))
    // PAUSE("Sending register value");
    spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction
    vTaskDelay(2);
    i++;
  }

  /*
   * All done, return
   */
  PAUSE("Finished")
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
 * It reads all the samples in the FIFO and stores them in the
 * sample buffer.
 *
 *---------------------------------------------------------------*/
unsigned int BMI270_pull_FIFO(void)
{
  spi_transaction_t transaction;
  int               FIFO_length; // How many samples are waiting?

  run_state |= IN_COLLECTION;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BMI270_FIFO_read()");))

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

  DEBUG

  FIFO_length = (transaction.rx_data[2] << 8) + transaction.rx_data[1];

  if ( FIFO_length != 0 )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "FIFO length %d", FIFO_length);))
  }

  run_state &= ~IN_COLLECTION;
  FIFO_length = 0;

  return FIFO_length;
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
void BMI270_convert_to_g(trace_raw_t *sample, trace_point_t *actual)
{
  actual->x_dotdot = 2.0 * ((real_t)sample->f.x_dotdot) * G_PER_LSB; // Convert raw X-axis data to g
  if ( F_ABS(actual->x) < ACCEL_DEAD_BAND )
  {
    actual->x_dotdot = 0;
  }

  actual->y_dotdot = 2.0 * ((real_t)sample->f.y_dotdot) * G_PER_LSB; // Convert raw Y-axis data to g
  if ( F_ABS(actual->y_dotdot) < ACCEL_DEAD_BAND )
  {
    actual->y_dotdot = 0;
  }

  actual->z_dotdot = 2.0 * ((real_t)sample->f.z_dotdot) * G_PER_LSB; // Convert raw Z-axis data to g
  if ( F_ABS(actual->z_dotdot) < ACCEL_DEAD_BAND )
  {
    actual->z_dotdot = 0;
  }

  actual->rho_dot = ((real_t)sample->f.rho_dot) * GYRO_PER_LSB;      // Convert raw X-axis data to g
  if ( F_ABS(actual->rho_dot) < GYRO_DEAD_BAND )
  {
    actual->rho_dot = 0;
  }

  actual->theta_dot = ((real_t)sample->f.theta_dot) * GYRO_PER_LSB;  // Convert raw X-axis data to g
  if ( F_ABS(actual->theta_dot) < GYRO_DEAD_BAND )
  {
    actual->theta_dot = 0;
  }

  actual->phi_dot = ((real_t)sample->f.phi_dot) * GYRO_PER_LSB;      // Convert raw X-axis data to g
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
  int               address;
  uint8_t           buffer[128];
  spi_transaction_t transaction;

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
  }

  /*
   * Show the FIFO contents
   */
  memset(&transaction, 0, sizeof(transaction));     // Clear the transaction structure
  transaction.addr      = 0x80 | FIFO_DATA;         // Register address to read from
  transaction.length    = sizeof(buffer) * 8;       // Transmit length in bits
  transaction.tx_buffer = NULL;                  // Transmit buffer not used
  transaction.rxlength  = sizeof(buffer) * 8;       // Receive length in bits
  transaction.rx_buffer = &buffer;
  transaction.flags     = 0;                        // Indicate that this is a read operation
  spi_device_transmit(BMI270_handle, &transaction); // Transmit the transaction

  printf("\r\n");
  for ( int i = 0; i != sizeof(buffer);  i++ )
  {
    printf("%02X ", buffer[i]);
  }

  SEND(ALL, sprintf(_xs, _DONE_);)
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

  if ( transaction.rx_data[1] != 0x24 )             // Check the device ID
  {
    SEND(ALL, sprintf(_xs, "Failed to read BMI270 device ID: 0x%02X", transaction.rx_data[1]);)
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

  if ( transaction.rx_data[1] != 0x1 )              // Check the device ID
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Initialization failed: 0x%02X", transaction.rx_data[1]);))
  }
  else
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initialization successful: 0x%02X", transaction.rx_data[1]);))
  }
  /*
   * All done, return
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return transaction.rx_data[0]; // Return the device ID
}
