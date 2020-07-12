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

const char* which_one[4] = {"N:", "   E:", "   S: ", "   W: "};

#define RX(Z,X,Y) (8000 - (sqrt(sq(((X)  / s_of_sound * OSCILLATOR_MHZ )-s[(Z)].x) + sq(((Y) / s_of_sound * OSCILLATOR_MHZ)-s[(Z)].y))))
#define TEST_SAMPLES 500

/*----------------------------------------------------------------
 *
 * void self_test
 *
 * Execute self tests based on the jumper settings
 *
 *----------------------------------------------------------------
 * 
 * TEST 0 - Flash LEDs
 * 
 * 
 * Estimate 0.02mm / delta count   
 *   --> 400 counts -> 8mm
 *   
 *--------------------------------------------------------------*/
unsigned int tick;

void self_test(void)
{
  unsigned int dip;

  dip = (read_DIP() & 0x0E) >> 1;
  
/*
 *  Update the timer
 */
  tick++;

/*
 * Figure out what test to run
 */
  switch (dip)
  {
    case 0:           // Jumpers x-x-x  Flash LEDs and show revision
      Serial.print("\nBD Rev:"); Serial.print(revision());     
      Serial.print("   Temperature: "); Serial.print(temperature_C()) ;  Serial.print("'C");
      digitalWrite(LED_S, (~tick) & 1);
      digitalWrite(LED_X, (~tick) & 2);
      digitalWrite(LED_Y, (~tick) & 4);
      delay(500);
      break;
      
    case 1:           // Jumpers x-x-I  Show the cunter registes
      arm_counters();
      stop_counters();
      show_counters();
      delay(500);
      break;

    case 2:         // Jumpers x-I-x   Display analog voltages as a scope trace
      show_analog();                      // Display the sensor inputs
      break;
      
    case 3:

      break;

    case 4:
      break;

    case 5:

      break;

    case 6:
      break;
      
    case 7:

      break;
  }

 /* 
  *  All done, return;
  */
    return;
}

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
  unsigned int timer_value[4];    // Array of timer values
  int    i;
  
/*
 *  Read in the counter values 
 */
 timer_value[N] = read_counter(N);
 timer_value[E] = read_counter(E);
 timer_value[S] = read_counter(S);
 timer_value[W] = read_counter(W);

/*
 * Determine the location of the reference counter (longest time)
 */
  Serial.println();
  for (i=0; i != 4; i++)
  {
    Serial.print(which_one[i]);
    Serial.print(timer_value[i]);
  }


  /*
   * All done return
   */
  return;
}

/*----------------------------------------------------------------
 * 
 * void show_analog()
 * 
 * Read and display as a 4 channel scope trace
 * 
 *--------------------------------------------------------------*/
unsigned int channel[] = {NORTH_ANA, EAST_ANA, SOUTH_ANA, WEST_ANA};
#define SCALE         0.1        // Multiply volts by 64
#define FULL_SCALE    20        // Limit to 16 values

void show_analog(void)
{
  unsigned int i, j, k;

/*
 * Output as a scope trace
 */
  Serial.print("\nRef:"); Serial.print(TO_VOLTS(analogRead(V_REFERENCE))); Serial.print("  ");
  
   for (i=N; i != W + 1; i++)
  {
    Serial.print(which_one[i]);
    j = analogRead(channel[i]) * SCALE;
    if ( j > FULL_SCALE-1 )
    {
      j = FULL_SCALE-1;
    }
    for ( k=0; k != FULL_SCALE; k++)
    {
      if ( k == j )
        Serial.print("*");
      else
        Serial.print(" ");
    }
  }

 /*
  * All done.
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

//  Serial.print("\n\rEnter Test Number (00 for all):");
  
  while (1)
  {
 /*
  * Auto Generate spiral
  */
   if ( (read_DIP() & SPIRAL) != 0 )
    {
    for ( k = 0; k != TEST_SAMPLES; k++)
      {
      sample_calculations(k, 0);
      location = compute_hit(shot, &history);
      send_score(&history, shot);
      shot++;
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
      radius = 0.99 * RADIUS_PISTOL * (double)sample / TEST_SAMPLES;
      }
    else
      {
      radius = RADIUS_RIFLE * (double)sample / TEST_SAMPLES;
      }
    x = radius * cos(angle);
    y = radius * sin(angle);
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
      
    timer_value[N] = sample_scores[sample][(N + rotation) & 3];
    timer_value[E] = sample_scores[sample][(E + rotation) & 3];
    timer_value[S] = sample_scores[sample][(S + rotation) & 3];
    timer_value[W] = sample_scores[sample][(W + rotation) & 3];
    }
  return true;
}
#endif
