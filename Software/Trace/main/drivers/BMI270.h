
/******************************************************************************
 *
 * @file: BMI270.h
 *
 * Common interface to the Analog Devices BMI270 3-axis accelerometer.
 *
 *****************************************************************************
 *
 * See: https://www.analog.com/en/products/BMI270.html
 *
 *****************************************************************************/

/*
 *  Definitions
 */
#define FIFO_READ 100 // Read 100 samples every time

/*
 * Data as pulled in from the BMK270.
 * Note, the register order is LSB then MSB
 *
 * IMPORTANT
 *
 * The SPI driver will deliver the bytes in the order they come out of the BMI270.
 * On the other hand, the ESP32 will word align any uint16_t entries in the structure
 * meaning that the alignment between what the program thinks is aligned and how they
 * arrive from the BMI270 will be off by one byte.
 *
 * For example,    typedef struct {uint8_t dummy; uint16_t sample}
 * will have an empty byte inserted between dummy and sample so that sample is word
 * aligned.
 *
 * Any attempt to use sample will result in a mangled value.
 *
 * To get around this issue, an empty byte is added at the beginning of the
 * structure so that dummy is byte aligned, and uint16_t x becomes word aligned
 * and can be accessed in the correct little-big endian that is provided by the
 * BMI270.
 *
 */

typedef struct
{
  int16_t  x_dotdot;       // Sample frame from BMI270
  int16_t  y_dotdot;
  int16_t  z_dotdot;
  int16_t  rho_dot;
  int16_t  theta_dot;
  uint16_t phi_dot;        // Z axis rotation speed
} trace_raw_frame_t;       // Value read from sensor

typedef struct
{                          // Single read directly from the BMI270
  uint8_t           empty; // Here to force uint16_t x to be on a word boundary
  uint8_t           dummy; // Dummy byte as read from the SPI bus
  trace_raw_frame_t f;     // Single frame
} trace_raw_t;             // Value read from sensor

typedef struct
{
  uint8_t           empty; // Here to force uint16_t x to be on a word boundary
  uint8_t           dummy; // Dummy byte as read from the SPI bus
  trace_raw_frame_t f[FIFO_READ];
} trace_FIFO_read_t;       // Value read from sensor via FIFO

/*
 *  Functions
 */
void         BMI270_init(unsigned int bmi270_gpio);      // Initialize the BMI270
unsigned int BMI270_device_ID(void);                     // Read the device ID to confirm that we can communicate with the device
unsigned int BMI270_device_status(void);                 // Read the device status to confirm that the device is working
void         BMI270_read_raw_accel(trace_raw_t *sample); // Read the accelermeter
unsigned int BMI270_pull_FIFO(void);                     // Read all of the samples in the FIFO
void         BMI270_test(void);                          // Test the BMI270
void         BMI270_find_zero(void);                     // Take a zero sample to use for future adjustments
void         BMI270_convert_to_g(trace_raw_t *sample, trace_point_t *actual); // Convert from bits t g
void         BMI270_oscilliscope(void);                                       // Poor man's oscilliscope
void         BMI270_FIFO_read(void);                                          // FIFO handler
unsigned int BMI270_find_sample_out(unsigned int sample_count);               // Find the starting point in the sample buffer
void         BMI270_SPI_test(void); // Send a command to the BMI270 and read the response using SPI.
void         BMI270_SPI_dump(void); // Dump the BMI270 registers using SPI.