/*----------------------------------------------------------------
 *
 * Compute_hit.ino
 *
 * Determine the score
 *
 *-----------------------------------------------------------------
 * 
 *---------------------------------------------------------------*/
#include "freETarget.h"
#include "json.h"

#define THRESHOLD (0.001)

#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */
extern const char* which_one[4];
extern int json_clock[4];

static sensor_t sensor[4];

unsigned int  pellet_calibre;     // Time offset to compensate for pellet diameter

static void remap_target(double* x, double* y);  // Map a club target if used
static void dopper_fade(shot_record_t* record, sensor_t sensor[]);      // Take care of a fading signal
static int  adjust_clocks( shot_record_t* shot, sensor_t sensor[]);     // Adjust the clocks
static void target_geometry( shot_record_t* shot, sensor_t sensor[]);   // Work out the target geometyr

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
 * This function takes the physhot_mmical location of the sensors (mm)
 * and generates the sensor array based on time. (ex us / mm)
 *--------------------------------------------------------------*/
void init_sensors(void)
{
  if ( DLT(DLT_CRITICAL) ) 
  {
    Serial.print(T("init_sensors()"));
  }
  
/*
 * Determine the speed of sound and ajust
 */
  s_of_sound = speed_of_sound(temperature_C(), json_rh);
  pellet_calibre = ((double)json_calibre_x10 / s_of_sound / 2.0d / 10.0d) * OSCILLATOR_MHZ; // Clock adjustement
  
 /*
  * Work out the geometry of the sensors.  Sensors are located at the cardinal points
  */
  sensor[N].index = N;
  sensor[N].x_tick = json_north_x / s_of_sound * OSCILLATOR_MHZ;
  sensor[N].y_tick = (json_sensor_dia /2.0d + json_north_y) / s_of_sound * OSCILLATOR_MHZ;
  sensor[N].xphys_mm = 0;         // Located at the NW corner
  sensor[N].yphys_mm = json_sensor_dia / 2.0;
  
  sensor[E].index = E;
  sensor[E].x_tick = (json_sensor_dia /2.0d + json_east_x) / s_of_sound * OSCILLATOR_MHZ;
  sensor[E].y_tick = (0.0d + json_east_y) / s_of_sound * OSCILLATOR_MHZ;
  sensor[E].xphys_mm = json_sensor_dia / 2.0d;
  sensor[E].yphys_mm = 0;
  
  sensor[S].index = S;
  sensor[S].x_tick = 0.0d + json_south_x / s_of_sound * OSCILLATOR_MHZ;
  sensor[S].y_tick = -(json_sensor_dia/ 2.0d + json_south_y) / s_of_sound * OSCILLATOR_MHZ;
  sensor[S].xphys_mm = 0;
  sensor[S].yphys_mm = -(json_sensor_dia / 2.0d);
  
  sensor[W].index = W;
  sensor[W].x_tick = -(json_sensor_dia / 2.0d  + json_west_x) / s_of_sound * OSCILLATOR_MHZ;
  sensor[W].y_tick = json_west_y / s_of_sound * OSCILLATOR_MHZ;
  sensor[W].xphys_mm = -(json_sensor_dia / 2.0d);
  sensor[W].yphys_mm = 0;
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
 * return: Sensor location used to recognize shot
 *
 *----------------------------------------------------------------
 *
 * Computing the score takes place over a number of steps
 * 
 * 1 - 
 * See freETarget documentaton for algorithm
 * 
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  shot_record_t* shot              // Storing the results
  )
{
  int           i, j, count;
  double        estimate;          // Estimated position
  int           trigger_sensor;    // Which sensor started the process
  double        last_estimate, error; // Location error
  double        x_avg, y_avg;      // Running average location in clock ticks
  double        z_offset_clock;    // Time offset between paper and sensor plane
  double        clock_to_mm;       // Conversion from clock counts to mm
  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("compute_hit()")); 
  }

/* 
 *  Check for a miss
 */
  if ( (shot->face_strike != 0) || (shot->timer_count[N] == 0) || (shot->timer_count[E] == 0) || (shot->timer_count[S] == 0) || (shot->timer_count[W] == 0 ) )
  {
    if ( DLT(DLT_DIAG) )
    {
      Serial.print(T("Miss detected: F:")); Serial.print(shot->face_strike); 
      Serial.print(T(" N:")); Serial.print(shot->timer_count[N]);
      Serial.print(T(" E:")); Serial.print(shot->timer_count[E]);
      Serial.print(T(" S:")); Serial.print(shot->timer_count[S]);
      Serial.print(T(" W:")); Serial.print(shot->timer_count[W]);
    }
    return MISS;
  }

