/******************************************************************************
 *
 * file: ICM45686-define.h
 *
 * Definition file for ICM45686  fixed definitions
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
 * IMPORTANT
 *
 * This driver polls the FIFO to remove samples when the watermark is met.
 * The interrupt is not used to trigger the reading of the FIFO.  This is because
 * the I2C driver does not work with interrupts.
 *
 *****************************************************************************/
#ifndef _ICM_DEFINE_H_
#define _ICM_DEFINE_H_

#if USE_ICM45686

/*-----------------------------------------------------------------------------
 *
 * ICM45686 Register Addresses
 */
#define ACCEL_X 0x00                     // Acceleration X, Y, Z  Gyro X, Y, Z

#define TEMP_DATA1_UI 0x0C               // Temperature data, High byte
#define TEMP_DATA0_UI 0x0D               // Temperature data, Low byte

#define PWR_MGMT0  0x10                  // Power Management 0
#define gyro_mode  (3 << 2)              // Gyro mode, 0 = off, 1 = standby, 2 = low power, 3 = low noise
#define accel_mode (3 << 0)              // Accel mode, 0 = off, 1 = standby, 2 = low power, 3 = low noise

#define FIFO_COUNT_0            0x12     // LSB of FIFO length
#define FIFO_COUNT_1            0x13     // MSB of FIFO length
#define FIFO_DATA               0x14     // FIFO data register
#define INT1_CONFIG_0           0x16     // Interrupt 1 configuration
#define int1_status_en_fifo_ths (1 << 1) // Enable FIFO threshold interrupt

#define INT1_CONFIG_2 0x18               // Interrupt 1 configuration 2
#define int1_drive    (0 << 2)           // Drive mode, 0 = push-pull, 1 = open drain
#define int1_mode     (1 << 1)           // Interrupt mode, 0 = pulsed, 1 = latched
#define int1_polarity (0 << 0)           // Interrupt polarity, 0 = active low, 1 = active high

#define INT1_STATUS0         0x19        // Interrupt 1 status 0
#define int1_status_fifo_ths (1 << 1)    // FIFO threshold interrupt status

#define ACCEL_CONFIG0   0x1B             // Acceleration Configuration
#define accel_ui_fs_sel (4 << 4)         // Full Scale Selection, 0 = 32g, 1 = 16, 2 = 8, 3 = 4, 4 = 2
#define accel_odr       (5 << 0)         // Output Data Rate 5 = 1600 Hz

#define GYRO_CONFIG0 0x1C                // Gyro Configuration
#define gyro_ui_fs_sel                                                                                                                     \
  (7 << 4) // Full Scale Selection, 0 = 4000 dps, 1 = 2000 dps, 2 = 1000, 3 = 500, 4 = 250, 5 = 125, 6 = 62.5, 7 = 31.25, 8 = 15.62
#define gyro_odr                                                                                                                           \
  (5 << 0) // Output Data Rate, 3 = 6.4k, 4 = 3.2k, 5 = 1.6k, 6 = 800, 7 = 400, 8 = 200, 9 = 100, A = 50, B = 25, C = 12.5, D = 6.25, E
           // = 3.12, F = 1.56

#define FIFO_CONFIG0 0x1D             // FIFO Configuration
#define fifo_mode    (1 << 6)         // Stream mode, 0 = disabled, 1 = overwrite old data when full, 2 = stop when full
#define fifo_depth   (0x1F << 0)      // 1F = 8k bytes

#define FIFO_CONFIG1_0 0x1E           // Watermark low
#define FIFO_CONFIG1_1 0x1F           // Watermark high

#define FIFO_CONFIG2     0x20         // FIFO Configuration 2
#define fifo_flush       (1 << 7)     // Flush FIFO when set to 1
#define fifo_wr_wm_gt_th (1 << 3)     // Set the FIFO watermark interrupt when the FIFO count is greater than the FIFO_WTM registers

#define FIFO_CONFIG3  0x21            // FIFO Configuration 3
#define fifo_gyro_en  (1 << 2)        // Gyro data enabled in FIFO
#define fifo_accel_en (1 << 1)        // Accel data enabled in FIFO

#define FIFO_CONFIG4 0x22             // FIFO Configuration 4

