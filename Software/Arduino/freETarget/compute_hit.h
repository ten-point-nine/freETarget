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
struct sensor
{
  unsigned int index;   // Which sensor is this one
  bool   is_valid;      // TRUE if the sensor contains a valid time
  double angle_A;       // Angle to be computed
  double diagonal;      // Diagonal angle to next sensor (45')
  double x;             // Sensor Location (X us)
  double y;             // Sensor Location (Y us)
  double count;         // Working timer value
  double a, b, c;       // Working dimensions
  double xs;            // Computed X shot value
  double ys;            // Computed Y shot value
};

typedef struct sensor sensor_t;



/*
 *  Public Funcitons
 */
void init_sensors(void);                                      // Initialize sensor structure
unsigned int compute_hit(shot_record_t* shot, bool test_mode);// Find the location of the shot
void send_score(shot_record_t* shot);                         // Send the shot
void rotate_hit(unsigned int location, shot_record_t* shot);  // Rotate the shot back into the correct quadrant 
bool find_xy_3D(sensor_t* s, double estimate, double z_offset_clock);  // Estimated position including slant range
void send_timer(int sensor_status);                           // Show debugging information 
void send_miss(shot_record_t* shot);                          // Send a miss message
double speed_of_sound(double temperature, int relative_humidity); // Speed of sound in mm/us

#endif
