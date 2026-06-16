
/******************************************************************************
 *
 * @file: BMI270.h
 *
 * Common interface to the Bosch BMI270 3-axis accelerometer.
 *
 *****************************************************************************
 *
 * See: https://www.bosch-sensortec.com/products/motion-sensors/bmi270/
 *
 *****************************************************************************/

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

/*
 * A single sample frame as read from the FIFO
 */
typedef struct      // A single raw frame as read from the FIFO
{
  int16_t rho_dot;
  int16_t theta_dot;
  int16_t phi_dot;  // Z axis rotation speed
  int16_t x_dotdot; // Sample frame from BMI270
  int16_t y_dotdot;
  int16_t z_dotdot;
} FIFO_raw_frame_t; // Value read from sensor

/*
 * A large buffer to hold an entire WATERMARK of samples
 */
typedef struct            // Large buffer to hold all the FIFO data
{
  uint8_t          empty; // Here to force uint16_t x to be on a word boundary
  uint8_t          dummy; // Dummy byte as read from the SPI bus
  FIFO_raw_frame_t f[RAW_FRAME_COUNT];
} FIFO_raw_t;             // Value read from sensor via FIFO

/*
 * Pointers to access structures
 */
typedef struct
{
  int16_t inner; // Inner pointer
  int16_t outer; // Outer pointer
} trace_index_t;

/*
 *  Functions
 */
void BMI270_init(unsigned int bmi270_gpio);                                 // Initialize the BMI270
void BMI270_read_raw_accel(FIFO_raw_frame_t *sample);                       // Read the accelermeter
bool BMI270_pull_FIFO(void);                                                // Read all of the samples in the FIFO
void BMI270_test(void);                                                     // Test the BMI270
void BMI270_find_zero(bool automatic_confirm);                              // Take a zero sample to use for future adjustments
void BMI270_convert_to_g(FIFO_raw_frame_t *sample, trace_vector_t *actual); // Convert the raw sample to a vector
void BMI270_oscilliscope(void);                                             // Poor man's oscilliscope
void BMI270_FIFO_read(void);                                                // FIFO handler
void BMI270_SPI_dump(void);                                                 // Dump the BMI270 registers using SPI.
bool BMI270_get_next_raw_sample(FIFO_raw_frame_t *sample);                  // Pull out the next sample
bool BMI270_find_index_out(time_count_64_t shot);                           // Set the starting point in the list