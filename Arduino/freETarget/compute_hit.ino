/*----------------------------------------------------------------
 *
 * Compute_hit
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"
#include "compute_hit.h"
#include "freETarget.h"
#include "compute_hit.h"
#include "analog_io.h"

#define THRESHOLD (0.001)

#define PI_ON_4 (3.14159269d / 4.0d)
#define PI_ON_2 (3.14159269d / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */

sensor_t s[4];

unsigned int bit_mask[] = {0x01, 0x02, 0x04, 0x08};
unsigned int timer_value[4];    // Array of timer values
  
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
  double speed;

  speed = (331.3d + 0.606d * temperature) * 1000.0d / 1000000.0d; 
  
  if ( read_DIP() & VERBOSE_TRACE )
    {
    Serial.print("\n\rSpeed of sound: "); Serial.print(speed); Serial.print("mm/us");
    }

  return speed;  
}

/*----------------------------------------------------------------
 *
 * void init_sensors()
 *
 *Setup the constants in the strucure
 *
 *----------------------------------------------------------------
 *
 *                             N     (+,+)
 *                             
 *                             
 *                      W      0--R-->E
 *
 *
 *               (-,-)         S 
 * 
 *--------------------------------------------------------------*/
void init_sensors(void)
{
  double s_speed;         // Speed of sound
  double temperature;     // Temperature in C

  s_speed = speed_of_sound(temperature_C());
  
  s[N].index = N;
  s[N].x = 0;
  s[N].y = RADIUS / s_speed * OSCILLATOR_MHZ;

  s[E].index = E;
  s[E].x = s[N].y;
  s[E].y = 0;

  s[S].index = S;
  s[S].x = 0;
  s[S].y = -s[N].y;

  s[W].index = W;
  s[W].x = -s[E].x;
  s[W].y = 0;

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

unsigned int compute_hit
  (
  unsigned int shot,                // Shot being processed
  history_t* h                      // Storing the results
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           i, j;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        r1, r2;            // Distance between points
  double        x, y;              // Computed location
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  unsigned int  sensor_status;     // Sensor collection status
  double        length_c;          // lenght of side C

/*
 * Compute the length of the outer box in clock cycles
 */
  s_of_sound = speed_of_sound(temperature_C());
  length_c = sqrt(2.0d) * RADIUS / s_of_sound * OSCILLATOR_MHZ;
  
/*
 *  Read in the counter values 
 */
 for (i=N; i <= W; i++)
 {
   timer_value[i] = read_counter(i);
 }

#if SAMPLE_CALCULATIONS
void sample_calculations (void);
  sensor_status = 0x0F;
  sample_calculations();
#endif

 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rNorth: 0x"); Serial.print(timer_value[N], HEX); Serial.print("  us:"); Serial.print(timer_value[N] / OSCILLATOR_MHZ);
   Serial.print("  East: 0x");    Serial.print(timer_value[E], HEX); Serial.print("  us:"); Serial.print(timer_value[E] / OSCILLATOR_MHZ);
   Serial.print("  South: 0x");   Serial.print(timer_value[S], HEX); Serial.print("  us:"); Serial.print(timer_value[S] / OSCILLATOR_MHZ);
   Serial.print("  West: 0x");    Serial.print(timer_value[W], HEX); Serial.print("  us:"); Serial.print(timer_value[W] / OSCILLATOR_MHZ);
   }
   
/*
 * Determine the location of the reference counter (longest time)
 */
  reference = timer_value[N];
  location = N;
  for (i=E; i <= W; i++)
  {
    if ( timer_value[i] > reference )
    {
      reference = timer_value[i];
      location = i;
    }
  }
  
 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rreference: "); Serial.print(reference); Serial.print("  location:"); Serial.print(location);
   }
   
