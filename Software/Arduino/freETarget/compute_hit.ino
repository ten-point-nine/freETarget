/*----------------------------------------------------------------
 *
 * Compute_hit.ino
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/
#include "freETarget.h"

#define THRESHOLD (0.001)

#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points
#define RH_50 50.0d                   // Fixed Relative Humidity (for now)

/*
 *  Variables
 */
extern const char* which_one[4];

sensor_t s[4];

unsigned long timer_value[4];     // Array of timer values
unsigned int  pellet_calibre;     // Time offset to compensate for pellet diameter

static void remap_target(double* x, double* y);  // Map a club target if used

/*----------------------------------------------------------------
 *
 * function: speed_of_sound
 *
 * brief: Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 * The speed of sound is computed based on the board temperature
 * The Relative Humitity is fixed at 50% pending the addtion of
 * a humidity sensor
 * 
 * Corrected tempeature algorithm from S. Carrington - Thanks
 *--------------------------------------------------------------*/

#define TO_MM   1000.0d       // Convert Meters to MM
#define TO_US 1000000.0d      // Convert seconds to microseconds

double speed_of_sound
  (
  double temperature,         // Current temperature in degrees C
  double relative_humidity    // RH, 0-1005
  )
{
  double speed_MPS;           // Speed Meters Per Second
  double speed_mmPuS;         //  Speed mm per microsecond
  

  speed_MPS   = 331.3d * sqrt( (temperature + 273.15d) / 273.15d )                                          // Temperature
                  + relative_humidity/10.0d * (0.0344857d - (0.000187143d*temperature) + (0.000236429d*sq(temperature)));  // Humidity

  speed_mmPuS  = speed_MPS * TO_MM / TO_US;      // Convert down to mm/us
  
  if ( is_trace )
    {
    Serial.print(T("\r\nSpeed of sound: ")); Serial.print(speed_mmPuS); Serial.print(T("mm/us"));
    Serial.print(T("  Worst case delay: ")); Serial.print(json_sensor_dia / speed_mmPuS * OSCILLATOR_MHZ); Serial.print(T(" counts"));
    }

  return speed_mmPuS;  
}

/*----------------------------------------------------------------
 *
 * function: init_sensors()
 *
 * brief: Setup the constants in the strucure
 * 
 * return: Sensor array updated with current geometry
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
 * The layout of the sensors is shown above.  0 is the middle of
 * the target, and the sensors located at the cardinal points.
 * 
 * This function takes the physical location of the sensors (mm)
 * and generates the sensor array based on time. (ex us / mm)
 *--------------------------------------------------------------*/
void init_sensors(void)
{

/*
 * Determine the speed of sound and ajust
 */
  s_of_sound = speed_of_sound(temperature_C(), RH_50);
  pellet_calibre = ((double)json_calibre_x10 / s_of_sound / 2.0d / 10.0d) * OSCILLATOR_MHZ; // Clock adjustement
  
 /*
  * Work out the geometry of the sensors
  */
  s[N].index = N;
  s[N].x = json_north_x / s_of_sound * OSCILLATOR_MHZ;
  s[N].y = (json_sensor_dia /2.0d + json_north_y) / s_of_sound * OSCILLATOR_MHZ;

  s[E].index = E;
  s[E].x = (json_sensor_dia /2.0d + json_east_x) / s_of_sound * OSCILLATOR_MHZ;
  s[E].y = (0.0d + json_east_y) / s_of_sound * OSCILLATOR_MHZ;

  s[S].index = S;
  s[S].x = 0.0d + json_south_x / s_of_sound * OSCILLATOR_MHZ;
  s[S].y = -(json_sensor_dia/ 2.0d + json_south_y) / s_of_sound * OSCILLATOR_MHZ;

  s[W].index = W;
  s[W].x = -(json_sensor_dia / 2.0d  + json_west_x) / s_of_sound * OSCILLATOR_MHZ;
  s[W].y = json_west_y / s_of_sound * OSCILLATOR_MHZ;
  
 /* 
  *  All done, return
  */
  return;
}

