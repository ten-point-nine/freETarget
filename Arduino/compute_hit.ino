/*----------------------------------------------------------------
 *
 * Compute_hit
 *
 * Determine the score
 *
 *----------------------------------------------------------------
 *
 */
unsigned int step;


#define NOT_FOUND -1000     // No minimum found

#define SEC_PER_COUNT (1.0/4000000.0) // 4MHz count

#define N 0
#define E 1
#define S 2
#define W 3

/*
 *  Variables
 */
struct sensor
{
  double xa;
  double ya;   // Location of reference sensor
  double xb;
  double yb;
  double tb;   // Location and distance of first sensor
  double xc;
  double yc;
  double tc;   // Location and distance of second sensor
  double x;
  double y;    // Computed location of hit
  double error;
};


struct sensor s[3];
void find_xy(unsigned int index);

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
 * void compute_hit()
 *
 * determine the location of the hit
 *
 *----------------------------------------------------------------
 *
 * See http://eecs.ucf.edu/seniordesign/fa2009sp2010/g13/files/ATD%20NewUpdate.pdf
 *
 * For triangulation algorithm details
 *
 * There are four sensors, so one (the shortest time) is the reference
 * to the remaining 3.
 *
 * In this example,
 *
 *                         (N)
 *
 *
 *                        (hit)
 *              tw                          te
 *     (W)                  d                     (E)
 *
 *
 *  The physical locations (N) (E) (W) are known, and the times
 *  tw and te are known relative to the (hit)
 *
 *  Using the speed of sound to determine dw and de, the location
 *  of the hit can be computed using the cosine law
 *
 *  As in this example, (N) is the reference sensor, we can
 *  compute three possible locations for (hit)
 *        (N)-(W)-(E)
 *        (N)-(W)-(S)
 *        (N)-(E)-(S)
 *
 *  The output of this function is the average of all three
 *  Calculations
 *--------------------------------------------------------------*/

