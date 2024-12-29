/*
 * compute_hit.h
 */
#ifndef _COMPUTE_HIT_H
#define _COMPUTE_HIT_H

/*
 * What score items will be included in the JSON
 */
#define S_SHOT   true  // Include the shot number
#define S_XY     true  // Include X-Y coordinates
#define S_POLAR  false // Include polar coordinates
#define S_TIMERS true  // Include counter values
#define S_MISC   true  // Include miscelaneous diagnotics
#define S_SCORE  false // Include estimated score

/*
 *  Local Structures
 */

extern sensor_t s[4];

/*
 *  Public Funcitons
 */
void         init_sensors(void);                                              // Initialize sensor structure
unsigned int compute_hit(shot_record_t *shot);                                // Find the location of the shot
void         send_score(shot_record_t *shot, unsigned int shot_number);       // Send the shot
void         send_replay(shot_record_t *shot, unsigned int shot_number);      // Replay an earlier shot
void         send_miss(shot_record_t *shot, unsigned int shot_number);        // Send a miss message
void         rotate_hit(unsigned int location, shot_record_t *shot);          // Rotate the shot back into the correct quadrant
bool         find_xy_3D(sensor_t *s, double estimate, double z_offset_clock); // Estimated position including slant range
double       speed_of_sound(double temperature, double relative_humidity);    // Speed of sound in mm/us
double       sq(double x);                                                    // Square function
#endif
