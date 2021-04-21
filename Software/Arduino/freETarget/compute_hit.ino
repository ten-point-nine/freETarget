/*----------------------------------------------------------------
 *
 * Compute_hit.ino
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"
#include "compute_hit.h"
#include "freETarget.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "arduino.h"

#define THRESHOLD (0.001)

#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */

extern const char* which_one[4];

sensor_t s[4];

unsigned int  bit_mask[] = {0x01, 0x02, 0x04, 0x08};
unsigned long timer_value[4];     // Array of timer values
unsigned long minion_value[4];    // Array of timers values from minion
unsigned long minion_ref;         // Trip voltage from minion
unsigned int  pellet_calibre;     // Time offset to compensate for pellet diameter


/*----------------------------------------------------------------
 *
 * function: speed_of_sound
 *
 * brief: Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 * The speed of sound is computed based on the board temperature
 * 
 *--------------------------------------------------------------*/

double speed_of_sound
  (
  double temperature          // Current temperature in degrees C
  )
{
  double speed;

  speed = (331.3d + 0.606d * temperature) * 1000.0d / 1000000.0d; 
  
  if ( is_trace )
    {
    Serial.print("\r\nSpeed of sound: "); Serial.print(speed); Serial.print("mm/us");
    Serial.print("  Worst case delay: "); Serial.print(json_sensor_dia / speed * OSCILLATOR_MHZ); Serial.print(" counts");
    }

  return speed;  
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
  s_of_sound = speed_of_sound(temperature_C());
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
 * return: History array updated with new position
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  unsigned int sensor_status,      // Bits read from status register
  history_t*   h,                  // Storing the results
  bool         test_mode           // Fake counters in test mode
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           discard;           // Discard the smallest timer value
  int           i, j, count;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        r1, r2;            // Distance between points
  double        x, y;              // Computed location
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  double        constillation;     // How many constillations did we compute
  int           v_ref;             // reference voltage for this board
  
  if ( is_trace )
  {
    Serial.print("\r\ncompute_hit()");
  }
  
/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  
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
      Serial.print(which_one[i]); Serial.print(timer_value[i]); Serial.print(" "); 
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
   Serial.print("\r\nReference: "); Serial.print(reference); Serial.print("  location:"); Serial.print(nesw[location]);
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
    Serial.print("\r\nCounts       ");
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(s[i].count); Serial.print(" ");
    }
    Serial.print("\r\nMicroseconds ");
    for (i=N; i <= W; i++)
    {
     Serial.print(*which_one[i]); Serial.print(((double)s[i].count) / ((double)OSCILLATOR_MHZ)); Serial.print(" ");
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
  discard = N;
  for (i=N+1; i <= W; i++)
  {
    if ( s[i].count < smallest )
    {
      smallest = s[i].count;
      discard = i;
    }
  }
//  s[discard].is_valid = false;    // Throw away the shortest time. (test pathc removed)
  
/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  estimate = s[N].c - smallest + 1.0d;
 
  if ( is_trace )
   {
   Serial.print("\r\nestimate: "); Serial.print(estimate);
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

    constillation = 0.0;
    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      if ( find_xy(&s[i], estimate) )
      {
        x_avg += s[i].xs;        // Keep the running average
        y_avg += s[i].ys;
        constillation += 1.0;
      }
    }

    x_avg /= constillation;      // Work out the average intercept
    y_avg /= constillation;

    estimate = sqrt(sq(s[location].x - x_avg) + sq(s[location].y - y_avg));
    error = abs(last_estimate - estimate);

    if ( is_trace )
    {
      Serial.print("\r\nx_avg:");  Serial.print(x_avg);   Serial.print("  y_avg:"); Serial.print(y_avg); Serial.print(" estimate:"),  Serial.print(estimate);  Serial.print(" error:"); Serial.print(error);
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
  h->shot = shot;
  h->x = x_avg;
  h->y = y_avg;
  return location;
}

/*----------------------------------------------------------------
 *
 * function: find_xy
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
 *--------------------------------------------------------------*/

bool find_xy
    (
     sensor_t* s,           // Sensor to be operatated on
     double estimate        // Estimated position   
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
      Serial.print("\r\nSensor: "); Serial.print(s->index); Serial.print(" no data");
    }
    return false;           // Sensor did not trigger.
  }

/*
 * It looks like we have valid data.  Carry on
 */
  ae = s->a + estimate;     // Dimenstion with error included
  be = s->b + estimate;

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
        Serial.print("\n\nUnknown Rotation:"); Serial.print(s->index);
      }
      break;
  }