/*
 * Correct the time to remove the shortest distance
 * Also rotate the reference sensor into the NORTH location
 */
  for (i=N; i <= W; i++)
  {
    s[i].count = reference - timer_value[R(i)];
    s[i].is_valid = ((sensor_status & bit_mask[R(i)]) != 0);
  }

 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rCounts");
   Serial.print("\n\rNorth: "); Serial.print(s[N].count); Serial.print("  East: "); Serial.print(s[E].count);
   Serial.print(" South: ");    Serial.print(s[S].count); Serial.print("  West: "); Serial.print(s[W].count);
   }
/*
 * Find the smallest non-zero value
 */
  smallest = 1.0e10;
  for (i=N; i <= W; i++)
  {
    if ( (s[i].count != 0) 
        && (s[i].count < smallest ) )
    {
      smallest = s[i].count;
    }
  }

/*
 *  Prime the estimate based on the smallest identified time.
 */
 estimate = length_c / 1.414d - smallest + 1.0d;
 
 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\restimate: "); Serial.print(estimate);
   }
/*
 * Fill up the structure with the counter geometry
 * Rotated so that the longest time points north
 */
  for (i=N; i <= W; i++)
  {
    s[i].b = s[i].count;
    s[i].c = length_c;
  }
  
  for (i=N; i <= W; i++)
  {
    s[i].a = s[(i+1) % 4].b;
  }

#if (SAMPLE_CALCULATIONS)
  for (i=N; i <= W; i++)
  {
    s[i].a = s[i].a / 10.0d;
    s[i].b = s[i].b / 10.0d;
    s[i].c = s[i].c / 10.0d;
  }
#endif

/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  error = 999999;               // Start with a big error
  
  while (error > THRESHOLD )
  {
    x_avg = 0;                     // Zero out the average values
    y_avg = 0;
    last_estimate = estimate;
    
    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      find_xy(&s[i], estimate);// Locate the shot
      x_avg += s[i].xs;        // Keep the running average
      y_avg += s[i].ys;
    }

    x_avg /= 4.0d;                // Work out the average intercept
    y_avg /= 4.0d;

    estimate = sqrt(sq(s[N].x - x_avg) + sq(s[N].y - y_avg));
    error = abs(last_estimate - estimate);

    if ( read_DIP() & VERBOSE_TRACE )
    {
      Serial.print("\n\rx_avg:");  Serial.print(x_avg);   Serial.print("  y_avg:"); Serial.print(y_avg); Serial.print(" estimate:"),  Serial.print(estimate);  Serial.print(" error:"); Serial.print(error);
      Serial.println();
    }
  }
  
 /*
  * All done return
  */
  h->shot = shot;
  h->x = x_avg;
  h->y = y_avg;
  return location;
}

/*----------------------------------------------------------------
 *
 * find_xy
 *
 * Calaculate where the shot seems to lie
 *
 *----------------------------------------------------------------
 *
 *  Using the law of Cosines
 *  
 *                    C
 *                 /     \   
 *             b             a  
 *          /                   \
 *     A ------------ c ----------  B
 *  
 *  a^2 = b^2 + c^2 - 2(bc)cos(A)
 *  
 *  Rearranging terms
 *            ( a^2 - b^2 - c^2 )
 *  A = arccos( ----------------)
 *            (      -2bc       )
 *            
 *  In our system, a is the estimate for the shot location
 *                 b is the measured time + estimate of the shot location
 *                 c is the fixed distance between the sensors
 *                 
 *--------------------------------------------------------------*/

void find_xy
    (
     sensor_t* s,           // Index to be operatated on
     double estimate        // Estimated position   
     )
{
  double ae, be;            // Locations with error added
  double rotation;          // Angle shot is rotated through
  
  ae = s->a + estimate;     // Dimenstion with error included
  be = s->b + estimate;
  
  s->angle_A = acos( (sq(ae) - sq(be) - sq(s->c))/(-2.0d * be * s->c));

/*
 *  Compute the X,Y based on the detection sensor
 */
  switch (s->index)
  {
    case (N): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * sin(rotation));
      s->ys = s->y - ((be) * cos(rotation));
      break;
      
    case (E): 
      rotation = s->angle_A - PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (S): 
      rotation = s->angle_A + PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (W): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
  }

