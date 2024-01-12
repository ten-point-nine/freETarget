/*****************************************************************************
 *
 * Compute_hit.c
 *
 * Determine the score
 *
 *****************************************************************************/
#include "math.h"
#include "analog_io.h"
#include "stdio.h" 
#include "math.h"
#include "stdbool.h"
#include "serial_io.h"

#include "freETarget.h"
#include "json.h"
#include "compute_hit.h"
#include "diag_tools.h"
#include "token.h"
#include "timer.h"

#define THRESHOLD (0.001)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */
sensor_t s[4];
unsigned int  pellet_calibre;     // Time offset to compensate for pellet diameter
static volatile unsigned long wdt; // Warchdog  timer

static void remap_target(double* x, double* y);  // Map a club target if used

/*----------------------------------------------------------------
 *
 * @function: init_sensors()
 *
 * @brief: Setup the constants in the strucure
 * 
 * @return: Sensor array updated with current geometry
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
  if ( DLT(DLT_DIAG) ) 
  {
    printf("init_sensors()");
  }
  
/*
 * Determine the speed of sound and ajust
 */
  s_of_sound = speed_of_sound(temperature_C(), humidity_RH());
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
 * @funtion: compute_hit
 *
 * @brief: Determine the location of the hit
 * 
 * @return: Sensor location used to recognize shot
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 * 
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  shot_record_t* shot              // Storing the results
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           i, count;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  double        z_offset_clock;    // Time offset between paper and sensor plane

  x_avg = 0;
  y_avg = 0;
  
  timer_new(&wdt, 20);
      
  if ( DLT(DLT_DIAG) )
  {
    printf("compute_hit()"); 
  }

/* 
 *  Check for a miss
 */
  if ( (shot->face_strike != 0) || (shot->timer_count[N] == 0) || (shot->timer_count[E] == 0) || (shot->timer_count[S] == 0) || (shot->timer_count[W] == 0 ) )
  {
    if ( DLT(DLT_DIAG) )
    {
      printf("Miss detected");
    }
    return MISS;
  }

/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  z_offset_clock = (double)json_z_offset  * OSCILLATOR_MHZ / s_of_sound; // Clock adjustement for paper to sensor difference
  if ( DLT(DLT_DIAG) )
  {
    printf("z_offset_clock: %4.2f", z_offset_clock);
  }
  
 /* 
  *  Display the timer registers if in trace mode
  */  
  if ( DLT(DLT_DIAG) )
  { 
    for (i=N; i <= W_lo; i++)
    {
      printf("%s: %d ", which_one[i], shot->timer_count[i]);
    }
  }
  
/*
 * Determine the location of the reference counter (longest time)
 */
  reference = shot->timer_count[N];
  location = N;
  for (i=E; i <= W; i++)
  {
    if ( shot->timer_count[i] > reference )
    {
      reference = shot->timer_count[i];
      location = i;
    }
  }
  
 if ( DLT(DLT_DIAG) )
 {
   printf("Reference: %4.2f   location: %s", reference, which_one[location]);
 }

/*
 * Correct the time to remove the shortest distance
 */
  for (i=N; i <= W; i++)
  {
    s[i].count = reference - shot->timer_count[i];
    s[i].is_valid = true;
    if ( shot->timer_count[i] == 0 )
    {
      s[i].is_valid = false;
    }
  }


  if ( DLT(DLT_DIAG) )
  {
    printf("Counts       ");
    for (i=N; i <= W; i++)
    {
     printf("%s: %4.2f ", which_one[i], s[i].count);
    }
  }
  if ( DLT(DLT_DIAG) )
  {
    printf("\r\nMicroseconds ");
    for (i=N; i <= W; i++)
    {
     printf("%s: %4.2f ", which_one[i], (double)s[i].count / ((double)OSCILLATOR_MHZ));
    }
  }

#if(0)

/*
 * Compensate for sound attenuation over a long distance
 * 
 * For large targets the sound will attenuate between the closest and furthest sensor.  For small targets this is negligable, but for a Type 12
 * target the difference can be measured in microseconds.  This loop subtracts an function of the distance between the closest and 
 * current sensor.
 * 
 * The value of SOUND_ATTENUATION is found by analyzing the closest and furthest traces
 * 
 * From tests, the error was 7us over a 700us delay.  Since sound attenuates as the square of distance, this is 
 */
  for (i=N; i <= W; i++)
  {
    printf("\r\n%s %4.2f", which_one[i], s[i].count);
    x = (double)s[i].count;             // Time difference in clock ticks
    x = x / OSCILLATOR_MHZ;             // Convert to a time in us
    x = x * x;                          // Dopper's inverse Square
    x = x * json_doppler;               // Compensation in us
    x = x * OSCILLATOR_MHZ;             // Compensation in clock ticks
    s[i].count -= (int)x;               // Add in the correction
  }

  if ( DLT(DLT_DIAG) )
  {
    printf("Compensate Counts       ");
    for (i=N; i <= W; i++)
    {
     printf("%s:%4.2f  ", which_one[i], (double)s[i].count / ((double)OSCILLATOR_MHZ));
    }
  }
