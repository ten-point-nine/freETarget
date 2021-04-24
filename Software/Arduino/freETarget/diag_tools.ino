
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

static void show_analog_on_PC(int v);
static void unit_test(unsigned int mode);
static bool sample_calculations(unsigned int mode, unsigned int sample);
void set_trip_point(int v);

/*----------------------------------------------------------------
 *
 * function: void self_test
 *
 * brief: Execute self tests based on the jumper settings
 * 
 * return: None
 *
 *----------------------------------------------------------------
 *   
 *   This function is a large case statement with each element
 *   of the case statement 
 *--------------------------------------------------------------*/
unsigned int tick;
void self_test(uint16_t test)
{
  double       volts;         // Reference Voltage
  unsigned int i;
  char         ch;
  unsigned int sensor_status; // Sensor running inputs
  unsigned long sample;       // Sample used for comparison
  unsigned int random_delay;  // Random sampe time
  bool         pass;
  unsigned long start_time;   // Running time
  
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
      if ( revision() >= REV_220 )
      {
        Serial.print("\r\n 3 - Counter values (internal trigger)");
      }
      Serial.print("\r\n 4 - Oscilloscope");
      Serial.print("\r\n 5 - Oscilloscope (PC)");
      Serial.print("\r\n 6 - Advance paper backer");
      Serial.print("\r\n 7 - Spiral Unit Test");
      Serial.print("\r\n 8 - Grid calibration pattern");
      Serial.print("\r\n 9 - One time calibration pattern");
      Serial.print("\r\n 8 - Grid calibration pattern");
      if ( revision() >= REV_220 )
      {
        Serial.print("\r\n 10 - Aux port passthrough");
      }
      Serial.print("\r\n11 - Set detection trip point"); 
      Serial.print("\r\n12 - Transfer loopback");
      Serial.print("\r\n13 - Serial port test");
      Serial.print("\r\n14 - LED brightness test");
      Serial.print("\r\n15 - Face strike test");
      Serial.print("\r\n");
      break;

    case T_DIGITAL: 
      Serial.print("\r\nTime:");                      Serial.print(micros());
      Serial.print("\r\nBD Rev:");                    Serial.print(revision());       
      Serial.print("\r\nDIP: 0x");                    Serial.print(read_DIP(), HEX); 
      digitalWrite(STOP_N, 0);
      digitalWrite(STOP_N, 1);                        // Reset the fun flip flop
      Serial.print("\r\nRUN FlipFlop: 0x");           Serial.print(is_running(), HEX);   
      Serial.print("\r\nTemperature: ");              Serial.print(temperature_C());  Serial.print("'C ");
      Serial.print(speed_of_sound(temperature_C()));  Serial.print("mm/us");
      Serial.print("\r\nV_REF: ");                    Serial.print(volts); 
      Serial.print("\r\n");
      for (tick=0; tick != 8; tick++)
      {
        digitalWrite(LED_S, (~tick) & 1);
        digitalWrite(LED_X, (~tick) & 2);
        digitalWrite(LED_Y, (~tick) & 4);
        delay(250);
      }
      json_test = T_HELP;               // and stop the test
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
        if ( revision() >= REV_220 )  
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
      show_analog(0);                  
      break;
      
    case T_OSCOPE_PC:
      show_analog_on_PC(0);
      break;

    case T_PAPER: 
      Serial.print("\r\nAdvanciing backer paper "); Serial.print(json_paper_time * 10); Serial.print(" ms");
      drive_paper();
      Serial.print("\r\nDone");
      json_test = T_HELP;
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
      set_trip_point(0);              // Stay in the trip point loop
      json_test = T_HELP;
      break;

    case T_SERIAL_PORT:
      Serial.print("\r\nArduino Serial Port: Hello World\r\n");
      AUX_SERIAL.print("\r\nAux Serial Port: Hello World\r\n");
      DISPLAY_SERIAL.print("\r\nDisplay Serial Port: Hello World\r\n");
      json_test = T_HELP;
      break;

    case T_LED:
      Serial.print("\r\nRamping the LED");
      for (i=0; i != 256; i++)
      {
        analogWrite(LED_PWM, i);
        delay(20);
      }
      for (i=255; i != -1; i--)
      {
        analogWrite(LED_PWM, i);
        delay(20);
      }
      analogWrite(LED_PWM, 0);
      Serial.print(" Done\r\n");
      json_test = T_HELP;
      break;

      
    case T_FACE:
      Serial.print("\r\nFace strike test");
      face_strike = 0;
      EEPROM.put(NONVOL_TEST_MODE, T_HELP);     // Stop the test on the next boot cycle
      while (1)
      {        
        if ( face_strike != 0 )
        {
          set_LED(LED_S, true);     // If something comes in, 
          set_LED(LED_X, true);
          set_LED(LED_Y, true);     // turn on all of the LEDs
          face_strike = 0;
        }
        else
        {
          set_LED(LED_S, false);
          set_LED(LED_X, false);
          set_LED(LED_Y, false);
        }
        delay(500);
      }

      break;
  }

 /* 
  *  All done, return;
  */
    if ( json_test == T_HELP )
    {
      EEPROM.put(NONVOL_TEST_MODE, T_HELP);     // Stop the test on the next boot cycle
    }
    return;
}
  