/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  clock_to_mm = speed_of_sound(temperature_C(), json_rh) / OSCILLATOR_MHZ;
  z_offset_clock = (double)json_z_offset  * OSCILLATOR_MHZ / s_of_sound; // Clock adjustement for paper to sensor difference
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("z_offset_clock:")); Serial.print(z_offset_clock); Serial.print(T("\r\n"));
  }
  
 /* 
  *  Display the timer registers if in trace mode
  */  
  if ( DLT(DLT_DIAG) )
  { 
    for (i=N; i <= W; i++)
    {
      Serial.print(which_one[i]); Serial.print(shot->timer_count[i]); Serial.print(T(" ")); 
    }
  }

  error = 999999;                  // Start with a big error
  count = 0;
  estimate = json_sensor_dia / 2.0d * OSCILLATOR_MHZ;
  
 /*
  * Iterate to minimize the error
  */
  while (error > THRESHOLD )
  {
    doppler_fade(shot, sensor);
    trigger_sensor = adjust_clocks(shot, sensor);
    target_geometry(shot, sensor);
    
    x_avg = 0;                     // Zero out the average values
    y_avg = 0;
    last_estimate = estimate;

    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      if ( find_xy_3D(&sensor[i], estimate, z_offset_clock) )
      {
        x_avg += sensor[i].xr_tick;        // Keep the running average
        y_avg += sensor[i].yr_tick;        // Average in clocks
      }
    }

    x_avg /= 4.0d;
    y_avg /= 4.0d;
    shot->xphys_mm = x_avg * clock_to_mm;         // Write this back into the shot record
    shot->yphys_mm = y_avg * clock_to_mm;
    
    estimate = sqrt(sq(sensor[trigger_sensor].x_tick - x_avg) + sq(sensor[trigger_sensor].y_tick - y_avg));
    error = abs(last_estimate - estimate);

    if ( DLT(DLT_DIAG) )
    {
      Serial.print(T("x_avg:"));  Serial.print(x_avg);   Serial.print(T("  y_avg:")); Serial.print(y_avg); Serial.print(T(" estimate:")),  Serial.print(estimate);  Serial.print(T(" error:")); Serial.print(error);
      Serial.print(T("  shot->xphys_mm:"));  Serial.print(shot->xphys_mm);   Serial.print(T("  shot->yphys_mm:")); Serial.print(shot->yphys_mm); 
      Serial.print("\r\n");
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

  return location;
}


/*----------------------------------------------------------------
 *
 * function: dopper_fade
 *
 * brief:    Compensate for the fading signal with distance
 * 
 * return:   Compensation computed
 *
 *----------------------------------------------------------------
 * 
 * Compensate for sound attenuation over a long distance
 * 
 * For large targets the sound will attenuate between the closest and 
 * furthest sensor.  For small targets this is negligable, but for a 
 * Type 12 target the difference can be measured in microseconds.  
 * This loop subtracts an function of the distance between the closest and 
 * current sensor.
 * 
 * The value of SOUND_ATTENUATION is found by analyzing the closest and furthest traces
 * 
 * From tests, the error was 7us over a 700us delay.  
 * Since sound attenuates as the square of distance, this is 
 * 
 *----------------------------------------------------------*/
#define DOPPLER_RATIO    100.0          // Normalize everything to 100mm

static void doppler_fade
(
   shot_record_t* shot,                 // Storing the results
   sensor_t       sensor[]              // Sensor Geometry
)
{
  int i;
  double distance;                     // Shot distance in mm
  double ratio;                        // distance = 100mm 

/*
 * The count correction is a function of the squate of the distance
 */
  for (i=N; i <= W; i++)
  {
    distance = sqrt(sq(sensor[i].xphys_mm - shot->xphys_mm) + sq(sensor[i].yphys_mm - shot->yphys_mm));      // Distance between shot and sensor
    ratio = sq(distance / DOPPLER_RATIO);                     // Normalize to 100 mm ans square
    sensor[i].doppler = (int)((json_doppler * ratio) + 0.5);  // Record the correction
    if ( DLT(DLT_DIAG) )
    {
      Serial.print(T("Doppler Correction ")); Serial.print(which_one[i]); 
      Serial.print(T("\r\n   X_mm: ")); Serial.print(sensor[i].xphys_mm); Serial.print(T(" x_mm: ")); Serial.print(shot->xphys_mm); 
      Serial.print(T("\r\n   Y_mm: ")); Serial.print(sensor[i].yphys_mm); Serial.print(T(" y_mm: ")); Serial.print(shot->yphys_mm); 
      Serial.print(T("\r\n   dist: ")); Serial.print(distance); Serial.print(T(" ratio: ")); Serial.print(ratio); Serial.print(" doppler: "); Serial.print(sensor[i].doppler); 
    }
  }

/*
 * All done, return
 */
  return;
}