/*----------------------------------------------------------------
 *
 * funtion: compute_hit
 *
 * brief: Determine the location of the hit
 * 
 * return: record array updated with new position
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  unsigned int sensor_status,      // Bits read from status register
  this_shot*   h,                  // Storing the results
  bool         test_mode           // Fake counters in test mode
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           i, j, count;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        r1, r2;            // Distance between points
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  double        z_offset_clock;    // Time offset between paper and sensor plane
  
  if ( is_trace )
  {
    Serial.print(T("\r\ncompute_hit()"));
  }
  
/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  z_offset_clock = (double)json_z_offset  * OSCILLATOR_MHZ / s_of_sound; // Clock adjustement for paper to sensor difference
  if ( is_trace )
  {
    Serial.print(T("\r\nz_offset_clock:")); Serial.print(z_offset_clock); Serial.print(T("\r\n"));
  }
  
 /* 
  *  Read the counter registers
  */
  if ( test_mode == false )                              // Skip if using test values
  {
    read_timers();
  }
  
  if ( is_trace )
  { 
    for (i=N; i <= W; i++)
    {
      Serial.print(which_one[i]); Serial.print(timer_value[i]); Serial.print(T(" ")); 
    }
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
  
 if ( is_trace )
 {
   Serial.print(T("\r\nReference: ")); Serial.print(reference); Serial.print(T("  location:")); Serial.print(nesw[location]);
 }

/*
 * Correct the time to remove the shortest distance
 */
  for (i=N; i <= W; i++)
  {
    s[i].count = reference - timer_value[i];
    s[i].is_valid = true;
    if ( timer_value[i] == 0 )
    {
      s[i].is_valid = false;
    }
  }

  if ( is_trace )
  {
    Serial.print(T("\r\nCounts       "));
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(":"); Serial.print(s[i].count); Serial.print(T(" "));
    }
    Serial.print(T("\r\nMicroseconds "));
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(T(":")); Serial.print(((double)s[i].count) / ((double)OSCILLATOR_MHZ)); Serial.print(T(" "));
    }
  }

/*
 * Fill up the structure with the counter geometry
 */
  for (i=N; i <= W; i++)
  {
    s[i].b = s[i].count;
    s[i].c = sqrt(sq(s[(i) % 4].x - s[(i+1) % 4].x) + sq(s[(i) % 4].y - s[(i+1) % 4].y));
   }
  
  for (i=N; i <= W; i++)
  {
    s[i].a = s[(i+1) % 4].b;
  }
  
/*
 * Find the smallest non-zero value, this is the sensor furthest away from the sensor
 */
  smallest = s[N].count;
  for (i=N+1; i <= W; i++)
  {
    if ( s[i].count < smallest )
    {
      smallest = s[i].count;
    }
  }
  
/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  estimate = s[N].c - smallest + 1.0d;
 
  if ( is_trace )
   {
   Serial.print(T("\r\nestimate: ")); Serial.print(estimate);
   }
  error = 999999;                  // Start with a big error
  count = 0;

 /*
  * Iterate to minimize the error
  */
  while (error > THRESHOLD )
  {
    x_avg = 0;                     // Zero out the average values
    y_avg = 0;
    last_estimate = estimate;

    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      if ( find_xy_3D(&s[i], estimate, z_offset_clock) )
      {
        x_avg += s[i].xs;        // Keep the running average
        y_avg += s[i].ys;
      }
    }

    x_avg /= 4.0d;
    y_avg /= 4.0d;
    
    estimate = sqrt(sq(s[location].x - x_avg) + sq(s[location].y - y_avg));
    error = abs(last_estimate - estimate);

    if ( is_trace )
    {
      Serial.print(T("\r\nx_avg:"));  Serial.print(x_avg);   Serial.print(T("  y_avg:")); Serial.print(y_avg); Serial.print(T(" estimate:")),  Serial.print(estimate);  Serial.print(T(" error:")); Serial.print(error);
      Serial.println();
    }
    count++;
    if ( count > 20 )
    {
      break;
    }
  }
  
 /*
  * All done return
  */
  h->shot = shot;           // Record the shot
  h->x = x_avg;             
  h->y = y_avg;
  
  return location;
}