#endif
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
  estimate = s[N].count - smallest + 1.0d;
 
  if ( DLT(DLT_DIAG) )
  {
   printf("estimate: %4.2f", estimate);
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
    error = fabs(last_estimate - estimate);

    if ( DLT(DLT_DIAG) )
    {
      printf("x_avg: %4.2f  y_avg: %4.2f estimate: %4.2f error: %4.2f", x_avg, y_avg, estimate, error);
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
  shot->x = x_avg;             
  shot->y = y_avg;

  return location;
}


/*----------------------------------------------------------------
 *
 * @function: find_xy_3D
 *
 * @brief: Calaculate where the shot seems to lie
 * 
 * @return: TRUE if the shot was computed correctly
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
  double x;                 // Temporary value
  
/*
 * Check to see if the sensor data is correct.  If not, return an error
 */
  if ( s->is_valid == false )
  {
    if ( DLT(DLT_DIAG) )
    {
      printf("Sensor: %d no data", s->index);
    }
    return false;           // Sensor did not trigger.
  }

/*
 * It looks like we have valid data.  Carry on
 */
  x = sq(s->a + estimate); // - sq(z_offset_clock);
  if ( x < 0 )
  {
    sq(s->a + estimate);
    if ( DLT(DLT_DIAG) )
    {
      printf("s->a is complex, truncting");
    }
  }
  ae = sqrt(x);                             // Dimenstion with error included
  
  x = sq(s->b + estimate); // - sq(z_offset_clock);
  if ( x < 0 )
  {
    if ( DLT(DLT_DIAG) )
    {
      printf("s->b is complex, truncting");
    }
    sq(s->b + estimate);
  }
  be = sqrt(x);  

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
  rotation = 0;
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
      if ( DLT(DLT_DIAG) )
      {
        printf("\n\nUnknown Rotation:, %d", s->index);
      }
      break;
  }

/*
 * Debugging
 */
  if ( DLT(DLT_DIAG) )
    {
    printf("index: %d  a:%4.2f b: %4.2f ae: %4.2f  be: %4.2f c: %4.2f", s->index, s->a, s->b, ae, be, s->c);
    printf(" cos: %4.2f  sin: %4.2f  angle_A: %4.2f  x: %4.2f y: %4.2f", cos(rotation), sin(rotation), s->angle_A, s->x, s->y);
    printf(" rotation: %4.2f  xs: %4.2f  ys: %4.2f", rotation, s->xs, s->ys);
    }
 
/*
 *  All done, return
 */
  return true;
}

  
/*----------------------------------------------------------------
 *
 * @function: send_score
 *
 * @brief: Send the score out over the serial port
 * 
 * @return: None
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
  shot_record_t* shot             //  record
  )
{
  double x, y;                    // Shot location in mm X, Y
  double real_x, real_y;          // Shot location in mm X, Y before remap
  double radius;
  double angle;
  char   str[256];                // String holding buffers
  
  if ( DLT(DLT_DIAG) )
  {
    printf("Sending the score");
  }

/*
 * Grab the token ring if needed
 */
  if ( json_token != TOKEN_NONE )
  {
     while(my_ring != whos_ring)
    {
      token_take();                              // Grab the token ring
      timer_new(&wdt, 2 * ONE_SECOND);
      while ( ( wdt != 0 )                      // Wait up to 2 seconds
        && (whos_ring != my_ring) )             // Or we own the ring
      { 
        token_poll();
      }
    }
    set_status_LED(LED_WIFI_SEND);
  }
  
 /* 
  *  Work out the hole in perfect coordinates
  */
  x = shot->x * s_of_sound * CLOCK_PERIOD;        // Distance in mm
  y = shot->y * s_of_sound * CLOCK_PERIOD;        // Distance in mm
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(shot->y, shot->x) / PI * 180.0d;

/*
 * Rotate the result based on the construction, and recompute the hit
 */
  angle += json_sensor_angle;
  x = radius * cos(PI * angle / 180.0d);          // Rotate onto the target face
  y = radius * sin(PI * angle / 180.0d);
  real_x = x;
  real_y = y;                                     // Remember the original target valuee
  remap_target(&x, &y);                           // Change the target if needed
  