/*----------------------------------------------------------------
 *
 * function: adjust_clocks
 *
 * brief:    Adjust the clocks based on the doppler fading
 * 
 * return:   Index to trigger sensor
 *
 *----------------------------------------------------------------
 * 
 * Compensate for sound attenuation over a long distance
 * 
 * For large targets the sound will attenuate between the closest and 
 * furthest sensor.  For small targets this is negligable, but for a 
 * Type 12 target the difference can be measured in microseconds.  
 * This loop subtracts an function of the distance between the closest and 
 * current sensor.
 * 
 * The value of SOUND_ATTENUATION is found by analyzing the closest and furthest traces
 * 
 * From tests, the error was 7us over a 700us delay.  
 * Since sound attenuates as the square of distance, this is 
 * 
 *----------------------------------------------------------*/
static int adjust_clocks
(
   shot_record_t* shot,                 // Storing the results
   sensor_t       sensor[]              // Sensor Geometry
)
{
  int       i;
  int       largest;                    // Largest timer value
  int       trigger_sensor;             // What sensor triggered the start
  
/*
 * Find the largest count.  This is the one closest to the shot
 */
  largest = 0;
  for (i=N; i <= W; i++)
  {
    sensor[i].count = shot->timer_count[i] + sensor[i].doppler;  // Adding because sound arrived "sooner"
    if ( sensor[i].count > largest )
    {
      largest = sensor[i].count;
      trigger_sensor = i;
    }
  }

/*
 * Normalize the times so that the closest sensor is the smallest time.  Subtract the doppler fade
 */
  for (i=N; i <= W; i++)
  {
    sensor[i].count = largest - sensor[i].count;
  }

/*
 * All done, return
 */
  return trigger_sensor;
}

 /*----------------------------------------------------------------
 *
 * function: target_geometry
 *
 * brief:    Compute the target geometry based on speed of sound
 *           and doppler fade
 * 
 * return:   sensor strucure updated
 *
 *----------------------------------------------------------------
 * 
 * The timing is adjusted based on the physical assembly of the  
 * target and the adjustments based on geometry and where the
 * shot falls
 * 
 *----------------------------------------------------------*/ 
static void target_geometry
(
   shot_record_t* shot,                 // Storing the results
   sensor_t       sensor[]              // Sensor Geometry
)
{
  int i;          // Iteration counter
  
/*
 * Fill up the structure with the counter geometry
 */
  for (i=N; i <= W; i++)
  {
    sensor[i].b = sensor[i].count;
    sensor[i].c = sqrt(sq(sensor[(i) % 4].x_tick - sensor[(i+1) % 4].x_tick) + sq(sensor[(i) % 4].y_tick - sensor[(i+1) % 4].y_tick));
   }
  
  for (i=N; i <= W; i++)
  {
    sensor[i].a = sensor[(i+1) % 4].b;
  }
  
/*
 * All done, return
 */
  return;
}



