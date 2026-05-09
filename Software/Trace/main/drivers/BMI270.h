
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

/*
 *  Functions
 */
void         BMI270_init(unsigned int bmi270_gpio);                           // Initialize the BMI270
unsigned int BMI270_read_raw_accel(trace_raw_t *sample, bool zero_offset);    // Read the accelermeter
unsigned int BMI270_read_FIFO_accel(void);                                    // Read all of the samples in the FIFO
void         BMI270_test(void);                                               // Test the BMI270
void         BMI270_find_zero(void);                                          // Take a zero sample to use for future adjustments
void         BMI270_convert_to_g(trace_raw_t *sample, trace_point_t *actual); // Convert raw acceleration data to g
void         BMI270_oscilliscope(void);                                       // Poor man's oscilliscope
void         BMI270_FIFO_read(void);                                          // FIFO handler
unsigned int BMI270_find_sample_out(unsigned int sample_count);               // Find the starting point in the sample buffer
