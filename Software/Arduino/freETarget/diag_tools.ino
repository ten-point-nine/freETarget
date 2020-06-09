/*----------------------------------------------------------------
 *
 * diag_tools.ino
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#include "freETarget.h"
#include "mechanical.h"
#include "gpio.h"

const char* which_one[4] = {"  <<NORTH>>  ", "  <<EAST>>  ", "  <<SOUTH>>  ", "  <<WEST>>  "};

#define RX(S,H,V) (8000 - ((sqrt(sq((H)-s[(S)].x) + sq((V)-s[(S)].y))) / 0.33 * OSCILLATOR_MHZ))



/*----------------------------------------------------------------
 *
 * void show_counters
 *
 * Read the counter registers and print the results
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/

void show_counters(void)
{
  double timer_value[4];    // Array of timer values
  int    i;
  
/*
 *  Read in the counter values 
 */
 timer_value[N] = read_counter(NORTH_HI);
 timer_value[E] = read_counter(EAST_HI);
 timer_value[S] = read_counter(SOUTH_HI);
 timer_value[W] = read_counter(WEST_HI);

/*
 * Determine the location of the reference counter (longest time)
 */
  for (i=0; i != 4; i++)
  {
    Serial.print(which_one[i]);
    Serial.print(timer_value[i]);
    Serial.println();
  }
  
  /*
   * All done return
   */
  return;
}

#if ( SAMPLE_CALCULATIONS == true )
/*----------------------------------------------------------------
 *
 * void sample_calculations()
 *
 * Setup a known target for sample calculations
 *
 *----------------------------------------------------------------
 * 
 * See excel spread sheet sample calculations.xls
 * 
 * Estimate 0.02mm / delta count   
 *   --> 400 counts -> 8mm
 *   
 *--------------------------------------------------------------*/
unsigned int sample_scores[][4] = {
/*0*/  { 0, 0, 0, 0 },                     // Unused
//        NORTH    EAST   SOUTH    WEST
/*01*/ { 0x4000, 0x4000, 0x4000, 0x4000 }, // 10.9   // Works
/*02*/ {   8250,   8250,   8000,   8000 }, // @N-E   // Works
/*03*/ {   8250,   8000,   8000,   8250 }, // @N-W   // Works
  
/*04*/ {   8000,   8000,   8250,   8250 }, // @S-W   // Works
/*05*/ {   8000,   8250,   8250,   8000 }, // @S-E

/*06*/ {   8400,   8400,   8000,   8000 }, // @N-E
/*07*/ {   8400,   8000,   8000,   8400 }, // @N-W
  
/*08*/ {   8000,   8000,   8400,   8400 }, // @S-W
/*09*/ {   8000,   8400,   8400,   8000 }, // @S-E

/*10*/ {   8000,   8000,   8900,   8900 }, // @S-W
/*11*/ {   8000,   8900,   8900,   8000 }, // @S-E

/*12*/ {   4600,   2000,   2000,   4600 }, // @S-W  MAX
/*13*/ {   2000,   4600,   4600,   2000 },  // @S-E

/*
 * PASS
 */
/*14*/ {  8278,   7268,   7262,   8260 }, // 32.13
/*15*/ {  8351,   6992,   6859,   7171 }, // 33.1
/*16*/ {  8055,   7960,   8198,   8426 }, // 6.96
/*17*/ {  6817,   8347,   7379,   6878 }, // nan  SB 4.5 @ 03:00
/*18*/ {  6676,   6682,   8208,   7796 }, // nan  SB 6.5 @ 06:00
/*19*/ {  6981,   6611,   7935,   8206 }, // nan  SB 5.5 @ 07:00

};

/*
 * Prompt the user for a test number and execute the test.
 */
void unit_test(void)
{
  unsigned int ch_h, ch_l;      // Input character
  unsigned int i, j, k;
  bool more_samples;
  unsigned int location;

  Serial.print("\n\rEnter Test Number (00 for all):");
  
  while (1)
  {
 /*
  * Auto Generate spiral
  */
   if ( (read_DIP() & SPIRAL) != 0 )
    {
    for ( k = 0; k != 60; k++)
      {
      sample_calculations(k, 0);
      location = compute_hit(shot, &history);
      rotate_shot(location, &history);  // Rotate the shot back onto the target
      send_score(&history, shot);
      shot++;
      delay(250);
      }
    for (;;);
    }

    
/*
 * Use a sample from the table
 */
  else
  {
/*
 * Poll for a test number
 */
    while ( Serial.available() != 3 )
      continue;
    ch_h = (Serial.read() - '0') & 0x0f;
    ch_l = (Serial.read() - '0') & 0x0f;
    j = (ch_h*10) + ch_l;

 /*
  * Execute the test(s)
  */
    for ( k = j; k != 1000; k++)
    {
      Serial.println();
      Serial.println("*************************");

 /*
  * Loop 4x to cover all quadrants
  */
      for (i=N; i <= W; i++)
        {      
        more_samples = sample_calculations(k, i);
        if ( more_samples )
          {
          location = compute_hit(shot, &history);
          rotate_shot(location, &history);  // Rotate the shot back onto the target
          send_score(&history, shot);
          shot++;
          }
        }
        if ( (j != 0) || (more_samples == false) )
        {
          break;
        }
    }

/*
 * Eat any characters before trying again
 */
    while ( Serial.available() != 0 )
      Serial.read();
    }
  }
}


/*
 * Fill up counters with sample values.  Return false if the sample does not exist
 */
bool sample_calculations (unsigned int sample, unsigned int rotation)
{
  double angle;
  double radius;
  double x, y;

/*
 * Generate a spiral pattern
 */
  if ( (read_DIP() & SPIRAL) != 0 )
    {
    angle = (PI_ON_4) / 5.0 * ((double)sample);
    if ( read_DIP() & PISTOL )
      {
      radius = RADIUS_PISTOL * (double)sample / 60.0;
      }
    else
      {
      radius = RADIUS_RIFLE * (double)sample / 60.0;
      }
    x = radius * cos(angle);
    y = radius * sin(angle);
    Serial.print(x); Serial.print(y); Serial.print(angle);
    timer_value[N] = RX(N, x, y);
    timer_value[E] = RX(E, x, y);
    timer_value[S] = RX(S, x, y);
    timer_value[W] = RX(W, x, y);
    return true;
    }

 /*
  * Use the samples from the table
  */
  else
    {
    if ( sample > sizeof(sample_scores) / sizeof(int) / 4 )
      return false;
    
    Serial.print("\n\rSample calculations:"); Serial.print(sample); 
  
    timer_value[N] = sample_scores[sample][(N + rotation) & 3];
    timer_value[E] = sample_scores[sample][(E + rotation) & 3];
    timer_value[S] = sample_scores[sample][(S + rotation) & 3];
    timer_value[W] = sample_scores[sample][(W + rotation) & 3];
    }
  return true;
}
#endif

