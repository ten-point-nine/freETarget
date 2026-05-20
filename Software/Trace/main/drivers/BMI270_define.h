/******************************************************************************
 *
 * file: BMI270_define.h
 *
 * Definition file for BMI270 fixed definitions
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
#ifndef _BMI_define_h_
#define _BMI_define_h_

/*
 * Definitions
 */

#define G2        (0)                                    // Select +/- 2g range
#define G2_RANGE  (4.0)
#define G4        1                                      // Select +/- 4g range
#define G4_RANGE  (8.0)
#define G8        2                                      // Select +/- 8g range
#define G8_RANGE  (16.0)
#define G16       3                                      // Select +/- 16g range
#define G16_RANGE (32.0)

#define G_RANGE   G2
#define G_PER_LSB (G2_RANGE / 65535.0)                   // g per LSB for the selected range

#define GYRO_2000       0                                // Select +/- 2000 degrees per second
#define GYRO_RANGE_2000 (4000)                           //
#define GYRO_1000       1                                // Select +/- 1000 degrees per second
#define GYRO_RANGE_1000 (2000)                           //
#define GYRO_500        2                                // Select +/- 500 degrees per second
#define GYRO_RANGE_500  (1000)                           //
#define GYRO_250        3                                // Select +/- 250 degrees per second
#define GYRO_RANGE_259  (500)                            //
#define GYRO_125        4                                // Select +/- 125 degrees per second
#define GYRO_RANGE_125  (250)                            //

#define GYRO_RANGE   GYRO_125
#define GYRO_PER_LSB (GYRO_RANGE / 65535.0 * PI / 180.0) // in Radians per LSB

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

#define ACC_RANGE     0x41             // Acceleration range
#define acc_range_2g  0x00             // +/- 2g
#define acc_range_4g  0x01             // +/- 4g
#define acc_range_8g  0x02             // +/- 8g
#define acc_range_16g 0x02             // +/- 16g

#define GYR_CONF        0x42           // Gyro Configuration
#define gyr_odr         0x09           // (odr_200)
#define gyr_bwp         (0x2 << 4)     // (norm) Bandwidth coefficient
#define gyr_noise_perf  (1 << 6)       // (hp) performance optiminzed
#define gyr_filter_perf (1 << 7)       // (hp) performance optimized

#define GYR_RANGE 0x43                 // Gyro Range
#define gyr_range 0x04                 // (range_125) +/- 125 dps
#define ois_range (0x00 << 3)          // (range_250) Full scale resolution +/- 250 dps

#define FIFO_DOWNS         0x45        // FIFO downsampling
#define gyr_fifo_downs     (0x00 << 0) // Downsampling for gyro, 2**gyr_fifo_downs
#define gyr_fifo_filt_data (0x01 << 3) // Filtered data
#define acc_fifo_downs     (0x00 << 4) // Downsampling for accel, 2**acc_fifo_downs
#define acc_fifo_filt_data (0x01 << 7) // Filtered data

#define FIFO_WTM_0 0x46                // FIFO Watermark lsb
#define FIFO_WTM_1 0x47                // FIFO Watermark msb
#define watermark  400                 // Interrupt after 400 samples

#define FIFO_CONFIG_0     0x48         // FIFO Frame Configuration
#define fifo_stop_on_full (0x00 << 0)  // (disable) do not stop on full
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
#define adv_power_save    (0x01 << 0)  // Advanced power save enabled
#define fifo_self_wake_up (0x01 << 1)  // (fsw_on) FIFO enabled in low power mode
#define fup_en            (0x00 << 2)  // (fup_on) Fast power up enabled

#define PWR_CTRL 0x7D                  // Power mode control register
#define aux_en   (0x00 << 0)           // (aux_off) disable aux sensor
#define gyr_en   (0x01 << 1)           // (gyr_on) gyro enabled
#define acc_en   (0x01 << 2)           // (acc_on) Acclerometer enabled
#define temp_en  (0x00 << 3)           // (temp_off) Temperature disabled

#define CMD        0x7E                // Command register
#define fifo_flush (0xb0)              // Start the FIFO

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
    .clock_speed_hz   = 1 * 1000 * 1000,                              // 2 MHz clock speed (do not set higher than 2 MHz)
    .input_delay_ns   = 0,                                            // No input delay
    .spics_io_num     = BMI270_CS,                                    // CS pin
    .flags            = SPI_DEVICE_NO_DUMMY,                          // No special flags
    .queue_size       = 1,
    .pre_cb           = NULL,                                         // Callback to be called before a transmission is started.
    .post_cb          = NULL                                          // Callback to be called after a transmission has completed.
};

static BMI270_config_t BMI270_config[] = {
    {ACC_CONF,      acc_odr | acc_bwp | acc_filter_perf                                      },
    {ACC_RANGE,     acc_range_2g                                                             }, // ACC_RANGE +/-2g
    {PWR_CONF,      adv_power_save | fifo_self_wake_up | fup_en                              }, //
    {GYR_CONF,      gyr_odr | gyr_bwp | gyr_noise_perf | gyr_filter_perf                     }, //  GYR_CONF, Performance Optimized,    Average 4, 800 samples
    {GYR_RANGE,     gyr_range | ois_range                                                    }, //    GYR_RANGE, 125 dps
    {FIFO_DOWNS,    gyr_fifo_downs | gyr_fifo_filt_data | acc_fifo_downs | acc_fifo_filt_data}, // FIFO_DOWNS
    {FIFO_WTM_0,    watermark & 0x00ff                                                       }, // FIFO Watermark lsb
    {FIFO_WTM_1,    (watermark >> 8) & 0x00ff                                                }, // FIFO Watermark msb
    {FIFO_CONFIG_0, fifo_stop_on_full | fifo_time_en                                         }, // FIFO_CONFIG_0, No timestamp, do
    {FIFO_CONFIG_1, fifo_tag_int_1_en | fifo_tag_int_2_en | fifo_header_en | fifo_aux_en | fifo_acc_en |
                        fifo_gyr_en                                       }, // FIFO_CONFIG_1, Store gyro and accel data
    {INT_1_IO_CTRL, lvl | od | output_en | input_en                                          }, // INT_1_IO_CTRL Interrupt 1 used
    {INT_2_IO_CTRL, int_2_not_used                                                           }, // INT_2_IO_CTRL, not used
    {INT_LATCH,     int_latch                                                                }, // INT_LATCH, latched
    {INT_MAP_DATA,  fwm_int1                                                                 }, // INT1_MAP_DATA, FIFO watermark  interrupt mapped to INT1
    {PWR_CTRL,      aux_en | gyr_en | acc_en | temp_en                                       }, // PWR_CTRL,    Enable Gyro and Accel, disable Aux and temperature
    {CMD,           fifo_flush                                                               }, // CMD, clear the FIFO
    {0x00,          0x00                                                                     }  // End of the configuration file
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
    // 0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19    20    21
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

#endif