/*----------------------------------------------------------------
 *
 * function: find_xy_3D
 *
 * brief: Calaculate where the shot seems to lie
 * 
 * return: TRUE if the shot was computed correctly
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
 * See freETarget documentaton for algorithm
 * 
 * If there is a large distance between the target plane and the 
 * sensor plane, then the distance between the computed position
 * and the actual postion includes a large error the further
 * the pellet hits from the centre.
 * 
 * This is because the sound path from the target to the 
 * sensor includes a slant distance from the paper to the sensor
 * ex.
 * 
 *                                            // ()  Sensor  ---
 *                          Slant Range  ////     |           |
 *                                 ////           |      z_offset
 * ==============================@================|= -----------
 *                               | Paper Distance |
 *                               
 * This algorithm is the same as the regular compute_hit()
 * but corrects for the sound distance based on the z_offset 
 * between the paper and sensor
 * 
 * Sound Distance = sqrt(Paper Distance ^2 + z_offset ^2)
 * 
 * Paper Distance = sqrt(Sound Distance ^2 - z_offset ^2)
 *               
 *                 
 *--------------------------------------------------------------*/

bool find_xy_3D
    (
     sensor_t* s,           // Sensor to be operatated on
     double estimate,       // Estimated position
     double z_offset_clock  // Time difference between paper and sensor plane
     )
{
  double ae, be;            // Locations with error added
  double rotation;          // Angle shot is rotated through

/*
 * Check to see if the sensor data is correct.  If not, return an error
 */
  if ( s->is_valid == false )
  {
    if ( is_trace )
    {
      Serial.print(T("\r\nSensor: ")); Serial.print(s->index); Serial.print(T(" no data"));
    }
    return false;           // Sensor did not trigger.
  }

/*
 * It looks like we have valid data.  Carry on
 */
  ae = sqrt(sq(s->a + estimate) - sq(z_offset_clock));     // Dimenstion with error included
  be = sqrt(sq(s->b + estimate) - sq(z_offset_clock));

  if ( (ae + be) < s->c )   // Check for an accumulated round off error
    {
    s->angle_A = 0;         // Yes, then force to zero.
    }
  else
    {  
    s->angle_A = acos( (sq(ae) - sq(be) - sq(s->c))/(-2.0d * be * s->c));
    }
  
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

    default:
      if ( is_trace )
      {
        Serial.print(T("\n\nUnknown Rotation:")); Serial.print(s->index);
      }
      break;
  }

/*
 * Debugging
 */
  if ( is_trace )
    {
    Serial.print(T("\r\nindex:")); Serial.print(s->index) ; 
    Serial.print(T(" a:"));        Serial.print(s->a);       Serial.print(T("  b:"));  Serial.print(s->b);
    Serial.print(T(" ae:"));       Serial.print(ae);         Serial.print(T("  be:")); Serial.print(be);    Serial.print(T(" c:")),  Serial.print(s->c);
    Serial.print(T(" cos:"));      Serial.print(cos(rotation)); Serial.print(T(" sin: ")); Serial.print(sin(rotation));
    Serial.print(T(" angle_A:"));  Serial.print(s->angle_A); Serial.print(T("  x:"));  Serial.print(s->x);  Serial.print(T(" y:"));  Serial.print(s->y);
    Serial.print(T(" rotation:")); Serial.print(rotation);   Serial.print(T("  xs:")); Serial.print(s->xs); Serial.print(T(" ys:")); Serial.print(s->ys);
    }
 
/*
 *  All done, return
 */
  return true;
}

  
/*----------------------------------------------------------------
 *
 * function: send_score
 *
 * brief: Send the score out over the serial port
 * 
 * return: None
 *
 *----------------------------------------------------------------
 * 
 * The score is sent as:
 * 
 * {"shot":n, "x":x, "y":y, "r(adius)":r, "a(ngle)": a, debugging info ..... }
 * 
 * It is up to the PC program to convert x & y or radius and angle
 * into a meaningful score relative to the target.
 *    
 *--------------------------------------------------------------*/