#define ODR_DECIMATE_CONFIG 0x28      // Decimation configuration for the FIFO
#define gyro_fifo_odr_dec   (0 << 4)  // Decimation for gyro, 0 = no decimation, 1 = 2, 2 = 4, 3 = 8, 4 = 16
#define accel_fifo_odr_dec  (0 << 0)  // Decimation for accel,

#define SREG_CTRL            0x67     // System Register Control.
#define sreg_data_endian_sel (0 << 1) // Data Endian Selection (only applies to FIFO data), 0 = little endian, 1 = big endian

#define WHO_AM_I        0x72          // Chip ID register
#define WHO_I_SHOULD_BE 0xE9          // Expected device ID

/*
 * Definitions
 */
/*
 *  Understanding the ICM45686 settings
 *
 * odr - Output Data Rate       This is the rate at which samples are generated at the output point
 * bwp - Bandwidth Coefficient  This is a hardware averaging that is applied BEFORE the odr
 * performance optimized        Trading sample rate for power consumption
 * FIFO downsample              How many samples are thrown away before putting into the FIFO  expresed as 2**n
 */

#define GYRO_PER_LSB ((15.0 - (-15.0)) / 65565.0) // Use 2G for the range
#define G_PER_LSB    ((2.0 - (-2.0)) / 65565.0)

/*
 *  Typedefs
 */
typedef struct
{
  int address; // Register being accessed
  int value;   // Value to be written to the register
} ICM45686_config_t;

/*
 * Function Prototypes
 */
#define DEBUG                                                                                                                              \
  printf("\r\nrx_data: %02X %02X %02X %02X", transaction.rx_data[0], transaction.rx_data[1], transaction.rx_data[2],                       \
         transaction.rx_data[3]);

/*
 * Variables
 */
static spi_device_handle_t ICM45686_handle; // Handle for the SPI device

static spi_device_interface_config_t ICM45686_spi_config = {
    // Configuration for the SPI device
    .command_bits     = 0,                   // No command phase
    .address_bits     = 8,                   //
    .dummy_bits       = 8,                   // No dummy bits
    .mode             = 0,                   // SPI mode 0
    .clock_source     = SPI_CLK_SRC_DEFAULT, // Use default clock source
    .duty_cycle_pos   = 128,                 // 50% duty cycle
    .cs_ena_pretrans  = 0,                   // No pre-transaction CS activation
    .cs_ena_posttrans = 0,                   // No post-transaction CS activation
    .clock_speed_hz   = 20 * 1000 * 1000,    // 2 MHz clock speed (do not set higher than 2 MHz)
    .input_delay_ns   = 0,                   // No input delay
    .spics_io_num     = ICM45686_CS,         // CS pin
    .flags            = SPI_DEVICE_NO_DUMMY, // No special flags
    .queue_size       = 1,
    .pre_cb           = NULL,                // Callback to be called before a transmission is started.
    .post_cb          = NULL                 // Callback to be called after a transmission has completed.
};

static const ICM45686_config_t ICM45686_config[] = {
    {SREG_CTRL,           sreg_data_endian_sel                  }, // Data Endian Selection, 0 = little endian, 1 = big endian
    {INT1_CONFIG_0,       int1_status_en_fifo_ths               },
    {INT1_CONFIG_2,       int1_drive | int1_mode | int1_polarity},
    {ACCEL_CONFIG0,       accel_ui_fs_sel | accel_odr           },
    {GYRO_CONFIG0,        gyro_ui_fs_sel | gyro_odr             },
    {FIFO_CONFIG0,        fifo_mode | fifo_depth                },
    {FIFO_CONFIG1_0,      WATERMARK & 0x00ff                    },
    {FIFO_CONFIG1_1,      (WATERMARK >> 8) & 0x00ff             },
    {FIFO_CONFIG2,        fifo_flush | fifo_wr_wm_gt_th         }, // FIFO Configuration 2
    {FIFO_CONFIG3,        fifo_gyro_en | fifo_accel_en          }, // FIFO Configuration 3
    {FIFO_CONFIG4,        0                                     },
    {ODR_DECIMATE_CONFIG, gyro_fifo_odr_dec | accel_fifo_odr_dec},
    {PWR_MGMT0,           gyro_mode | accel_mode                }, // Gyro and Accel mode
    {0,                   0                                     }
};

#endif
#endif