/*----------------------------------------------------------------
 * 
 * function: POST_1()
 * 
 * brief: Show the LEDs are working
 * 
 * return: None
 * 
 *----------------------------------------------------------------
 *
 *  Cycle the LEDs to show that the board has woken up and has
 *  freETarget software in it.
 *  
 *--------------------------------------------------------------*/

 void POST_1(void)
 {
   unsigned int i;

  if ( is_trace )
  {
    Serial.print("\r\nPOST 1");
  }
  for (i=0; i !=4; i++)
  {
    digitalWrite(LED_S, ~(1 << i) & 1);
    digitalWrite(LED_X, ~(1 << i) & 2);
    digitalWrite(LED_Y, ~(1 << i) & 4);
    delay(250);
  }

  return;
 }

/*----------------------------------------------------------------
 * 
 * function: void POST_2()
 * 
 * brief: Verify the counter circuit operation
 * 
 * return: None
 * 
 *----------------------------------------------------------------
 *
 *  Trigger the counters from inside the circuit board and 
 *  read back the results and look for an expected value.
 *  
 *  Return TRUE if the complete circuit is working
 *  
 *  IMPORTANT
 *  This test will fail if the sensor cable harness is not attached
 *  
 *--------------------------------------------------------------*/
 bool POST_2(void)
 {
   unsigned int i, j;            // Iteration counter
   unsigned int random_delay;    // Delay duration
   unsigned int sensor_status;   // Sensor status
   int          x;               // Time difference (signed)

  if ( is_trace )
  {
    Serial.print("\r\nPOST 2");
  }
  
/*
 * The test only works on V2.2 and higher
 */
  if ( revision() < REV_220 )
  {
    return true;                   // Fake a positive response  
  }
  
/*
 * Do the test 5x looking for stuck bits.
 * 
 */
  digitalWrite(LED_S, 1);           // Show first test starting
  digitalWrite(LED_X, 0);
  digitalWrite(LED_Y, 1);
  delay(200);
  for (i=0; i!= 5; i++)
  {
    
/*
 *  Test 1, Trigger the circuit and make sure all of the running states are triggered
 */
    random_delay = random(1, 6000);   // Pick a random delay time in us
    stop_counters();                  // Get the circuit ready
    arm_counters();

    digitalWrite(CLOCK_START, 0);
    digitalWrite(CLOCK_START, 1);     // Trigger the clocks from the D input of the FF
    digitalWrite(CLOCK_START, 0);
    delayMicroseconds(random_delay);  // Delay a random time
  
    sensor_status = is_running();     // Remember all of the running timers
    stop_counters();

    if ( sensor_status != 0x0F )      // The circuit was triggered but not all
    {                                 // FFs latched
      return false;
    }

/*
 * Test 2. Read back the counters and make sure they match
 */
    random_delay *= 8;                // Convert to clock ticks
    for (j=N; j != (W+1); j++ )       // Check all of the counters
    {
      x = read_counter(j) - random_delay;
 
      if ( x < 0 )
      {
        x = -x;                       // Get the absolute value
      }

      if ( x > 1000 )                 // The time should be 
      {                               // Within 1000 counts.
        if ( is_trace )
        {
          Serial.print("\r\nFailed Clock Test. Counter:"); Serial.print(nesw[i]); Serial.print(" Error:"); Serial.print(x);
        }
        return false;                 // since there is delay  in
      }                               // Turning off the counters
    }
    delay(50);
  }
  
/*
 * Got here, the test completed successfully
 */
  digitalWrite(LED_S, 1);           // Show first test Ending
  digitalWrite(LED_X, 1);
  digitalWrite(LED_Y, 1);
  return true;
}
  
