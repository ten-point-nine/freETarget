
/******************************************************************************
 *
 * @file: ICM45686.h
 *
 * Common interface to the Bosch ICM45686  3-axis accelerometer.
 *
 *****************************************************************************
 *
 * See: https://www.bosch-sensortec.com/products/motion-sensors/bmi270/
 *
 *****************************************************************************/
#ifndef ICM45686_H
#define ICM45686_H

#if USE_ICM45686

/*
 * Data as pulled in from the BMK270.
 * Note, the register order is LSB then MSB
 *
 * IMPORTANT
 *
 * The SPI driver will deliver the bytes in the order they come out of the ICM45686.
 * On the other hand, the ESP32 will word align any uint16_t entries in the structure
 * meaning that the alignment between what the program thinks is aligned and how they
 * arrive from the ICM45686 will be off by one byte.
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
 * ICM45686.
 *
 */

/*
 * A single sample frame as read from the FIFO
 */
typedef struct         // A single raw frame as read from the FIFO
{
  int8_t  header;      // Header byte for the FIFO frame, not used in this program
  int16_t x_dotdot;    // Sample frame from ICM45686
  int16_t y_dotdot;
  int16_t z_dotdot;
  int16_t rho_dot;
  int16_t theta_dot;
  int16_t phi_dot;     // Z axis rotation speed
  int8_t  temperature; // Temperature data from ICM45686
  int16_t timestamp;   // Timestamp for the sample, not used in this program
} FIFO_raw_frame_t;    // Value read from sensor

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
void ICM45686_init(unsigned int bmi270_gpio);                                 // Initialize the ICM45686
void ICM45686_read_raw_accel(FIFO_raw_frame_t *sample);                       // Read the accelermeter
bool ICM45686_pull_FIFO(void);                                                // Read all of the samples in the FIFO
void ICM45686_test(void);                                                     // Test the ICM45686
void ICM45686_find_zero(bool automatic_confirm);                              // Take a zero sample to use for future adjustments
void ICM45686_convert_to_g(FIFO_raw_frame_t *sample, trace_vector_t *actual); // Convert the raw sample to a vector
void ICM45686_oscilliscope(void);                                             // Poor man's oscilliscope
void ICM45686_FIFO_read(void);                                                // FIFO handler
void ICM45686_SPI_dump(void);                                                 // Dump the ICM45686 registers using SPI.
bool ICM45686_get_next_raw_sample(FIFO_raw_frame_t *sample);                  // Pull out the next sample
bool ICM45686_find_index_out(time_count_64_t shot);                           // Set the starting point in the list
void ICM45686_read_temperature(void);                                         // Read the temperature data from the ICM45686
#endif
#endif
