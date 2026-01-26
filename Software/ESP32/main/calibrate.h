/*
 * calibrate.h
 */
#ifndef _CALIBRATE_H
#define _CALIBRATE_H

/*
 * Functions
 */
void   calibrate(void);
double solve_spline(double theta); // Use the spline coeficients to find the scale factor
void   get_calibration_data(void); // Retrieve calibration data from NONVOL
void   calibration_test(void);     // Generate test calibration data
#endif