void compute_hit(void)
{
  double reference;        // Time of reference counter
  int    location;         // Sensor chosen for reference location
  double north_t, east_t, south_t, west_t;// Distance in mm 
  int    i, j;
  
  /*
   * Determine the location of the reference counter (longest time)
   */
  reference = north;
  location  = N;

  if ( east > reference )
  {
    reference = east;
    location = E;
  }

  if ( south > reference )
  {
    reference = south;
    location = S;
  }

  if ( west > reference )
  {
    reference = west;
    location = W;
  }

/*
 * Convert from counts to seconds.  Make sure the time is non-zero
 */
  north_t = east_t = south_t = west_t = -1.0;
  if ( north != 0 )
  {
    north_t = (double)(reference - north) * (CLOCK_PERIOD);        // Time in seconds
  }

  if ( east != 0 )
  {
    east_t  = (double)(reference - east) * (CLOCK_PERIOD);
  }

  if ( south != 0 )
  {
    south_t = (double)(reference - south) * (CLOCK_PERIOD);
  }

  if ( west_t != 0 )
  {
    west_t  = (double)(reference - west) * (CLOCK_PERIOD);
  }
#if ( TRACE_COUNTERS == true )
Serial.print("\n\rTime: "); show_location(NORTH); Serial.print(north_t), Serial.print(" "); Serial.print(east_t), Serial.print(" "); Serial.print(south_t), Serial.print(" "); Serial.print(west_t), Serial.print("\n\r"); 
#endif

/*
 * Knowing the reference, put the values into the working array
 * 
 * Example.  North is the reference microphone.
 *           Distance ref to west is known
 *           Distance ref to east is known
 *           Distance West to Wast is known
 */
  switch (location)
  {
    case N:                                               // North is the reference point (unknown)
      s[0].xa = NX; s[0].ya = NY;                         // North - West - East Triangle (West is base point)
      s[0].xb = WX; s[0].yb = WY; s[0].tb = west_t;
      s[0].xc = EX; s[0].yc = EY; s[0].tc = east_t;

      s[1].xa = NX; s[1].ya = NY;                         // North - West - South Triangle (South is base point)
      s[1].xb = WX; s[1].yb = WY; s[1].tb = west_t;
      s[1].xc = SX; s[1].yc = SY; s[1].tc = south_t;

      s[2].xa = NX; s[2].ya = NY;                         // Nort - East - South Triangle (South is base point)
      s[2].xb = EX; s[2].yc = EY; s[2].tb = east_t;
      s[2].xc = SX; s[2].yc = SY; s[2].tc = south_t;       


      break;

    case E:  
      s[0].xa = EX; s[0].ya = EY;
      s[0].xb = WX; s[0].yb = WY; s[0].tb = west_t;
      s[0].xc = NX; s[0].yc = NY; s[0].tc = north_t;
      
      s[1].xa = EX; s[1].ya = EY;
      s[1].xb = WX; s[1].yb = WY; s[1].tb = west_t;
      s[1].xc = SX; s[1].yc = SY; s[1].tc = south_t;

      s[2].xa = EX; s[2].ya = EY;
      s[2].xb = SX; s[2].yb = SY; s[2].tb = south_t;
      s[2].xc = NX; s[2].yc = NY; s[2].tc = north_t;

      break;

    case S:  
      s[0].xa = SX; s[0].ya = SY;
      s[0].xb = EX; s[0].yb = EY; s[0].tb = east_t;
      s[0].xc = NX; s[0].yc = NY; s[0].tc = north_t;

      s[1].xa = SX; s[1].ya = SY;
      s[1].xb = EX; s[1].yb = EY; s[1].tb = east_t;
      s[1].xc = WX; s[1].yc = WY; s[1].tc = west_t;

      s[2].xa = SX; s[2].ya = SY;
      s[2].xb = NX; s[2].yb = NY; s[2].tb = north_t;
      s[2].xc = WX; s[2].yc = WY; s[2].tc = west_t;

      break;

    case W:  
      s[0].xa = WX; s[0].ya = WY;
      s[0].xb = EX; s[0].yb = EY; s[0].tb = east_t;
      s[0].xc = NX; s[0].yc = NY; s[0].tc = north_t;

      s[1].xa = WX; s[1].ya = WY;
      s[1].xb = EX; s[1].yb = EY; s[1].tb = east_t;
      s[1].xc = SX; s[1].yc = SY; s[1].tc = south_t;

      s[2].xa = WX; s[2].ya = WY;
      s[2].xb = NX; s[2].yb = NY; s[2].tb = north_t;
      s[2].xc = SX; s[2].yc = SY; s[2].tc = south_t;

      break;
  }

  /*
   * Use the cosine law to compute the individual X & Y of the hit
   */

  find_xy(0);
  find_xy(1);
  find_xy(2);
  Serial.print("\n\rX0: "); Serial.print(s[0].x);  Serial.print(" Y:");  Serial.print(s[0].y); 
  Serial.print("\n\rX1: "); Serial.print(s[1].x);  Serial.print(" Y:");  Serial.print(s[1].y); 
  Serial.print("\n\rX2: "); Serial.print(s[2].x);  Serial.print(" Y:");  Serial.print(s[2].y); 


  /*
   * Average out the X & Y
   */
  hit_x = 0.0;
  hit_y = 0.0;
  j = 0;
  for (i=0; i != 3; i++)
  {
    if ( s[i].error != 1000 ) 
    {
      hit_x += s[i].x;
      hit_y += s[i].y;
      j++;
    }
  }
  
  if ( j != 0 )
  {
    hit_x /= j;
    hit_y /= j;
  }
  
Serial.print("\n\rX: "); Serial.print(hit_x); Serial.print(" Y:"); Serial.print(hit_y); Serial.print(" D:"); Serial.print(sqrt(sq(hit_x)+sq(hit_y)));

  /*
   * All done return
   */

  return;
}
/*----------------------------------------------------------------
 *
 * double distance()
 *
 * Determine distance from estimate to the actual
 *
 *----------------------------------------------------------------
 *
 *  See:
 *  http://eecs.ucf.edu/seniordesign/fa2009sp2010/g13/files/ATD%20NewUpdate.pdf
 *  
 *--------------------------------------------------------------*/
double f1(unsigned int index, double x, double y)
{
  return abs(sqrt(sq(x - s[index].xb) + sq(y - s[index].yb)) - sqrt(sq(x - s[index].xa) + sq(y - s[index].ya)) - (s[index].tb * speed_of_sound(20)));
}

