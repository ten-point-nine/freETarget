
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
  int16_t    raw_x; // X acceleration read from sensor
  int16_t    raw_y; // Y acceleration read from sensor
  int16_t    raw_z; // Z acceleration read from sensor
  real_t ax;    // X-axis acceleration in g
  real_t ay;    // Y-axis acceleration in g
  real_t az;    // Z-axis acceleration in g
  real_t vx;    // Velocity in the X axis
  real_t vy;    // Velocity in the Y axis
  real_t vz;    // Velocity in the Z axis
  real_t x;     // X position
  real_t y;     // Y position
  real_t z;     // Z position
} ADXL345_sample_t;

/*
 *  Functions
 */
void ADXL345_init(void);                               // Initialize the ADXL345
void ADXL345_read_raw_accel(ADXL345_sample_t *sample); // Read the accelermeter
void ADXL345_test(void);                               // Test the ADXL345
void ADXL345_find_zero(void);                          // Take a zero sample to use for future adjustments
void ADXL345_adjust_zero(ADXL345_sample_t *sample);    // Adjust a sample by subtracting the zero sample
void ADXL345_convert_to_g(ADXL345_sample_t *sample);   // Convert raw acceleration data to g