/* 
 *  Display the results
 */
  sprintf(str, "\r\n{");
  serial_to_all(str, ALL);
  
#if ( S_SHOT )
  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF))
  {
    sprintf(str, "\"shot\":%d, \"miss\":0, \"name\":\"%s\"", shot->shot_number,  names[json_name_id]);
  }
  else
  {
    sprintf(str, "\"shot\":%d, \"name\":\"%d\"", shot->shot_number,  my_ring);
  }
  serial_to_all(str, ALL);
  sprintf(str, ", \"time\":%4.2f ", (float)shot->shot_time/(float)(ONE_SECOND));
  serial_to_all(str, ALL);
#endif

#if ( S_XY )
  sprintf(str, ",\"x\":%4.2f, \"y\":%4.2f ", x, y);
  serial_to_all(str, ALL);
  
  if ( json_target_type > 1 )
  {
    sprintf(str, ",\"real_x\":%4.2f, \"real_y\":%4.2f ", real_x, real_y);
    serial_to_all(str, ALL);
  }
#endif

#if ( S_POLAR )
  if ( json_token == TOKEN_NONE )
  {
    dtostrf(radius, 4, 2, str_c );
    sprintf(str, ", \"r\":%4.2f,  \"a\":%s, ", radius, angle``);
  }
#endif

#if ( S_TIMERS )
  if ( json_token == TOKEN_NONE )
  {
    sprintf(str, ", \"N\":%d, \"E\":%d, \"S\":%d, \"W\":%d ", (int)s[N].count, (int)s[E].count, (int)s[S].count, (int)s[W].count);
    serial_to_all(str, ALL);
  }
#endif

  sprintf(str, "}\r\n");
  serial_to_all(str, ALL);
  
/*
 * All done, return
 */
  if ( json_token != TOKEN_NONE )
  {
    token_give();                            // Give up the token ring
    set_status_LED(LED_READY);
  }
  return;
}
 
/*----------------------------------------------------------------
 *
 * @function: send_miss
 *
 * @brief: Send out a miss message
 * 
 * @return: None
 *
 *----------------------------------------------------------------
 * 
 * This is an abbreviated score message to show a miss
 *    
 *--------------------------------------------------------------*/

void send_miss
  (
  shot_record_t* shot                    // record record
  )
{
  char str[256];                          // String holding buffer
  
  if ( json_send_miss != 0)               // If send_miss not enabled
  {
    return;                               // Do nothing
  }

/*
 * Grab the token ring if needed
 */
  if ( json_token != TOKEN_NONE )
  {
     while(my_ring != whos_ring)
    {
      token_take();                              // Grab the token ring
      timer_new(&wdt, ONE_SECOND);
      while ( (wdt != 0)
        && ( my_ring == whos_ring) )
      { 
        token_poll();
      }
      set_status_LED(LED_WIFI_SEND);
    }
  }
  
/* 
 *  Display the results
 */
  sprintf(str, "\r\n{");
  serial_to_all(str, ALL);
  
 #if ( S_SHOT )
   if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF))
  {
    sprintf(str, "\"shot\":%d, \"miss\":0, \"name\":\"%s\"", shot->shot_number,  names[json_name_id]);
  }
  else
  {
    sprintf(str, "\"shot\":%d, \"miss\":1, \"name\":\"%d\"", shot->shot_number,  my_ring);
  }
  serial_to_all(str, ALL);
  sprintf(str, ", \"time\":%4.2f ", (float)shot->shot_time/(float)(ONE_SECOND));
#endif

#if ( S_XY )
  if ( json_token == TOKEN_NONE )
  { 
    sprintf(str, ", \"x\":0, \"y\":0 ");
    serial_to_all(str, ALL);
  }
#endif

#if ( S_TIMERS )
  if ( json_token == TOKEN_NONE )
  {
    sprintf(str, ", \"N\":%d, \"E\":%d, \"S\":%d, \"W\":%d ", (int)shot->timer_count[N], (int)shot->timer_count[E], (int)shot->timer_count[S], (int)shot->timer_count[W]);
    serial_to_all(str, ALL);
    sprintf(str, ", \"face\":%d ", shot->face_strike);
    serial_to_all(str, ALL);
  }
#endif

  sprintf(str, "}\n\r");
  serial_to_all(str, ALL);

/*
 * All done, go home
 */
  token_give();
  set_status_LED(LED_READY);
  return;
}