/*----------------------------------------------------------------
 * 
 * function: void POST_3()
 * 
 * brief: Display the trip point
 * 
 * return: None
 *----------------------------------------------------------------
 *
 *  Run the set_trip_point function once
 *  
 *--------------------------------------------------------------*/
 void POST_3(void)
 {
   if ( is_trace )
   {
    Serial.print("\r\nPOST 3");
   }
   
   set_trip_point(10);               // Show the trip point once (10 cycles)
   delay(ONE_SECOND);
   digitalWrite(LED_S, 1);           // Show test test Ending
   digitalWrite(LED_X, 1);
   digitalWrite(LED_Y, 1);
   return;
 }
 
/*----------------------------------------------------------------
 * 
 * function: set_trip_point
 * 
 * brief: Read the pot and display the voltage on the LEDs as a grey code
 * 
 * return: Potentiometer set for the desired trip point
 *----------------------------------------------------------------
 *
 *  The reference voltage is divided into 8 bands from 0.5 volt
 *  to 1.5 volts in 1/8 volt increments.
 *  
 *  This function averages the voltage over 1/2 second and
 *  determines what band the reference belongs in and displays
 *  it on the LEDs as a Grey code.
 *  
 *  The function will remain here 
 *     If started by a CAL jumper until the jumper is removed
 *     If the voltage is out of spec on power up
 *     If started by a {TEST} forever
 *     
 *  Calibration Display
 *  
 *  V_REF           S  X  Y
 *  0.350           .  .  .
 *  0.400           .  .  *
 *  0.450           .  .  B
 *  0.500           .  *  .
 *  0.550           .  B  .
 *  0.600           .  *  *
 *  0.650           .  B  B
 *  0.700           *  .  .
 *  0.750           B  .  .
 *  0.800           *  .  *
 *  0.900           B  .  B
 *  1.000           *  *  .
 *  1.100           B  B  .
 *  1.200           *  *  *
 *  1.3^^           B  B  B
 *  
 *  Calibration Modes 
 *  No Jumpers          Regular Range
 *  CAL_LOW             Reduced Detection
 *  CAL_HI              Increased Detetion
 *  
 *--------------------------------------------------------------*/
#define CT(x) (1023l * (long)(x+25) / 5000l )   // 1/16 volt = 12.8 counts
#define SPEC_RANGE   50            // Out of spec if within 50 couts of the rail
#define BLINK        0x80
#define NOT_IN_SPEC  0x40
const unsigned int volts_to_LED[] = { NOT_IN_SPEC,     1,    BLINK+1,    2,     BLINK+2,    3,    BLINK+3,    4,    BLINK+4,    5,    BLINK+5,    6,     BLINK+6,       7,      NOT_IN_SPEC };
const unsigned int mv_to_counts[] = {   CT(350),    CT(400), CT(450), CT(500),  CT(550), CT(600), CT(650), CT(700), CT(750), CT(800), CT(900), CT(1000), CT(1100), CT(1200)};

