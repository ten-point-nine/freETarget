
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
void ADXL345_init(void);
void ADXL345_write(real_t volts[]);
void ADXL345_read(void);
void ADXL345_calibrate(void);