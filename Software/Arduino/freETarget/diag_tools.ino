/*----------------------------------------------------------------
 *
 * diag_tools.ino
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#include "freETarget.h"
#include "mechanical.h"
#include "analog_io.h"
#include "gpio.h"
#include "diag_tools.h"

const char* which_one[4] = {"North: ", "East: ", "South: ", "West: "};

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define GRID_SIDE 25                              // Should be an odd number
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))
#define OVER_TRIP (0.025)                         // Trip point +/- 25mV

static void show_analog_on_PC(void);
static void unit_test(unsigned int mode);
static bool sample_calculations(unsigned int mode, unsigned int sample);

void set_trip_pt(void){self_test(T_SET_TRIP);}

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
  char         ch;
  unsigned int sensor_status; // Sensor running inputs
  unsigned int sample;        // Sample used for comparison
  unsigned int random_delay;  // Random sampe time
  bool         pass;
  
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
      
    case T_HELP:
      Serial.print("\r\n 1 - Digital inputs");
      Serial.print("\r\n 2 - Counter values (external trigger)");
      if ( revision() >= REV_22 )
      {
        Serial.print("\r\n 3 - Counter values (internal trigger)");
      }
      Serial.print("\r\n 4 - Oscilloscope");
      Serial.print("\r\n 5 - Oscilloscope (PC)");
      Serial.print("\r\n 6 - Advance paper backer");
      Serial.print("\r\n 7 - Spiral Unit Test");
      Serial.print("\r\n 8 - Grid calibration pattern");
      if ( revision() >= REV_22 )
      {
        Serial.print("\r\n 9 - Aux port passthrough");
      }
      Serial.print("\r\n10 - Set detection trip point"); 
      Serial.print("\r\n11 - BOSS-minion loopback test"); 
      Serial.print("\r\n");
      break;

    case T_DIGITAL: 
      Serial.print("\r\nBD Rev:");                    Serial.print(revision());       Serial.print("\r\nDIP: 0x"); Serial.print(read_DIP(), HEX);    
      Serial.print("\r\nTemperature: ");              Serial.print(temperature_C());  Serial.print("'C ");
      Serial.print(speed_of_sound(temperature_C()));  Serial.print("mm/us");
      Serial.print("\r\nV_REF: "); Serial.print(volts); 
      Serial.print("\r\n");
      for (tick=0; tick != 8; tick++)
      {
        digitalWrite(LED_S, (~tick) & 1);
        digitalWrite(LED_X, (~tick) & 2);


        
        digitalWrite(LED_Y, (~tick) & 4);
        delay(250);
      }
      break;

    case T_TRIGGER:                       // Show the timer values (Wait for analog input)
      Serial.print("\r\nWaiting for Trigger\r\n");
    case T_CLOCK:                        // Show the timer values (Trigger input)
      stop_counters();
      arm_counters();

      digitalWrite(LED_S, 0);
      digitalWrite(LED_X, 1);
      digitalWrite(LED_Y, 1);
      
      if ( json_test == T_CLOCK )
      {
        if ( revision() >= REV_22 )  
        {
          random_delay = random(1, 6000);   // Pick a random delay time in us
          Serial.print("\r\nRandom clock test: "); Serial.print(random_delay); Serial.print("us. All outputs must be the same. ");
          digitalWrite(CLOCK_START, 0);
          digitalWrite(CLOCK_START, 1);     // Trigger the clocks from the D input of the FF
          digitalWrite(CLOCK_START, 0);
          delayMicroseconds(random_delay);  // Delay a random time
        }
        else
        {
          Serial.print("\r\nThis test not supported on this hardware revision");
          json_test = 0;
          break;
        }
      }
  
      while ( !is_running() )
      {
        continue;
      }
      sensor_status = is_running();       // Remember all of the running timers
      stop_counters();
      timer_value[N] = read_counter(N);
      timer_value[E] = read_counter(E);
      timer_value[S] = read_counter(S);
      timer_value[W] = read_counter(W); // Read the counters
      if ( json_test == T_CLOCK )       // Test the results
      {
        sample = timer_value[N];
        pass = true;

        for(i=N; i<=W; i++)
        {
          if ( timer_value[i] != sample )
          {
            pass = false;                 // Make sure they all match
          }
        }

        if ( pass == true )
        {
          Serial.print(" PASS\r\n");
        }
        else
        {
          Serial.print(" FAIL\r\n");
        }
      }
      send_timer(sensor_status);
      
      digitalWrite(LED_S, 1);
      digitalWrite(LED_X, 1);
      digitalWrite(LED_Y, 0);
      delay(1000);
      break;

    case T_OSCOPE:                       // Show the analog input
      show_analog();                  
      break;
      
    case T_OSCOPE_PC:
      show_analog_on_PC();
      break;

    case T_PAPER: 
      Serial.print("\r\nAdvanciing backer paper "); Serial.print(json_paper_time * 10); Serial.print(" ms");
      digitalWrite(PAPER, PAPER_ON);    // Advance the backer paper
      delay(json_paper_time * 10);
      digitalWrite(PAPER, PAPER_OFF);
      json_test = 0;                    // Turn off this test
      Serial.print("\r\nDone");
      break;
      
    case T_SPIRAL: 
      Serial.print("\r\nSpiral Calculation\r\n");
      unit_test( T_SPIRAL );            // Generate a spiral
      json_test = T_HELP;               // and stop the test
      break;

    case T_GRID:
      Serial.print("\r\nGrid Calculation\r\n");
      unit_test( T_GRID);               // Generate a grid
      json_test = T_HELP;               // and stop the test
      break;  
      
    case T_ONCE:
      Serial.print("\r\nSingle Calculation\r\n");
      unit_test( T_ONCE);               // Generate a SINGLE calculation
      json_test = T_HELP;               // and stop the test
      break;
        
    case T_PASS_THRU:
      Serial.print("\r\nPass through active.  Cycle power to exit\r\n");
      while (1)
      {
        if ( Serial.available() )
        {
          ch = Serial.read(); AUX_SERIAL.print(ch);
        }
        if ( AUX_SERIAL.available() )
        {
          ch = AUX_SERIAL.read(); Serial.print(ch);
        }
      }
      break;

    case T_SET_TRIP:
      Serial.print("\r\nSetting trip point. @"); Serial.print(json_trip_point); Serial.print("mV. Cycle power to exit\r\n");
      while (1)
      {
        volts = TO_VOLTS(analogRead(V_REFERENCE));             // Read the DAC. 0-5V
        volts = volts -(((double)json_trip_point) / 1000.0d);   // Subtract the trip point
        if ( volts >= OVER_TRIP )
        {
          digitalWrite(LED_S, 0); digitalWrite(LED_X, 1);      digitalWrite(LED_Y, 1);
        }
        else if ( volts <= (-OVER_TRIP ) )
        {
          digitalWrite(LED_S, 1); digitalWrite(LED_X, 1);      digitalWrite(LED_Y, 0);
        }
        else
        {
          digitalWrite(LED_S, 1); digitalWrite(LED_X, 0);      digitalWrite(LED_Y, 1);
        }
      }
      break;

   case T_XFR_LOOP:
      if ( read_DIP() & BOSS )              // Boss side
      {
        Serial.print("\r\nBOSS loopback.\r\nType %%%% to start MINION.  \r\nCycle power to exit\r\n");
        while (1)
        {
          if ( Serial.available() )           // Read the console and send it 
          {
            ch = Serial.read();
            MINION_SERIAL.print(ch);
          }
          if ( MINION_SERIAL.available() )    // Read the minon port
          {
            ch = MINION_SERIAL.read();
            Serial.print(ch);               
          }
        }
      }
      else
      {   
        Serial.print("\r\nMINION loopback.  \r\vEnter characters from BOSS port. \r\nCycle power to exit\r\n");
        while (1)
        {
         if ( MINION_SERIAL.available() )    // Read the minon port
          {
            ch = MINION_SERIAL.read();
            Serial.print(ch);               
            ch++;
            MINION_SERIAL.print(ch);         // add one and send it back
          }
        }
      }
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
  char nesw[]="NESW";
  
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
  
  Serial.print("{\"OSCOPE\": "); Serial.print(o_scope);  Serial.print("\"}\r\n");     // Display the trace as JSON

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
static void unit_test(unsigned int mode)
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
    location = compute_hit(0x0F, &history, true);
    send_score(&history, shot_number);
    shot_number++;
    delay(200);
    }
    if ( mode == T_ONCE )
    {
      break;
    }
  }

/*
 * All done, return
 */
  json_test = 0;          // Stop the test
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
  double polar;
  int    ix, iy;
  double step_size;             // Rectangular coordinates
  double grid_step;
  
  switch (mode)
  {
/*
 * Generate a single calculation
 */
  case T_ONCE:
    angle = 0;
    radius =json_sensor_dia / sqrt(2.0d) / 2.0d;
    
    x = radius * cos(angle);
    y = radius * sin(angle);
    timer_value[N] = RX(N, x, y);
    timer_value[E] = RX(E, x, y);
    timer_value[S] = RX(S, x, y);
    timer_value[W] = RX(W, x, y);
    timer_value[W] -= 200;              // Inject an error into the West sensor

    Serial.print("\r\nResult should be: ");   Serial.print("x:"); Serial.print(x); Serial.print(" y:"); Serial.print(y); Serial.print(" radius:"); Serial.print(radius); Serial.print(" angle:"); Serial.print(angle * 180.0d / PI);
    break;

 /*
 * Generate a spiral pattern
 */
  default:
  case T_SPIRAL:
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
  case T_GRID:
    radius = 0.99d * (json_sensor_dia / 2.0d / sqrt(2.0d));                      
    grid_step = radius * 2.0d / (double)GRID_SIDE;

    ix = -GRID_SIDE/2 + (sample % GRID_SIDE);      // How many steps
    iy = GRID_SIDE/2 - (sample / GRID_SIDE);

    x = (double)ix * grid_step;                     // Compute the ideal X-Y location
    y = (double)iy * grid_step;
    polar = sqrt(sq(x) + sq(y));
    angle = atan2(y, x) - (PI * json_sensor_angle / 180.0d);
    x = polar * cos(angle);
    y = polar * sin(angle);                        // Rotate it through the sensor position.

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