/*----------------------------------------------------------------
 *
 * function: find_xy_3D
 *
 * brief: Calaculate where the shot seems to lie
 * 
 * return: TRUE if the shot was computed correctly
 *         sensor->xr/yr Rotated shot position in clocks from ctr
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
 *  In our syshot_mmtem, a is the estimate for the shot location
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
     sensor_t* sensor,      // Sensor to be operatated on
     double estimate,       // Estimated position
     double z_offset_clock  // Time difference between paper and sensor plane
     )
{
  double ae, be;            // Locations with error added
  double rotation;          // Angle shot is rotated through
  double x;                 // Temporary value
  
/*
 * It looks like we have valid data.  Carry on
 */
  x = sq(sensor->a + estimate) - sq(z_offset_clock);
  if ( x < 0 )
  {
    sq(sensor->a + estimate);
    if ( DLT(DLT_DIAG) )
    {
      Serial.print(T("sensor->a is complex, truncting"));
    }
  }
  ae = sqrt(x);                             // Dimenstion with error included
  
  x = sq(sensor->b + estimate) - sq(z_offset_clock);
  if ( x < 0 )
  {
    if ( DLT(DLT_DIAG) )
    {
      Serial.print(T("sensor->b is complex, truncting"));
    }
    sq(sensor->b + estimate);
  }
  be = sqrt(x);  

  if ( (ae + be) < sensor->c )   // Check for an accumulated round off error
    {
    sensor->angle_A = 0;         // Yes, then force to zero.
    }
  else
    {  
    sensor->angle_A = acos( (sq(ae) - sq(be) - sq(sensor->c))/(-2.0d * be * sensor->c));
    }
  
/*
 *  Compute the X,Y based on the detection sensor
 */
  switch (sensor->index)
  {
    case (N): 
      rotation = PI_ON_2 - PI_ON_4 - sensor->angle_A;
      sensor->xr_tick = sensor->x_tick + ((be) * sin(rotation));      // Relocating relative to the cardinal points
      sensor->yr_tick = sensor->y_tick - ((be) * cos(rotation));
      break;
      
    case (E): 
      rotation = sensor->angle_A - PI_ON_4;
      sensor->xr_tick = sensor->x_tick - ((be) * cos(rotation));
      sensor->yr_tick = sensor->y_tick + ((be) * sin(rotation));
      break;
      
    case (S): 
      rotation = sensor->angle_A + PI_ON_4;
      sensor->xr_tick = sensor->x_tick - ((be) * cos(rotation));
      sensor->yr_tick = sensor->y_tick + ((be) * sin(rotation));
      break;
      
    case (W): 
      rotation = PI_ON_2 - PI_ON_4 - sensor->angle_A;
      sensor->xr_tick = sensor->x_tick + ((be) * cos(rotation));
      sensor->yr_tick = sensor->y_tick + ((be) * sin(rotation));
      break;

    default:
      if ( DLT(DLT_DIAG) )
      {
        Serial.print(T("\n\nUnknown Rotation:")); Serial.print(sensor->index);
      }
      break;
  }

/*
 * Debugging
 */
  if ( DLT(DLT_DIAG) )
    {
    Serial.print(T("index:")); Serial.print(sensor->index) ; 
    Serial.print(T(" a:"));        Serial.print(sensor->a);       Serial.print(T("  b:"));  Serial.print(sensor->b);
    Serial.print(T(" ae:"));       Serial.print(ae);         Serial.print(T("  be:")); Serial.print(be);    Serial.print(T(" c:")),  Serial.print(sensor->c);
    Serial.print(T(" cos:"));      Serial.print(cos(rotation)); Serial.print(T(" sin: ")); Serial.print(sin(rotation));
    Serial.print(T(" angle_A:"));  Serial.print(sensor->angle_A); Serial.print(T("  x:"));  Serial.print(sensor->x_tick);  Serial.print(T(" y:"));  Serial.print(sensor->y_tick);
    Serial.print(T(" rotation:")); Serial.print(rotation);   Serial.print(T("  xr:")); Serial.print(sensor->xr_tick); Serial.print(T(" yr:")); Serial.print(sensor->yr_tick);
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
  shot_record_t* shot             //  record
  )
{
  double x, y;                    // Shot location in mm X, Y
  double real_x, real_y;          // Shot location in mm X, Y before remap
  double radius;
  double angle;
  unsigned int volts;
  double clock_face;
  double coeff;                   // From Alex Bird
  int    z;
  double score;
  char   str[256], str_c[10];  // String holding buffers
  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("Sending the score"));
  }

/*
 * Grab the token ring if needed
 */
  if ( json_token != TOKEN_WIFI )
  {
     while(my_ring != whos_ring)
    {
      token_take();                              // Grab the token ring
      gpt = ONE_SECOND * 2;
      while ( gpt                               // Wati up to two seconds
        && (whos_ring != my_ring) )             // Or we own the ring
      { 
        token_poll();
      }
    }
    set_LED(LED_WIFI_SEND);
  }
  
 /* 
  *  Work out the hole in perfect coordinates
  */
  x = shot->xphys_mm;         // Distance in mm
  y = shot->yphys_mm;         // Distance in mm
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(shot->yphys_mm, shot->xphys_mm) / PI * 180.0d;

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
  output_to_all(str);
  