double f2(unsigned int index, double x, double y)
{
  return abs(sqrt(sq(x - s[index].xc) + sq(y - s[index].yc)) - sqrt(sq(x - s[index].xa) + sq(y - s[index].ya)) - (s[index].tc * speed_of_sound(20)));
}

unsigned int b[WIDTH], c[WIDTH];
unsigned int grid_line;
#define GRID_STEP 4

void find_xy(unsigned int index)
{
  double error;
  double x, y;
  unsigned int i, j, k;
  char ch_b, ch_c;
  unsigned int small_1, small_2;

/*
 * Determine the curve whenre the error is minimized
 */
  for (i = left; i != right; i++)                        // Scan from the top down
  {
    y = d[i];
#if (SHOW_PLOT == true )
    Serial.print("\n\r");
        if ( i == WIDTH/2)
      for(j=left; j!= right; j++) Serial.print('-');
    Serial.print(y);
#endif

    b[i] = NOT_FOUND;
    c[i] = NOT_FOUND;
    small_1 = 1.0;
    small_2 = 1.0;

    for ( j=left; j != right; j++)                       // Scan from left to right
    {
#if ( SHOW_PLOT == true )
     if ( j == WIDTH/2 )
      Serial.print(y);
#endif
     x = -d[j];
     error = f1(index, x, y);
     ch_b = ' ';
     if ( error < small_1  ) {ch_b = '*'; small_1 = error; b[i] = j;}
     
     error = f2(index, x, y);
     ch_c = ' ';
     if ( error < small_2  ) {ch_b = '+'; small_2 = error;  c[i] = j; }
#if (SHOW_PLOT == true )
     Serial.print(ch_b); Serial.print(ch_c);
#endif
    }      
  }

/*
 * Go back and determine where the two lines cross (get close)
 */
  small_1 = 1000;
  for (i = left; i != right; i++)
  {
    if ( (b[i] != NOT_FOUND) && (c[i] != NOT_FOUND) )
    {
      if ( (abs(b[i] - c[i])) < small_1 )
      {
        small_1 = abs(b[i] - c[i]);
        j = (b[i] + c[i])/2.0;
        k = i;
      }
    }
  }
  

 
/*
 * Fill in the location
 */
  s[index].x = s[index].y = RADIUS;
  s[index].error = small_1;
  if ( small_1 != 1000 )
  {
    s[index].y = d[k];
    s[index].x = -d[j];
  }
  Serial.print("\n\rX:"); Serial.print(s[index].x); Serial.print("  Y:"); Serial.print(s[index].y); Serial.print(" e:"); Serial.print(small_1);
  return;

}


/*----------------------------------------------------------------
 *
 * void compute_score(void)
 *
 * Calulate the score based on the point of impact
 *
 *----------------------------------------------------------------
 * 
 * From targettalk.org, the decimal score is
 * 
 * 10.9 - (0.1 * radius / 0.25)
 *    
 *--------------------------------------------------------------*/

double compute_score(void)
{
  double radius;

  radius = sqrt(sq(hit_x) + sq(hit_y));

  score = 10.9 - (0.1 * (radius / 0.25) );

  if ( score < 0 )
    score = 0;

  return score;
}

/*----------------------------------------------------------------
 *
 * void send_score(void)
 *
 * Send the score out over the serial port
 *
 *----------------------------------------------------------------
 * 

 *    
 *--------------------------------------------------------------*/

void send_score(void)
{
  Serial.print("\n\rShot:");
  Serial.print(shot_count);
  Serial.print(", ");

  Serial.print("Score:");
  Serial.print(compute_score(), 1);
  Serial.print(", ");

  Serial.print("DX:");
  Serial.print(hit_x, 1);
  Serial.print(", ");
  
  Serial.print("DY:");
  Serial.print(hit_y, 1);
  Serial.print(", ");

  Serial.println();

  return;
}



/*----------------------------------------------------------------
 *
 * void show_location(location)
 *
 * Print out the location
 *
 *----------------------------------------------------------------
 * 

 *    
 *--------------------------------------------------------------*/

void show_location(unsigned int location)
{
  switch (location)
  {
     case NORTH: Serial.print("<< NORTH >>  "); break;
     case EAST: Serial.print( "<<  EAST >>  "); break;
     case SOUTH: Serial.print("<< SOUTH >>  "); break;
     case WEST: Serial.print( "<<  WEST >>  "); break;   
  }

  return;
}
