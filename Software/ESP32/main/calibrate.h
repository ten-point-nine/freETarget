/*
 * calibrate.h
 */
#ifndef _CALIBRATE_H
#define _CALIBRATE_H

/*
 * Functions
 */
void   calibrate(int action);
real_t solve_spline_for_angle(double theta, bool valid); // Use the spline coeficients to find the angular correction
real_t solve_spline_for_scale(real_t angle, bool valid); // Angle to compute scaling factor
bool   get_target_calibration(void);                     // Retrieve calibration data from NONVOL
void   calibration_test(void);                           // Generate test calibration data

/*
 *  Variables
 */
extern bool calibration_is_valid;

#endif