void set_trip_point
  (
  int pass_count                                            // Number of passes to allow before exiting (0==infinite)
  )
{
  unsigned long start_time;                                 // Starting time of average loop 
  unsigned long sample;                                     // Counts read from ADC
  unsigned int  blink;                                      // Blink the LEDs on an over flow
  unsigned int  start_DIP;                                  // Starting value of the DIP switch
  bool          not_in_spec;                                // Set to true if the input is close to the limits
  
  if ( pass_count == 0 )                                    // Infinite number of passes?
  {
    Serial.print("\r\nSetting trip point. Type ! of cycle power to exit\r\n");
  }
  blink = 0;
  not_in_spec = true;                                      // Start off by assuming out of spec

/*
 * Loop forever and display the voltage as a grey code
 */
  start_DIP = read_DIP();
  while ( not_in_spec                                       // Out of tolerance
          ||   (((start_DIP | read_DIP()) & CALIBRATE) != 0) ) // Started by DIP switch
  {
    start_time = millis();
    sample = 0;
    i=0;
    while ( millis() - start_time < (ONE_SECOND/10) )       // Read voltage for 1/10 second
    {
      sample += analogRead(V_REFERENCE);                    // Read the ADC. 0-1023V
      i++;                                                  // and keep a running total
    }
    sample /= i;                                            // Get the average


/*
 *  See if we are on one of the limits and out of spec.
 */
    if ( (sample < SPEC_RANGE)                              // Close to 0
       || ( sample > (MAX_ANALOG - SPEC_RANGE)) )           // Near VCC 
    {                                                       // Blink the LEDs * - x - *
      if ( is_trace )
      {
        Serial.print("\n\rOut Of Spec: "); Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
      }
      digitalWrite(LED_S, 1); digitalWrite(LED_X, 0); digitalWrite(LED_Y, 1);
      delay(ONE_SECOND/10);
      digitalWrite(LED_S, 0); digitalWrite(LED_X, 1); digitalWrite(LED_Y, 0);
      delay(ONE_SECOND/10);
      pass_count = 0;                                       // Stay in this function forever
      continue;
    }

 /*
  * In spec, display the trip level on the LEDs
  */
    switch (read_DIP() & ( CAL_LOW + CAL_HIGH ) )
    {   
      case (CAL_LOW):                                       // Low Calibration 
        sample *= 3;
        sample /= 2;
        break;
        
      case (CAL_HIGH):                                      // Set scale if the high range
        sample *= 2;
        sample /= 3;
        break;
     
      default:
        break;
    }
    
 /*
  * Determine what band it belongs to 
  */
   i = 0;
   while (volts_to_LED[i] != 255)
   {
     if ( sample <= mv_to_counts[i] )
     {
      break;
     }
     i++;
   }

 /*
  * Use the band to find the LEDs and if they should blink
  */
   blink = ~blink;                        // Toggle the blink
   ch = volts_to_LED[i];
   if ( ch & BLINK )                      // Blink bit on?
   {
     ch ^= blink;
     ch &= ~BLINK; 
   }
   if ( volts_to_LED[i] == NOT_IN_SPEC )
   {
    ch = blink;
   }
   ch = ~ch;

   digitalWrite(LED_S, ch & 4); digitalWrite(LED_X, ch & 2); digitalWrite(LED_Y, ch & 1);

/*
 * Got to the end.  See if we are going to do this for a fixed time or forever
 */
   while ( Serial.available() )       // If there is a 
   {
    if ( Serial.read() == '!' )        // ! waiting in the serial port
    {
      Serial.print("\r\nExiting calibration\r\n");
      return;
    }
   }
   
   if ( pass_count != 0 )             // Set for a finite loop?
   {
      pass_count--;                   // Decriment count remaining
      if ( pass_count == 0 )
      {
        return;                       // Bail out when the count is done
      }
   }
   else
   {
     Serial.print("\r\nV_Ref: "); Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
   }
   delay(ONE_SECOND/10);
 }

 /*
  * Return
  */
  return;
}

/*----------------------------------------------------------------
 * 
 * function: show_analog
 * 
 * brief: Read and display as a 4 channel scope trace
 * 
 * return: None
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

void show_analog(int v)
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
  
  Serial.print("{\"OSCOPE\": "); Serial.print(o_scope);  Serial.print("\"}\r\n");     // Display the trace as JSON

 /*
  * All done.
  */
  return;

}

/*----------------------------------------------------------------
 * 
 * function: show_analog_on_PC
 * 
 * brief: Four channel scope shown on the PC
 * 
 * return: None
 *----------------------------------------------------------------
 *
 *  Special purpose version of the software for use on the PC test
 * .program  
 *--------------------------------------------------------------*/

static void show_analog_on_PC(int v)
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
 * function: unit_test
 *
 * brief: Setup a known target for sample calculations
 * 
 * return: None
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
    sensor_status = 0xF;
    send_score(&history, shot_number, sensor_status);
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
 * function: sample_calculations
 *
 * brief: Work out the clock values to generate a particular pattern
 *
 * return: TRUE to be compatable with other calcuation functions
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