void send_score
  (
  this_shot* h,                   // record record
  int shot,                       // Current shot
  int sensor_status               // Status at the time of the shot
  )
{
  int    i;                       // Iteration Counter
  double x, y;                    // Shot location in mm X, Y
  double radius;
  double angle;
  unsigned int volts;
  double clock_face;
  double coeff;                   // From Alex Bird
  int    z;
  double score;
  char   str[256], str_c[10];  // String holding buffers
  
  if ( is_trace )
  {
    Serial.print(T("\r\nSending the score"));
  }

 /* 
  *  Work out the hole in perfect coordinates
  */
  x = h->x * s_of_sound * CLOCK_PERIOD;
  y = h->y * s_of_sound * CLOCK_PERIOD;
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(h->y, h->x) / PI * 180.0d;

/*
 * Rotate the result based on the construction, and recompute the hit
 */
  angle += json_sensor_angle;
  x = radius * cos(PI * angle / 180.0d);
  y = radius * sin(PI * angle / 180.0d);
  remap_target(&x, &y);             // Change the target if needed
  
/* 
 *  Display the results
 */
  sprintf(str, "\r\n{");
  output_to_all(str);
  
#if ( S_SHOT )
  sprintf(str, "\"shot\":%d, \"miss\":0, \"name\":\"%s\", ", shot,  names[json_name_id]);
  output_to_all(str);
  sprintf(str, "\"time\":%ld, ", h->shot_time);
  output_to_all(str);
#endif

#if ( S_SCORE )
  coeff = 9.9 / (((double)json_1_ring_x10 + (double)json_calibre_x10) / 20.0d);
  score = 10.9 - (coeff * radius);
  z = 360 - (((int)angle - 90) % 360);
  clock_face = (double)z / 30.0;
  sprintf(str, "\"score\": %d, "\"clock\":\"%d:%d, \"  ", score,(int)clock_face, (int)(60*(clock_face-((int)clock_face))) ;
  output_to_all(str);
#endif

#if ( S_XY )
  dtostrf(x, 4, 2, str_c );
  sprintf(str, "\"x\":%s, ", str_c);
  output_to_all(str);
  dtostrf(y, 4, 2, str_c );
  sprintf(str, "\"y\":%s, ", str_c);
  output_to_all(str);
#endif

#if ( S_POLAR )
  dtostrf(radius, 4, 2, str_c );
  sprintf(str, " \"r\":%s, ", str_c);
  output_to_all(str);
  dtostrf(angle, 4, 2, str_c );
  sprintf(str, "\"a\":%s, ", str_c);
  output_to_all(str);
#endif

#if ( S_COUNTERS )
  sprintf(str, "\"N\":%d, \"E\":%d, \"S\":%d, \"W\":%d, ", (int)s[N].count, (int)s[E].count, (int)s[S].count, (int)s[W].count);
  output_to_all(str);
#endif

#if ( S_MISC ) 
  volts = analogRead(V_REFERENCE);
  dtostrf(TO_VOLTS(volts), 2, 2, str_c );
  sprintf(str, "\"V_REF\":%s, ", str_c);
  output_to_all(str);
  dtostrf(temperature_C(), 2, 2, str_c );
  sprintf(str, "\"T\":%s, ", str_c);
  output_to_all(str);
  sprintf(str, "\"VERSION\":%s ", SOFTWARE_VERSION);
  output_to_all(str);
#endif

  sprintf(str, "}\r\n");
  output_to_all(str);
  output_to_all(0);
  
/*
 * All done, return
 */
  
  return;
}
 
/*----------------------------------------------------------------
 *
 * function: send_miss
 *
 * brief: Send out a miss message
 * 
 * return: None
 *
 *----------------------------------------------------------------
 * 
 * This is an abbreviated score message to show a miss
 *    
 *--------------------------------------------------------------*/

void send_miss
  (
  int shot                        // Current shot
  )
{
  char str[256];    // String holding buffer
  
/* 
 *  Display the results
 */
  sprintf(str, "\r\n{");
  output_to_all(str);
  
 #if ( S_SHOT )
  sprintf(str, "\"shot\":%d, \"miss\":1, \"name\":\"%s\", \"time\":%d, ", shot, names[json_name_id], now/100) ;
  output_to_all(str);
#endif

#if ( S_XY )
  sprintf(str, "\"x\":0, \"y\":0");
  output_to_all(str);
#endif

  sprintf(str, "}\n\r");
  output_to_all(str);
  output_to_all(0);


/*
 * All done, go home
 */
  return;
}


/*----------------------------------------------------------------
 *
 * function: remap_target
 *
 * brief: Remaps shot into a different target
 * 
 * return: Pellet location remapped to centre bull
 *
 *----------------------------------------------------------------
 *
 *  For example a five bull target looks like
 *  
 *     **        **
 *     **        **
 *          **
 *          **
 *     **        **     
 *     **        **
 *               
 * The function send_score locates the pellet onto the paper                
 * This function finds the closest bull and then maps the pellet
 * onto the centre one.
 *--------------------------------------------------------------*/
struct new_target
{
  double       x;       // X location of Bull
  double       y;       // Y location of Bull
};

typedef new_target new_target_t;

#define D5 (74/2)                   // Five bull air rifle is 74mm centre-centre
new_target_t five_bull_air_rifle[] = { {-D5, D5}, {D5, D5}, {-D5, -D5}, {D5, -D5}, {0,0}};

static void remap_target
  (
  double* x,                        // Computed X location of shot
  double* y                         // Computed Y location of shot
  )
{
  double distance, closest;        // Distance to bull in clock ticks
  double dx, dy;                   // Best fitting bullseye
  new_target_t* ptr;               // Bull pointer
  int    which_one;                // Which target was selected
  
  if ( is_trace )
  {
    Serial.print(T("\n\rnew_target x:")); Serial.print(*x); Serial.print(" y:"); Serial.print(*y);
  }

/*
 * Dind the closes bull
 */
  switch ( json_target_type )
  {
    case FIVE_BULL_AIR_RIFLE:
      ptr = &five_bull_air_rifle[0];
      break;
    
    default:                      // Not defined, assume a regular       
      return;                     // bull and do nothing
  }
  
  closest = 100000.0;             // Distance to closest bull
  
/*
 * Loop and find the closest
 */
  which_one = 0;
  while ( ptr->x != 0 )
  {
    distance = sqrt(sq(ptr->x - *x) + sq(ptr->y - *y));
    if ( is_trace )
    {
      Serial.print(T("\n\rwhich_one:")); Serial.print(which_one); Serial.print(T(" distance:")); Serial.print(distance); 
    }
    if ( distance < closest )   // Found a closer one?
    {
      closest = distance;       // Remember it
      dx = ptr->x;
      dy = ptr->y;              // Remember the closest bull
      if ( is_trace)
      {
        Serial.print(T(" dx:")); Serial.print(dx); Serial.print(T(" dy:")); Serial.print(dy); 
      }
    }
    ptr++;
    which_one++;
  }

  distance = sqrt(sq(*x) + sq(*y)); // Last one is the centre bull
  if ( is_trace )
  {
    Serial.print(T("\n\rwhich_one:")); Serial.print(which_one); Serial.print(T(" distance:")); Serial.print(distance);
  }
  if ( distance < closest )   // Found a closer one?
  {
    closest = distance;       // Remember it
    dx = 0;
    dy = 0;
    if ( is_trace)
    {
      Serial.print(T(" dx:")); Serial.print(dx); Serial.print(T(" dy:")); Serial.print(dy); 
    }
  }

/*
 * Remap the pellet to the centre one
 */
  *x = *x - dx;
  *y = *y - dy;
  if ( is_trace )
  {
    Serial.print(T("\n\rx:")); Serial.print(*x); Serial.print(T(" y:")); Serial.print(*y);
  }
  
/*
 *  All done, return
 */
  return;
}
/*----------------------------------------------------------------
 *
 * function: show_timer
 *
 * brief: Display a timer message to identify errors
 *
 * return: None
 *----------------------------------------------------------------
 * 
 * The error is sent as:
 * 
 * {"error": Run Latch, timer information .... }
 *    
 *--------------------------------------------------------------*/

void send_timer
  (
  int sensor_status                        // Flip Flop Input
  )
{
  int i;

  read_timers();
  
  Serial.print(T("{\"timer\": \""));
  for (i=0; i != 4; i++ )
  {
    if ( sensor_status & (1<<i) )
    {
      Serial.print(nesw[i]);
    }
    else
    {
      Serial.print(T("."));
    }
  }

  Serial.print(T("\", "));
  for (i=N; i <= W; i++)
  {
    Serial.print(T("\"")); Serial.print(nesw[i]); Serial.print(T("\":"));  Serial.print(timer_value[i]);  Serial.print(T(", "));
  }

  Serial.print(T("\"V_REF\":"));   Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));  Serial.print(T(", "));
  Serial.print(T("\"Version\":")); Serial.print(SOFTWARE_VERSION);
  Serial.print(T("}\r\n"));      

  return;
}

 