/*
 * Debugging
 */
  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\n\rindex:"); Serial.print(s->index) ; 
    Serial.print(" a:");        Serial.print(s->a);       Serial.print("  b:");  Serial.print(s->b);
    Serial.print(" ae:");       Serial.print(ae);         Serial.print("  be:"); Serial.print(be);    Serial.print(" c:"),  Serial.print(s->c);
    Serial.print(" cos:");      Serial.print(cos(rotation)); Serial.print(" sin: "); Serial.print(sin(rotation));
    Serial.print(" angle_A:");  Serial.print(s->angle_A); Serial.print("  x:");  Serial.print(s->x);  Serial.print(" y:");  Serial.print(s->y);
    Serial.print(" rotation:"); Serial.print(rotation);   Serial.print("  xs:"); Serial.print(s->xs); Serial.print(" ys:"); Serial.print(s->ys);
  }
 
/*
 *  All done, return
 */
  return;
}

/*----------------------------------------------------------------
 *
 * void rotate_shot()
 *
 * Rotate the target back into the correct position
 *
 *----------------------------------------------------------------
 * 
 * Previosuly the shot was rotated so that the impact was processed
 * as-if it came from the north sensor.
 * 
 * This funciton rotates the impact back into the correct quadrant.
 *    
 *--------------------------------------------------------------*/
unsigned int rotate[] = {0, 90, 180, 270};

void rotate_shot
  (
  unsigned int location,        // Sensor that detected the shot
  history_t* h                  // History of the shot
  )
{
  double tx, ty;                // Working X and Y positions.
  double angle;                 // Computed Angle
  double radius;                // Computed Radius
/*
 * Correct the input for the fact that the sensors are out by 45 degrees 
 */
  tx = h->x;
  ty = h->y;

  radius = sqrt(sq(tx) + sq(ty));
  angle = atan2(tx, ty) - PI_ON_4;
  tx = radius * cos(angle);
  ty = radius * sin(angle);

/*
 *  Rotated back to North up
 */

  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\n\rRotating Shot (In) x:"); Serial.print(h->x); Serial.print(" y:"); Serial.print(h->y); Serial.print("  degrees:"); Serial.print(rotate[location]);
  }
  

  switch (location)
  {
    case N:                     // North sensor, no change
      h->x = tx;
      h->y = ty;
      break;                

    case E:
      h->y = tx;
      h->x = ty;
      break;

    case S:
      h->x = -tx;
      h->y = -ty;
      break;

    case W: 
      h->y = -tx;
      h->x = -ty;
      break;
  }
  
  if ( read_DIP() & VERBOSE_TRACE )
  {
    Serial.print("\n\rRotating Shot (Out) x:"); Serial.print(h->x); Serial.print(" y:"); Serial.print(h->y);
  }
  
/*
 *  All done, return
 */
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
 * {"shot":"freETarget", "value":{shot, x, y}}
 *    
 *--------------------------------------------------------------*/

void send_score
  (
  history_t* h                // History record
  )
{
  double x, y;                   // Shot location in mm X, Y
  double radius;
  double angle;
  
  x = h->x * s_of_sound * CLOCK_PERIOD;
  y = h->y * s_of_sound * CLOCK_PERIOD;
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(x, y) / PI_ON_2 * 180.0d;
  
  Serial.print("{\"shot\":");   Serial.print(h->shot + 1);
  Serial.print(", \"x\":");     Serial.print(x); 
  Serial.print(", \"y\":");     Serial.print(y); 
  Serial.print(", \"r\":");     Serial.print(radius);
  Serial.print(", \"a\":");     Serial.print(angle);

  Serial.print(", \"N\":");     Serial.print(timer_value[N]);
  Serial.print(", \"E\":");     Serial.print(timer_value[E]);
  Serial.print(", \"S\":");     Serial.print(timer_value[S]);
  Serial.print(", \"W\":");     Serial.print(timer_value[W]);

  Serial.print("}");
  Serial.println();

  return;
}



