/*----------------------------------------------------------------
 *
 * diag_tools.ino
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#include "freETarget.h"
#include "mechanical.h"
#include "analog_io.h";
#include "gpio.h"

const char* which_one[4] = {"N:", "   E:", "   S: ", "   W: "};
const char  nesw[] = {"NESW"};

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define TEST_SAMPLES 500

static void show_analog_on_PC(void);
static void sample_calculations(unsigned int step);

/*----------------------------------------------------------------
 *
 * void self_test
 *
 * Execute self tests based on the jumper settings
 *
 *----------------------------------------------------------------
 *   
 *--------------------------------------------------------------*/
unsigned int tick;

void self_test(uint16_t test)
{
  double       volts;         // Reference Voltage
  unsigned int i;
  unsigned int sensor_status; // Sensor running inputs
  
/*
 *  Update the timer
 */
  tick++;
  volts = TO_VOLTS(analogRead(V_REFERENCE));
  
/*
 * Figure out what test to run
 */
  switch (test)
  {
    default:                                    // Undefined test
      json_test = 0;                            // Force to 0 
      EEPROM.put(NONVOL_TEST_MODE, json_test);  // and fall through
      
    case 0:
      Serial.print("\n\r1 - Digital inputs");
      Serial.print("\n\r2 - Counter values (external trigger)");
      Serial.print("\n\r3 - Counter values (internal trigger)");
      Serial.print("\n\r4 - Oscilloscope");
      Serial.print("\n\r5 - Oscilloscope (PC)");
      Serial.print("\n\r6 - Advance paper backer");
      Serial.print("\n\r7 - Spiral Unit Test");
      Serial.print("\n\r");
      break;

    case 1: 
      Serial.print("\n\rBD Rev:"); Serial.print(revision()); Serial.print("\n\rDIP: 0x"); Serial.print(read_DIP(), HEX);    
      Serial.print("\n\rTemperature: "); Serial.print(temperature_C()) ;  Serial.print("'C"); speed_of_sound(temperature_C());
      Serial.print("\n\rREF: "); Serial.print(volts); Serial.print("\n\r");
      for (tick=0; tick != 8; tick++)
      {
        digitalWrite(LED_S, (~tick) & 1);
        digitalWrite(LED_X, (~tick) & 2);
        digitalWrite(LED_Y, (~tick) & 4);
        delay(250);
      }
      break;

    case 2:                               // Show the timer values (Wait for analog input)
    case 3:                               // Show the timer values (Trigger input)
      if ( json_test == 3 )
      {
        Serial.print("\Clock test.  Output should be 8000 counts");
        digitalWrite(CLOCK_START, 0);
        digitalWrite(CLOCK_START, 1);     // Trigger the clocks from outside
        delay(1000);  
      }
      Serial.print("\n\rWaiting for Trigger\n\e");
      arm_counters();
      while ( !is_running() )
      {
        digitalWrite(LED_S, (~tick) & 1);
        digitalWrite(LED_X, (~tick) & 2);
        digitalWrite(LED_Y, (~tick) & 4);
        tick <<= 1;
        if ( tick & 8 )
        {
          tick = 1;
        }
        delay(100);
      }
      sensor_status = is_running();       // Remember all of the running timers
      stop_counters();
      timer_value[N] = read_counter(N);
      timer_value[E] = read_counter(E);
      timer_value[S] = read_counter(S);
      timer_value[W] = read_counter(W);
      send_timer(sensor_status);
      digitalWrite(LED_S, 0);
      digitalWrite(LED_X, 0);
      digitalWrite(LED_Y, 0);
      delay(1000);
      break;

    case 4:                             // Show the analog input
      Serial.print("\n\rOscilloscope.  Press <ENTER> 5 times to exit");
      while ( Serial.available() <= 5 ) // Need 5 characters on the serial port to end
        show_analog();                  //
      break;
      
    case 5:
      while ( Serial.available() <= 5 )
      {
        show_analog_on_PC();
      }
      break;

    case 6: 
      Serial.print("\n\rAdvanciing backer paper");
      digitalWrite(PAPER, PAPER_ON);    // Advance the backer paper
      delay(json_paper_time * 10);
      digitalWrite(PAPER, PAPER_OFF);
      break;
      
    case 7: 
      unit_test();                    // Generate a spiral
      break;
  }

 /* 
  *  All done, return;
  */
    return;
}

