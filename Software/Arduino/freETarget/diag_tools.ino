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
#define GRID_SIDE 21
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))

#define SPIRAL  0           // Spiral test pattern
#define GRID    1           // Grid test pattern

static void show_analog_on_PC(void);
static void unit_test(unsigned int mode);
static bool sample_calculations(unsigned int mode, unsigned int sample);

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
      Serial.print("\n\r8 - Grid calibration pattern");
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
      show_analog();                  
      break;
      
    case 5:
      show_analog_on_PC();
      break;

    case 6: 
      Serial.print("\n\rAdvanciing backer paper");
      digitalWrite(PAPER, PAPER_ON);    // Advance the backer paper
      delay(json_paper_time * 10);
      digitalWrite(PAPER, PAPER_OFF);
      break;
      
    case 7: 
      unit_test( SPIRAL );                    // Generate a spirall
      break;

    case 8:
      unit_test(GRID);
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
unsigned int cycle = 0;

unsigned int max_input[4];
#define FULL_SCALE   128              // Max full scale is 128 (128 = 5V)
#define SCALE        128/128          // Gain applied to analog input
#define DECAY_RATE   16               // Decay rate for peak detection
#define SAMPLE_TIME  (500000U)        // 500 x 1000 us

void show_analog(void)
{
  unsigned int i, sample;
  char o_scope[FULL_SCALE];
  unsigned long now;
  
  digitalWrite(LED_S, ~(1 << cycle) & 1);
  digitalWrite(LED_X, ~(1 << cycle) & 2);
  digitalWrite(LED_Y, ~(1 << cycle) & 4);
  cycle = (cycle+1) % 4;

 /*
  *  Clear the oscope line
  */
  for ( i=0; i != FULL_SCALE; i++)              // Clear the oscope
  {
    o_scope[i] = ' ';
  }
  o_scope[FULL_SCALE-1] = 0;                    // Null terminate
/*
 * Draw in the trip point
 */
  i = analogRead(V_REFERENCE) * SCALE;
  o_scope[i] = '|';

/*
 * Sample the input for 250ms 
 */
  max_input[N] = 0;
  max_input[E] = 0;                             // Forget the maxium
  max_input[S] = 0;
  max_input[W] = 0;     
  now = micros();
  while ((micros() - now) <= SAMPLE_TIME ) // Enough time already
    { 
    for (i=N; i <= W; i++)
      {
      sample = analogRead(channel[i]) * SCALE;     // Read and scale the input
      if ( sample >= FULL_SCALE -1 )
      {
        sample = FULL_SCALE-2;
      }
      if ( sample > max_input[i] )                 // Remember the max
        {
        max_input[i] = sample;
        }
      }
    }

 /*
  * Put the values into the line
  */
   for (i=N; i <= W; i++)
   {
    o_scope[max_input[i]] = nesw[i];
   }
  
  Serial.print("{\"OSCOPE\": "); Serial.print(o_scope);  Serial.print("\"}\n\r");     // Display the trace as JSON

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
void unit_test(unsigned int mode)
{
  unsigned int i;
  unsigned int location;
  unsigned int shot_number;
  
 /*
  * Auto Generate spiral
  */
  init_sensors();
  shot_number = 1;
  for ( i = 0; i != TEST_SAMPLES; i++)
  {
    if ( sample_calculations(mode, i) )
    {
    location = compute_hit(0x0F, i, &history, true);
    send_score(&history, shot_number);
    shot_number++;
    delay(500);
    }
  }

/*
 * All done, return
 */
  return;
}

/*----------------------------------------------------------------
 *
 * void  sample_calculations()
 *
 * Work out the clock values to generate a particular pattern
 *
 *----------------------------------------------------------------
 * 
 * This function is used to generate a test pattern that the
 * PC or Arduino software is compared to.
 *   
 *--------------------------------------------------------------*/
/*
 * Fill up counters with sample values.  Return false if the sample does not exist
 */
static bool sample_calculations
  (
  unsigned int mode,            // What test mode are we generating
  unsigned int sample           // Current sample number
  )
{
  double x, y;                  // Resulting target position
  double angle;                 // Polar coordinates
  double radius;
  int    ix, iy;
  double step_size;             // Rectangular coordinates
  double grid_step;
  
  switch (mode)
  {
/*
 * Generate a spiral pattern
 */
  default:
  case SPIRAL:
    angle = (PI_ON_4) / 5.0 * ((double)sample);
    radius = 0.99d * (json_sensor_dia/2.0) / sqrt(2.0d) * (double)sample / TEST_SAMPLES;

    x = radius * cos(angle);
    y = radius * sin(angle);
    timer_value[N] = RX(N, x, y);
    timer_value[E] = RX(E, x, y);
    timer_value[S] = RX(S, x, y);
    timer_value[W] = RX(W, x, y);
    break;

 /*
 * Generate a grid
 */
  case GRID:
    radius = 0.99d * (json_sensor_dia / 2.0d / sqrt(2.0d));
    grid_step = radius * 2.0d / (double)GRID_SIDE;

    ix = -GRID_SIDE/2 + (sample % GRID_SIDE);      // How many steps
    iy = GRID_SIDE/2 - (sample / GRID_SIDE);

    x = (double)ix * grid_step;
    y = (double)iy * grid_step;

    if ( sqrt(sq(x) + sq(y)) > radius )
    {
      return false;
    }

    timer_value[N] = RX(N, x, y);
    timer_value[E] = RX(E, x, y);
    timer_value[S] = RX(S, x, y);
    timer_value[W] = RX(W, x, y);
    break;   
  }
  
/*
 * All done, return
 */
  return true;
}
