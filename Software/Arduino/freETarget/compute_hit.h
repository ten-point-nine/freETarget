/*
 * compute_hit.h
 */
#ifndef _COMPUTE_HIT_H
#define _COMPUTE_HIT_H

/*
 * What score items will be included in the JSON
 */
#define S_SHOT      true        // Include the shot number
#define S_XY        true        // Include X-Y coordinates
#define S_POLAR     false       // Include polar coordinates
#define S_TIMERS    true        // Include counter values
#define S_MISC      true        // Include miscelaneous diagnotics
#define S_SCORE     false       // Include estimated score

/*
 *  Local Structures
 */
typedef struct sensor_t
{
  unsigned int index;   // Which sensor is this one
  bool   is_valid;      // TRUE if the sensor contains a valid time
  double angle_A;       // Angle to be computed
  double diagonal;      // Diagonal angle to next sensor (45')
  double x_tick;        // Sensor Location (X in clock ticks)
  double y_tick;        // Sensor Location (Y in clock ticks)
  double xr_tick;       // Sensor Location after rotation (X in clock ticks)
  double yr_tick;       // Sensor Location after rotation (Y in clock ticks)
  double xphys_mm;      // Physical Sensor location X (in mm)
  double yphys_mm;      // Pyysical sensor location Y (in mm)
  double count;         // Working timer value as read from counter
  double doppler;       // Correction for doppler
  double a, b, c;       // Working dimensions
  double xshot_mm;      // Computed X shot value (in mm)
  double yshot_mm;      // Computed Y shot value (in mm)
} sensor_t;

/*
 *  Public Funcitons
 */
void init_sensors(void);                                      // Initialize sensor structure
unsigned int compute_hit(shot_record_t* shot);                // Find the location of the shot
void send_score(shot_record_t* shot);                         // Send the shot
void rotate_hit(unsigned int location, shot_record_t* shot);  // Rotate the shot back into the correct quadrant 
bool find_xy_3D(sensor_t* sensor, double estimate, double z_offset_clock);  // Estimated position including slant range
void send_timer(int sensor_status);                           // Show debugging information 
void send_miss(shot_record_t* shot);                          // Send a miss message
double speed_of_sound(double temperature, int relative_humidity); // Speed of sound in mm/us

#endif