#if ( S_SHOT )
  if ( (json_token == TOKEN_WIFI) || (my_ring == TOKEN_UNDEF))
  {
    sprintf(str, "\"shot\":%d, \"miss\":0, \"name\":\"%s\"", shot->shot_number,  namesensor[json_name_id]);
  }
  else
  {
    sprintf(str, "\"shot\":%d, \"name\":\"%d\"", shot->shot_number,  my_ring);
  }
  output_to_all(str);
  dtostrf((float)shot->shot_time/(float)(ONE_SECOND), 2, 2, str_c );
  sprintf(str, ", \"time\":%s ", str_c);
  output_to_all(str);
#endif

#if ( S_SCORE )
  if ( json_token == TOKEN_WIFI )
  {
    coeff = 9.9 / (((double)json_1_ring_x10 + (double)json_calibre_x10) / 20.0d);
    score = 10.9 - (coeff * radius);
    z = 360 - (((int)angle - 90) % 360);
    clock_face = (double)z / 30.0;
    sprintf(str, ", \"score\": %d, "\"clock\":\"%d:%d, \"  ", score,(int)clock_face, (int)(60*(clock_face-((int)clock_face))) ;
    output_to_all(str);
  }
#endif

#if ( S_XY )
  dtostrf(x, 2, 2, str_c );
  sprintf(str, ",\"x\":%s", str_c);
  output_to_all(str);
  dtostrf(y, 2, 2, str_c );
  sprintf(str, ", \"y\":%s ", str_c);
  output_to_all(str);
  
  if ( json_target_type > 1 )
  {
    dtostrf(real_x, 2, 2, str_c );
    sprintf(str, ", \"real_x\":%s ", str_c);
    output_to_all(str);
    dtostrf(real_y, 2, 2, str_c );
    sprintf(str, ", \"real_y\":%s ", str_c);
    output_to_all(str);
  }
#endif

#if ( S_POLAR )
  if ( json_token == TOKEN_WIFI )
  {
    dtostrf(radius, 4, 2, str_c );
    sprintf(str, ", \"r\":%s, ", str_c);
    output_to_all(str);
    dtostrf(angle, 4, 2, str_c );
    sprintf(str, ", \"a\":%s, ", str_c);
    output_to_all(str);
  }
#endif

#if ( S_TIMERS )
  if ( json_token == TOKEN_WIFI )
  {
    sprintf(str, ", \"N\":%d, \"E\":%d, \"S\":%d, \"W\":%d ", (int)sensor[N].count, (int)sensor[E].count, (int)sensor[S].count, (int)sensor[W].count);
    output_to_all(str);
  }
#endif

#if ( S_MISC ) 
  if ( json_token == TOKEN_WIFI )
  {
    volts = analogRead(V_REFERENCE);
    dtostrf(TO_VOLTS(volts), 2, 2, str_c );
    sprintf(str, ", \"V_REF\":%s, ", str_c);
      output_to_all(str);
    dtostrf(temperature_C(), 2, 2, str_c );
    sprintf(str, ", \"T\":%s, ", str_c);
    output_to_all(str);
    sprintf(str, ", \"VERSION\":%s ", SOFTWARE_VERSION);
    output_to_all(str);
  }
#endif

  sprintf(str, "}\r\n");
  output_to_all(str);
  
/*
 * All done, return
 */
  if ( json_token != TOKEN_WIFI )
  {
    token_give();                            // Give up the token ring
    set_LED(LED_READY);
  }
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
  shot_record_t* shot                    // record record
  )
{
  char str[256];                        // String holding buffer
  
  if ( json_send_miss != 0)               // If send_miss not enabled
  {
    return;                               // Do nothing
  }

/*
 * Grab the token ring if needed
 */
  
  if ( json_token != TOKEN_WIFI )
  {
     while(my_ring != whos_ring)
    {
      token_take();                              // Grab the token ring
      gpt = ONE_SECOND;
      while ( gpt 
        && ( my_ring == whos_ring) )
      { 
        token_poll();
      }
      set_LED(LED_WIFI_SEND);
    }
  }
  
/* 
 *  Display the results
 */
  sprintf(str, "\r\n{");
  output_to_all(str);
  
 #if ( S_SHOT )
   if ( (json_token == TOKEN_WIFI) || (my_ring == TOKEN_UNDEF))
  {
    sprintf(str, "\"shot\":%d, \"miss\":0, \"name\":\"%s\"", shot->shot_number,  namesensor[json_name_id]);
  }
  else
  {
    sprintf(str, "\"shot\":%d, \"miss\":1, \"name\":\"%d\"", shot->shot_number,  my_ring);
  }
  output_to_all(str);
  dtostrf((float)shot->shot_time/(float)(ONE_SECOND), 2, 2, str );
  sprintf(str, ", \"time\":%s ", str);
#endif

#if ( S_XY )
  if ( json_token == TOKEN_WIFI )
  { 
    sprintf(str, ", \"x\":0, \"y\":0 ");
    output_to_all(str);
  }


  
#endif

#if ( S_TIMERS )
  if ( json_token == TOKEN_WIFI )
  {
    sprintf(str, ", \"N\":%d, \"E\":%d, \"S\":%d, \"W\":%d ", (int)shot->timer_count[N], (int)shot->timer_count[E], (int)shot->timer_count[S], (int)shot->timer_count[W]);
    output_to_all(str);
    sprintf(str, ", \"face\":%d ", shot->face_strike);
    output_to_all(str);
  }
#endif

  sprintf(str, "}\n\r");
  output_to_all(str);

/*
 * All done, go home
 */
  token_give();
  set_LED(LED_READY);
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
#define DBB_H (238.0 / 3.0)              // Twelve bull Daisy BB
#define DBB_V (144.0 / 2.0)              // Twelve bull Daisy BB
new_target_t daisy_bb_rifle[]        =  { {-(DBB_H + DBB_H/2), DBB_V},  {-DBB_H/2,   DBB_V},  {DBB_H/2,   DBB_V}, {(DBB_H + DBB_H/2),   DBB_V},
                                          {-(DBB_H + DBB_H/2), 0},      {-DBB_H/2,   0},      {DBB_H/2,   0},     {(DBB_H + DBB_H/2),   0}, 
                                          {-(DBB_H + DBB_H/2), -DBB_V}, {-DBB_H/2,  -DBB_V},  {DBB_H/2,  -DBB_V}, {(DBB_H + DBB_H/2),  -DBB_V},
                                          {LAST_BULL, LAST_BULL}};

                                            
//                           0  1  2  3              4                        5              6  7  8  9  10           11                     12           13      14 (0xd)
new_target_t* ptr_list[] = { 0, 0, 0, 0, five_bull_air_rifle_74mm, five_bull_air_rifle_79mm, 0, 0, 0, 0, 0 , orion_bull_air_rifle , twelve_bull_air_rifle, 0, daisy_bb_rifle};

static void remap_target
  (
  double* x,                        // Computed X location of shot (returned)
  double* y                         // Computed Y location of shot (returned)
  )
{
  double distance, closest;        // Distance to bull in clock ticks
  double dx, dy;                   // Best fitting bullseye
  int i;
  
  new_target_t* ptr;               // Bull pointer
  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("remap_target x:")); Serial.print(*x); Serial.print(T("mm y:")); Serial.print(*y); (T("mm"));
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
      Serial.print(T(" distance:")); Serial.print(distance); 
    }
    if ( distance < closest )   // Found a closer one?
    {
      closest = distance;       // Remember it
      dx = ptr->x;
      dy = ptr->y;              // Remember the closest bull
      if ( DLT(DLT_DIAG) )
      {
        Serial.print(T("Target: ")); Serial.print(i); Serial.print(T("   dx:")); Serial.print(dx); Serial.print(T(" dy:")); Serial.print(dy); 
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
    Serial.print(T("rx:")); Serial.print(*x); Serial.print(T(" y:")); Serial.print(*y);
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
  unsigned int timer_count[4];
  
  read_timers(&timer_count[0]);
  
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
    Serial.print(T("\"")); Serial.print(nesw[i]); Serial.print(T("\":"));  Serial.print(timer_count[i]);  Serial.print(T(", "));
  }

  Serial.print(T("\"V_REF\":"));   Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));  Serial.print(T(", "));
  Serial.print(T("\"Version\":")); Serial.print(SOFTWARE_VERSION);
  Serial.print(T("}\r\n"));      



  return;
}

 