/*----------------------------------------------------------------
 * 
 * void show_analog()
 * 
 * Read and display as a 4 channel scope trace
 * 
 *----------------------------------------------------------------
 *
 *  The output appears as a 1 channel O'scope with all four
 *  sensors shown on the display.
 *  
 *  Tapping the microphone will be enough to trigger a response
 *  
 *  To make catching the trace easier, the input has a peak 
 *  detection and decay
 *  
 *--------------------------------------------------------------*/
unsigned int channel[] = {NORTH_ANA, EAST_ANA, SOUTH_ANA, WEST_ANA};

unsigned int max_input[4];
#define FULL_SCALE   128              // Max full scale is 128 (128 = 5V)
#define SCALE        128/1024         // Gain applied to analog input
#define DECAY_RATE   16               // Decay rate for peak detection

void show_analog(void)
{
  unsigned int i, j, k;
  char o_scope[FULL_SCALE];

  Serial.print("\n\r{\"OSCOPE\":\"");
 /*
  *  Clear the oscope line
  */
  for ( k=0; k != FULL_SCALE; k++)              // Clear the oscope
  {
    o_scope[k] = ' ';
  }

/*
 * Draw in the trip point
 */
  i = analogRead(V_REFERENCE) * SCALE;
  o_scope[i] = '|';
  
 /*
  * Put the values into the line
  */
   for (i=N; i != W + 1; i++)
   {
    if ( max_input[i] != 0 )                      // Capture the maxumum and decay it linearly
    {
      max_input[i]--;
    }
    
    j = analogRead(channel[i]) * SCALE;           // Read and scale the input
    
    if ( j > max_input[i] )                       // Remember the max
    {
      max_input[i] = j;
    }

    o_scope[max_input[i]] = nesw[i];

  }
  
  Serial.print(o_scope);  Serial.print("\"}");     // Display the trace as JSON

 /*
  * All done.
  */
  return;

}

/*----------------------------------------------------------------
 * 
 * void show_analog_on_PC()
 * 
 * Four channel scope shown on the PC
 * 
 *----------------------------------------------------------------
 *
 *  Special purpose version of the software for use on the PC test
 * .program  
 *--------------------------------------------------------------*/

static void show_analog_on_PC(void)
{
  unsigned int i, j, k;
  char o_scope[FULL_SCALE];
/*
 * Output as a scope trace
 */
  Serial.print("\n{Ref:"); Serial.print(TO_VOLTS(analogRead(V_REFERENCE))); Serial.print("  ");
  
   for (i=N; i != W + 1; i++)
  {
    Serial.print(which_one[i]);
    if ( max_input[i] != 0 )
    {
      max_input[i]--;
    }
    
    j = analogRead(channel[i]) * SCALE;           // Read and scale the input
    if ( (j * DECAY_RATE) > max_input[i] )        // Remember the max
    {
      max_input[i] = j * DECAY_RATE;
    }
          
    if ( j > FULL_SCALE-1 )
    {
      j = FULL_SCALE-1;
    }
    
    for ( k=0; k != FULL_SCALE; k++)              // Clear the oscope
    {
      o_scope[k] = ' ';
    }
    o_scope[j] = '*';                             // Put in the trace
    o_scope[(max_input[i]) / DECAY_RATE] = '#';
    o_scope[FULL_SCALE-1] = 0;
    
    Serial.print(o_scope);                        // Display this channel
  }
  Serial.print("}");

 /*
  * All done.
  */
  return;

}

/*----------------------------------------------------------------
 *
 * void  unit_test()
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

/*
 * Prompt the user for a test number and execute the test.
 */
void unit_test(void)
{
  unsigned int i;
  unsigned int location;

 /*
  * Auto Generate spiral
  */
  for ( i = 0; i != TEST_SAMPLES; i++)
  {
    sample_calculations(i);
    location = compute_hit(0x0F, i, &history, true);
    send_score(&history, i);
  }

/*
 * All done, return
 */
  return;
}

/*
 * Fill up counters with sample values.  Return false if the sample does not exist
 */
static void sample_calculations (unsigned int sample)
{
  double angle;
  double radius;
  double x, y;

/*
 * Generate a spiral pattern
 */
  angle = (PI_ON_4) / 5.0 * ((double)sample);
  radius = 0.99 * (json_sensor_dia/2.0) * (double)sample / TEST_SAMPLES;

  x = radius * cos(angle);
  y = radius * sin(angle);
  timer_value[N] = RX(N, x, y);
  timer_value[E] = RX(E, x, y);
  timer_value[S] = RX(S, x, y);
  timer_value[W] = RX(W, x, y);
  
/*
 * All done, return
 */
  return;
}
