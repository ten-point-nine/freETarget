
/******************************************************************************
 *
 * @file: ADXL345.h
 *
 * Common interface to the Analog Devices ADXL345 3-axis accelerometer.
 *
 *****************************************************************************
 *
 * See: https://www.analog.com/en/products/adxl345.html
 *
 *****************************************************************************/

/*
 *  Definitions
 */

/*
 *  Functions
 */
void         ADXL345_init(void);                             // Initialize the ADXL345
unsigned int ADXL345_read_raw_accel(accel_sample_t *sample); // Read the accelermeter
unsigned int ADXL345_read_FIFO_accel(void);                  // Read all of the samples in the FIFO
void         ADXL345_test(void);                             // Test the ADXL345
void         ADXL345_find_zero(void);                        // Take a zero sample to use for future adjustments
void         ADXL345_adjust_zero(accel_sample_t *sample);    // Adjust a sample by subtracting the zero sample
void         ADXL345_convert_to_g(accel_sample_t *sample);   // Convert raw acceleration data to g
void         ADXL345_oscilliscope(void);                        // Poor man's oscilliscope