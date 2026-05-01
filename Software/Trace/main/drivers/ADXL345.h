
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
typedef struct
{
  int16_t raw_x; // X-axis acceleration data (16-bit signed)
  int16_t raw_y; // Y-axis acceleration data (16-bit signed)
  int16_t raw_z; // Z-axis acceleration data (16-bit signed)
  real_t  x;     // X-axis acceleration in g
  real_t  y;     // Y-axis acceleration in g
  real_t  z;     // Z-axis acceleration in g
} ADXL345_sample_t;

/*
 *  Functions
 */
void ADXL345_init(void); // Initialize the ADXL345
void ADXL345_read_raw_accel(ADXL345_sample_t *sample);
void ADXL345_test(void); // Test the ADXL345