/*
 * Debugging
 */
  if ( is_trace )
    {
    Serial.print("\r\nindex:"); Serial.print(s->index) ; 
    Serial.print(" a:");        Serial.print(s->a);       Serial.print("  b:");  Serial.print(s->b);
    Serial.print(" ae:");       Serial.print(ae);         Serial.print("  be:"); Serial.print(be);    Serial.print(" c:"),  Serial.print(s->c);
    Serial.print(" cos:");      Serial.print(cos(rotation)); Serial.print(" sin: "); Serial.print(sin(rotation));
    Serial.print(" angle_A:");  Serial.print(s->angle_A); Serial.print("  x:");  Serial.print(s->x);  Serial.print(" y:");  Serial.print(s->y);
    Serial.print(" rotation:"); Serial.print(rotation);   Serial.print("  xs:"); Serial.print(s->xs); Serial.print(" ys:"); Serial.print(s->ys);
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
  history_t* h,                   // History record
  int shot,                       // Current shot
  int sensor_status               // Status at the time of the shot
  )
{
  double x, y;                   // Shot location in mm X, Y
  double radius;
  double angle;
  unsigned int volts;
  double clock_face;
  double coeff;                 // From Alex Bird
  int    z;
  double score;


  if ( is_trace )
  {
    Serial.print("\r\nSending the score");
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

/* 
 *  Display the results
 */
  PRINT("\r\n{");
  
#if ( S_SHOT )
  PRINT("\"shot\":");      PRINT(shot); PRINT(", ");
  PRINT("\"miss\": 0, ");
  
  if ( json_name_id != 0 )
  {
    PRINT("\"name\":\"");      PRINT(names[json_name_id]);     PRINT("\", ");
  }
#endif

#if ( S_SCORE )
  coeff = 9.9 / (((double)json_1_ring_x10 + (double)json_calibre_x10) / 20.0d);
  score = 10.9 - (coeff * radius);
  PRINT("\"score\":");      PRINT(score); PRINT(", ");
  z = 360 - (((int)angle - 90) % 360);
  clock_face = (double)z / 30.0;
  PRINT("\"clock\":\"");    PRINT((int)clock_face);     PRINT(":");     PRINT((int)(60*(clock_face-((int)clock_face))));     PRINT("\", ");
#endif

#if ( S_XY )
  PRINT("\"x\":");     PRINT(x);    PRINT(", ");
  PRINT("\"y\":");     PRINT(y);    PRINT(", ");
#endif

#if ( S_POLAR )
  PRINT("\"r\":");     PRINT(radius);    PRINT(", ");
  PRINT("\"a\":");     PRINT(angle);     PRINT(", ");
#endif

#if ( S_COUNTERS )
  PRINT("\", ");
  PRINT("\"N\":");     PRINT((int)s[N].count);      PRINT(", ");
  PRINT("\"E\":");     PRINT((int)s[E].count);      PRINT(", ");
  PRINT("\"S\":");     PRINT((int)s[S].count);      PRINT(", ");
  PRINT("\"W\":");     PRINT((int)s[W].count);      PRINT(", ");
#endif

#if ( S_MISC ) 
  volts = analogRead(V_REFERENCE);
  PRINT("\"V_REF\":");       PRINT(TO_VOLTS(volts));      PRINT(", ");
  PRINT("\"T\":");           PRINT(temperature_C());      PRINT(", ");
  PRINT("\"VERSION\":");     PRINT(SOFTWARE_VERSION);
#endif

  PRINT("}\r\n");
  
  return;
}
 
/*----------------------------------------------------------------
 *
 * vfunction: send_miss
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
  
/* 
 *  Display the results
 */
  PRINT("\r\n{");
  
#if ( S_SHOT )
  PRINT("\"shot\":");      PRINT(shot); PRINT(", ");
  PRINT("\"miss\": 1,"); 
  
  if ( json_name_id != 0 )
  {
    PRINT("\"name\":\"");      PRINT(names[json_name_id]);     PRINT("\", ");
  }
#endif

#if ( S_XY )
  PRINT("\"x\": 0, \"y\": 0")
#endif

  PRINT("}\r\n");
  
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
  
  Serial.print("{\"timer\": \"");
  for (i=0; i != 4; i++ )
  {
    if ( sensor_status & (1<<i) )
    {
      Serial.print(nesw[i]);
    }
    else
    {
      Serial.print('.');
    }
  }
  
  Serial.print("\", ");
  Serial.print("\"N\":");       Serial.print(timer_value[N]);                     Serial.print(", ");
  Serial.print("\"E\":");       Serial.print(timer_value[E]);                     Serial.print(", ");
  Serial.print("\"S\":");       Serial.print(timer_value[S]);                     Serial.print(", ");
  Serial.print("\"W\":");       Serial.print(timer_value[W]);                     Serial.print(", ");
  Serial.print("\"V_REF\":");   Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));  Serial.print(", ");
  Serial.print("\"Version\":"); Serial.print(SOFTWARE_VERSION);
  Serial.print("}\r\n");      

  return;
}

/*----------------------------------------------------------------
 *
 * function: hamming
 *
 * brief: Compute the Hamming weight of the input
 *
 * return: Hamming weight of the input word
 * 
 *----------------------------------------------------------------
 *    
 * Add up all of the 1's in the sample.
 * 
 *--------------------------------------------------------------*/

 unsigned int hamming
   (
   unsigned int sample
   )
 {
  unsigned int i;

  i = 0;
  while (sample)
  {
    if ( sample & 1 )
    {
      i++;                  // Add up the number of 1s
    }
    sample >>= 1;
    sample &= 0x7FFF;
  }
  
  if ( is_trace )
    {
    Serial.print("\r\nHamming weight: "); Serial.print(i);
    }

 /*
  * All done, return
  */
  return i;
 }
 
