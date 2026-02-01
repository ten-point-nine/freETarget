/*
 * calibrate.h
 */
#ifndef _CALIBRATE_H
#define _CALIBRATE_H

/*
 * Functions
 */
void   calibrate(void);
double solve_spline(double theta, bool valid); // Use the spline coeficients to find the scale factor
bool   get_calibration(void);                  // Retrieve calibration data from NONVOL
void   calibration_test(void);                 // Generate test calibration data

/*
 *  Variables
 */
extern bool calibration_is_valid;

#endif