/*----------------------------------------------------------------
 *
 * @function: remap_target
 *
 * @brief: Remaps shot into a different target
 * 
 * @return: Pellet location remapped to centre bull
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

typedef struct new_target new_target_t;

#define LAST_BULL (-1000.0)
#define D5_74 (74/2)                   // Five bull air rifle is 74mm centre-centre
new_target_t five_bull_air_rifle_74mm[] = { {-D5_74, D5_74}, {D5_74, D5_74}, {0,0}, {-D5_74, -D5_74}, {D5_74, -D5_74}, {LAST_BULL, LAST_BULL}};

#define D5_79 (79/2)                   // Five bull air rifle is 79mm centre-centre
new_target_t five_bull_air_rifle_79mm[] = { {-D5_79, D5_79}, {D5_79, D5_79}, {0,0}, {-D5_79, -D5_79}, {D5_79, -D5_79}, {LAST_BULL, LAST_BULL}};

#define D12_H (191.0/2.0)              // Twelve bull air rifle 95mm Horizontal
#define D12_V (195.0/3.0)              // Twelve bull air rifle 84mm Vertical
new_target_t twelve_bull_air_rifle[]    = { {-D12_H,   D12_V + D12_V/2},  {0,   D12_V + D12_V/2},  {D12_H,   D12_V + D12_V/2},
                                            {-D12_H,           D12_V/2},  {0,           D12_V/2},  {D12_H,           D12_V/2},
                                            {-D12_H,         - D12_V/2},  {0,         - D12_V/2},  {D12_H,          -D12_V/2},
                                            {-D12_H, -(D12_V + D12_V/2)}, {0, -(D12_V + D12_V/2)}, {D12_H, -(D12_V + D12_V/2)},
                                            {LAST_BULL, LAST_BULL}};

#define O12_H (144.0/2.0)              // Twelve bull air rifle Orion
#define O12_V (190.0/3.0)              // Twelve bull air rifle Orion
new_target_t orion_bull_air_rifle[]    =  { {-O12_H,   O12_V + O12_V/2},  {0,   O12_V + O12_V/2},  {O12_H,   O12_V + O12_V/2},
                                            {-O12_H,           O12_V/2},  {0,           O12_V/2},  {O12_H,           O12_V/2},
                                            {-O12_H,         - O12_V/2},  {0,         - O12_V/2},  {O12_H,          -O12_V/2},
                                            {-O12_H, -(O12_V + O12_V/2)}, {0, -(O12_V + O12_V/2)}, {O12_H, -(O12_V + O12_V/2)},
                                            {LAST_BULL, LAST_BULL}};
                                            
//                           0  1  2  3              4                        5              6  7  8  9  10           11                     12
new_target_t* ptr_list[] = { 0, 0, 0, 0, five_bull_air_rifle_74mm, five_bull_air_rifle_79mm, 0, 0, 0, 0, 0 , orion_bull_air_rifle , twelve_bull_air_rifle};

static void remap_target
  (
  double* x,                        // Computed X location of shot (returned)
  double* y                         // Computed Y location of shot (returned)
  )
{
  double distance, closest;        // Distance to bull in clock ticks
  double dx, dy;                   // Best fitting bullseye
  int i;
  dx = 0.0;
  dy = 0.0;

  new_target_t* ptr;               // Bull pointer
  
  if ( DLT(DLT_DIAG) )
  {
    printf("remap_target x: %4.2fmm  y: %4.2fmm", *x, *y);
  }

/*
 * Find the closest bull
 */
  if ( (json_target_type <= 1) || ( json_target_type > sizeof(ptr_list)/sizeof(new_target_t*) ) ) 
  {
    return;                         // Check for limits
  }
  ptr = ptr_list[json_target_type];
  if ( ptr == 0 )                   // Check for unassigned targets
  {
    return;
  }
  closest = 100000.0;             // Distance to closest bull
  
/*
 * Loop and find the closest target
 */
  i=0;
  while ( ptr->x != LAST_BULL )
  {
    distance = sqrt(sq(ptr->x - *x) + sq(ptr->y - *y));
    if ( DLT(DLT_DIAG) )
    {
      printf(" distance: %4.2f", distance); 
    }
    if ( distance < closest )   // Found a closer one?
    {
      closest = distance;       // Remember it
      dx = ptr->x;
      dy = ptr->y;              // Remember the closest bull
      if ( DLT(DLT_DIAG) )
      {
        printf("Target: %d   dx: %4.2f   dy: %4.2f", i, dx, dy); 
      }
    }
    ptr++;
    i++;
  }

/*
 * Remap the pellet to the centre one
 */
  *x = *x - dx;
  *y = *y - dy;
  if ( DLT(DLT_DIAG) )
  {
    printf("x: %4.2f , y: %4.2f ", *x, *y);
  }
  
/*
 *  All done, return
 */
  return;
}

 
double sq(double x) { return x*x;}