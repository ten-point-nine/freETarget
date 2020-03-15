/*----------------------------------------------------------------
 *
 * Compute_hit
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"



#define THRESHOLD 1

#define PI_ON_4 (atan(1))
#define PI_ON_2 (2.0 * PI_ON_4)
/*
 *  Variables
 */
struct sensor
{
  double angle_P;       // Left Angle
  double count;         // Counter value
  double x;             // Sensor Location (X)
  double y;             // Sensor Location (Y)
  double cx;            // Coputed cx calue
  double cy;            // Computed cy value
  double angle_offset;  // How much the angle is rotated
};


struct sensor s[4];

  
/*----------------------------------------------------------------
 *
 * double speed_of_sound(double temperature)
 *
 * Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/

double speed_of_sound(double temperature)
{
  return (331.3 + 0.606 * temperature) * 1000.0 / 1000000.0; 
}

/*----------------------------------------------------------------
 *
 * void init_sensors()
 *
 *Setup the constants in the strucure
 *
 *----------------------------------------------------------------
 *
 * 
 *--------------------------------------------------------------*/
void init_sensors(void)
{

  s[N].x = 0;
  s[N].y = RADIUS;
  s[N].angle_offset = atan2(s[N].x, s[N].y) + PI_ON_4;

  s[E].x = RADIUS;
  s[E].y = 0;
  s[E].angle_offset =  s[N].angle_offset + PI_ON_2;

  s[S].x = 0;
  s[S].y = -RADIUS;
  s[S].angle_offset = s[E].angle_offset + PI_ON_2;

  s[W].x = -RADIUS;
  s[W].y = 0;
  s[W].angle_offset = s[S].angle_offset + PI_ON_2;

  Serial.print("\n\rSensors Ready");
  return;
}
/*----------------------------------------------------------------
 *
 * void compute_hit()
 *
 * determine the location of the hit
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 *--------------------------------------------------------------*/

void compute_hit(double* ptr_x, double* ptr_y)
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  unsigned int  timer_value[4];    // Array of timer values
  int           i, j;
  double        estimate;          // Estimated position
  double        error;             // Location error
  double        r1, r2;            // Distance between points
  double        x, y;              // Computed location
  
/*
 *  Read in the counter values 
 */
 timer_value[N] = read_counter(N);
 timer_value[E] = read_counter(E);
 timer_value[S] = read_counter(S);
 timer_value[W] = read_counter(W);

 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rNorth: 0x"); Serial.print(timer_value[N], HEX); Serial.print("  ms:"); Serial.print(timer_value[N] / OSCILLATOR_MHZ);
   Serial.print("  East: 0x");    Serial.print(timer_value[E], HEX); Serial.print("  ms:"); Serial.print(timer_value[E] / OSCILLATOR_MHZ);
   Serial.print("  South: 0x");   Serial.print(timer_value[S], HEX); Serial.print("  ms:"); Serial.print(timer_value[S] / OSCILLATOR_MHZ);
   Serial.print("  West: 0x");    Serial.print(timer_value[W], HEX); Serial.print("  ms:"); Serial.print(timer_value[W] / OSCILLATOR_MHZ);
   Serial.println();
   }
/*
 * Determine the location of the reference counter (longest time)
 */
  reference = timer_value[N];
  location = N;
  for (i=1; i != 4; i++);
  {
    if ( timer_value[i] > reference )
    {
      reference = timer_value[i];;
      location = i;
    }
  }

/*
 * Correct the time to remove the shortest distance
 */
  for (i=0; i != 4; i++)
  {
    s[i].count = timer_value[location] - reference;
  }

/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  estimate = 0;                 // Start at 0 radius
  error = 999999;               // Start with a big error

  while (error > THRESHOLD )
  {
    for (i=0; i != 4; i++)
    {
      find_cxcy(i, location, estimate);         // Find the location
    }
    r1 = sqrt(sq(s[N].cx - s[S].cx) + sq(s[N].cy - s[S].cy)); // Distance 
    r2 = sqrt(sq(s[E].cx - s[E].cx) + sq(s[W].cy - s[W].cy)); // Distance
    error = min(r1, r2);
    estimate = estimate + error;
  }

/*
 *  We have the four computed locations, average and that's the hit
 */
  x = 0;
  y = 0;
  for (i=0; i != 4; i++ )
  {
    x += s[i].cx;
    y += s[i].cy;
  }
  *ptr_x /= 4.0;
  *ptr_y /= 4.0;
  
  /*
   * All done return
   */

  return;
}
/*----------------------------------------------------------------
 *
 * find_cxcy
 *
 * Calaculate where the shot seems to lie
 *
 *----------------------------------------------------------------

 *  
 *--------------------------------------------------------------*/

void find_cxcy
    (
     unsigned int i,        // Index to be operatated on
     unsigned int location, // Location of shortest sensor
     double   estimate      // Estimated location of shot
     )
{
  s[i].angle_P = acos( sq(estimate) - K_SQUARED/(2.0*(s[i].count + estimate) * K));
  s[i].cx = s[i].x + (s[i].count + estimate) * cos(s[i].angle_P + s[i].angle_offset);
  s[i].cy = s[i].y + (s[i].count + estimate) * sin(s[i].angle_P + s[i].angle_offset);
  
  return;
}



/*----------------------------------------------------------------
 *
 * void send_score(void)
 *
 * Send the score out over the serial port
 *
 *----------------------------------------------------------------
 * 
 * The score is sent as:
 * 
 * {"id":"freETarget", "value":{shot, x, y}}
 *    
 *--------------------------------------------------------------*/

void send_score
  (
  unsigned int shot,          // Current shot number
           double x_time,     // X location of shot
           double y_time      // Y location of shot
  )
{
  int x, y;                   // Shot location in mm X, Y

  x = x_time * speed_of_sound(23.0) * CLOCK_PERIOD;
  y = y_time * speed_of_sound(23.0) * CLOCK_PERIOD;

  Serial.print("{\"shot\":"); Serial.print(shot);
  Serial.print(", \"x\":");     Serial.print(x); 
  Serial.print(", \"y\":");     Serial.print(y); Serial.print("}");
  Serial.println();

  return;
}
