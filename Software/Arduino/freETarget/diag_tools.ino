
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
#include "json.h"

const char* which_one[4] = {"North:", "East:", "South:", "West:"};

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define GRID_SIDE 25                              // Should be an odd number
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))


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
      json_test = 0;                            // Force to 0 
      EEPROM.put(NONVOL_TEST_MODE, json_test);  // and fall through
      break;

/*
 * Test 0, Display the help
 */
    default:                // Undefined, show the tests
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

/*
 * Test 1, Display GPIO inputs
 */
    case T_DIGITAL: 
      Serial.print("\r\nTime:");                      Serial.print(micros()/1000000); Serial.print("."); Serial.print(micros()%1000000); Serial.print("s");
      Serial.print("\r\nBD Rev:");                    Serial.print(revision());       
      Serial.print("\r\nDIP: 0x");                    Serial.print(read_DIP(), HEX); 
      digitalWrite(STOP_N, 0);
      digitalWrite(STOP_N, 1);                        // Reset the fun flip flop
      Serial.print("\r\nRUN FlipFlop: 0x");           Serial.print(is_running(), HEX);   
      Serial.print("\r\nTemperature: ");              Serial.print(temperature_C());  Serial.print("'C ");
      Serial.print(speed_of_sound(temperature_C(), RH_50));  Serial.print("mm/us");
      Serial.print("\r\nV_REF: ");                    Serial.print(volts); Serial.print(" Volts");
      Serial.print("\r\n");
      POST_LEDs();
      json_test = T_HELP;               // and stop the test
      break;

/*
 * Test 2, 3, Test the timing circuit
 */
    case T_TRIGGER:                       // Show the timer values (Wait for analog input)
      Serial.print("\r\nWaiting for Trigger\r\n");
    case T_CLOCK:                        // Show the timer values (Trigger input)
      stop_counters();
      arm_counters();

      set_LED(L('*', '-', '-'));
      
      if ( json_test == T_CLOCK )
      {
        if ( revision() >= REV_220 )  
        {
          random_delay = random(1, 6000);   // Pick a random delay time in us
          Serial.print("\r\nRandom clock test: "); Serial.print(random_delay); Serial.print("us. All outputs must be the same. ");
          trip_counters();
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
      
      set_LED(L('-', '-', '-'));
      delay(ONE_SECOND);
      break;

/*
 * Test 4, 5, Simple O'Scope
 */
    case T_OSCOPE:                       // Show the analog input
      show_analog(0);                  
      break;
      
    case T_OSCOPE_PC:
      show_analog_on_PC(0);
      break;

/*
 * Test 6, Advance the paper
 */
    case T_PAPER: 
      Serial.print("\r\nAdvanciing backer paper "); Serial.print(((json_paper_time) + (json_step_time)) * 10); Serial.print(" ms  ");Serial.print(json_step_count); Serial.print(" steps");
      drive_paper();
      Serial.print("\r\nDone");
      json_test = T_HELP;
      break;

/*
 * Test 7, 8, 9, Generate test pattern for diagnosing software problems
 */
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

/*
 * Test 10, Test the pass through connector
 */
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

/*
 * Test 11
 */
    case T_SET_TRIP:
      set_trip_point(0);          // Stay in the trip point loop
      json_test = T_HELP;
      break;

/*
 * Test 13
 */
    case T_SERIAL_PORT:
      Serial.print("\r\nArduino Serial Port: Hello World\r\n");
      AUX_SERIAL.print("\r\nAux Serial Port: Hello World\r\n");
      DISPLAY_SERIAL.print("\r\nDisplay Serial Port: Hello World\r\n");
      json_test = T_HELP;
      break;

/* 
 *  Test 14
 */
    case T_LED:
      Serial.print("\r\nRamping the LED");
      for (i=0; i != 256; i++)
      {
        analogWrite(LED_PWM, i);
        delay(ONE_SECOND/50);
      }
      for (i=255; i != -1; i--)
      {
        analogWrite(LED_PWM, i);
        delay(ONE_SECOND/50);
      }
      analogWrite(LED_PWM, 0);
      Serial.print(" Done\r\n");
      json_test = T_HELP;
      break;

 /*
  * Test 15
  */
    case T_FACE:
      Serial.print("\r\nFace strike test");
      enable_interrupt(1);
      face_strike = 0;
      EEPROM.put(NONVOL_TEST_MODE, T_HELP);     // Stop the test on the next boot cycle
      while (1)
      {        
        if ( face_strike != 0 )
        {
          set_LED(L('*', '*', '*'));           // If something comes in, turn on all of the LEDs 
          Serial.print("\n\rStrike");
          delay(ONE_SECOND/4);
          face_strike = 0;
          enable_interrupt(1);
        }
        else
        {
          set_LED(L('-', '-', '-'));
        }
        delay(ONE_SECOND/2);
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
 * function: POST_version()
 * 
 * brief: Show the Version String
 * 
 * return: None
 * 
 *----------------------------------------------------------------
 *
 *  Common function to show the version. Routed to the selected
 *  port(s)
 *  
 *--------------------------------------------------------------*/
 void POST_version
    (
    int port        // Port to display output on
    )
{
  int i;
  char str_a[256];

  sprintf(str_a, "\r\nfreETarget %s\r\n", SOFTWARE_VERSION);
  
/*
 * Display the version on the default serial port
 */
  if ( (port == 0) || (port & PORT_SERIAL) ) // No port or Serial port selected
  {
    Serial.print(str_a);
  }

/*
 * Display the version on the AUX port, taking care of the WiFi if present
 */
  if ( port & PORT_AUX )
  {
    if ( esp01_is_present() )
    {
      for (i=0; i != MAX_CONNECTIONS; i++ )
      {
        esp01_send(str_a, i);
        esp01_send(0, i);
      }
    }
    else 
    {
      AUX_SERIAL.print(str_a);        // No ESP-01, then use just the AUX port
    }
  }

 /*
  * Output to the spare USB serial port
  */
  if ( port & PORT_DISPLAY )
  {
    DISPLAY_SERIAL.print(str_a);
  }

/*
 * All done, return
 */
  return;
}
 
/*----------------------------------------------------------------
 * 
 * function: POST_LEDs()
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

 void POST_LEDs(void)
 {
  if ( is_trace )
  {
    Serial.print("\r\nPOST LEDs");
  }

  set_LED(L('*', '.', '.'));
  delay(ONE_SECOND/4);
  set_LED(L('.', '*', '.'));
  delay(ONE_SECOND/4);
  set_LED(L('.', '.', '*'));
  delay(ONE_SECOND/4);
  set_LED(L('.', '.', '.'));
  
  return;
 }

/*----------------------------------------------------------------
 * 
 * function: void POST_counters()
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
 *  Test 1, Arm the circuit and make sure there are no random trips
 *          This test will fail if the sensor cable harness is not attached
 *  Test 2, Arm the circuit amd make sure it is off (No running counters
 *  Test 3: Trigger the counter and make sure that all sensors are triggered
 *  Test 4: Stop the clock and make sure that the counts have stopped
 *  Test 5: Verify that the counts are correctia
 *  
 *--------------------------------------------------------------*/
 #define POST_counters_cycles 10 // Repeat the test 10x
 #define CLOCK_TEST_LIMIT 500    // Clock should be within 500 ticks
 
 bool POST_counters(void)
 {
   unsigned int i, j;            // Iteration counter
   unsigned int random_delay;    // Delay duration
   unsigned int sensor_status;   // Sensor status
   int          x;               // Time difference (signed)
   bool         test_passed;     // Record if the test failed
   long         now;             // Current time
   
/*
 * The test only works on V2.2 and higher
 */

  if ( revision() < REV_300 )
  {
    return true;                      // Fake a positive response  
  }
  
  if ( is_trace )
  {
    Serial.print("\r\nPOST counters");
  }

  test_passed = true;                 // And assume that it will pass
  
/*
 * Test 1, Arm the circuit and see if there are any random trips
 */
  stop_counters();                    // Get the circuit ready
  arm_counters();                     // Arm it. 
  delay(1);                           // Wait a millisecond  
  sensor_status = is_running();       // Remember all of the running timers
  if ( sensor_status != 0 )
  {
    Serial.print("\r\nFailed Clock Test. Spurious trigger:"); show_sensor_status(sensor_status);
    return false;                     // No point in any more tests
  }
  
/*
 * Loop and verify the opertion of the clock circuit using random times
 */
  for (i=0; i!= POST_counters_cycles; i++)
  {
    if ( is_trace )
    {
      Serial.print("\r\nCycle:"); Serial.print(i);
    }
    
/*
 *  Test 2, Arm the circuit amd make sure it is off
 */
    stop_counters();                  // Get the circuit ready
    arm_counters();
    delay(1);                         // Wait for a bit
    
    for (j=N; j <= W; j++ )           // Check all of the counters
    {
      if ( read_counter(j) != 0 )     // Make sure they stay at zero
      {
        Serial.print("\r\nFailed Clock Test. Counter free running:"); Serial.print(nesw[j]);
        test_passed =  false;         // return a failed test
      }   
    }
    
 /*
  * Test 3: Trigger the counter and make sure that all sensors are triggered
  */
    stop_counters();                  // Get the circuit ready
    arm_counters();
    delay(1);  
    random_delay = random(1, 6000);   // Pick a random delay time in us
    now = micros();                   // Grab the current time
    trip_counters();
    sensor_status = is_running();     // Remember all of the running timers

    while ( micros() < (now + random_delay ) )
    {
      continue;
    }
    
    stop_counters();
    if ( sensor_status != 0x0F )      // The circuit was triggered but not all
    {                                 // FFs latched
      Serial.print("\r\nFailed Clock Test. sensor_status:"); show_sensor_status(sensor_status);
      test_passed = false;
    }

/*
 * Test 4: Stop the clock and make sure that the counts have stopped
 */
    random_delay *= 8;                // Convert to clock ticks
    for (j=N; j <= W; j++ )           // Check all of the counters
    {
      x  = read_counter(j);
      if ( read_counter(j) != x )
      {
        Serial.print("\r\nFailed Clock Test. Counter did not stop:"); Serial.print(nesw[j]); show_sensor_status(sensor_status);
        test_passed = false;          // since there is delay  in
      }                               // Turning off the counters
 
/*
 * Test 5: Verify that the counts are correct
 */
      x =x - random_delay;
      if( x < 0 )
      {
        x = -x;
      }
      
      if ( x > CLOCK_TEST_LIMIT )     // The time should be positive and within limits
      { 
        Serial.print("\r\nFailed Clock Test. Counter:"); Serial.print(nesw[j]); Serial.print(" Is:"); Serial.print(read_counter(j)); Serial.print(" Should be:"); Serial.print(random_delay); Serial.print(" Delta:"); Serial.print(x);
        test_passed = false;          // since there is delay  in
      }                               // Turning off the counters
      else
      {
        if ( is_trace )
        {
          Serial.print("\r\nClock Pass. Counter:"); Serial.print(nesw[j]); Serial.print(" Is:"); Serial.print(read_counter(j)); Serial.print(" Should be:"); Serial.print(random_delay); Serial.print(" Delta:"); Serial.print(x);
        }
      }
    }
  }
  
/*
 * Got here, the test completed successfully
 */
  set_LED(L('.', '.', '.'));
  return test_passed;
}
  
/*----------------------------------------------------------------
 * 
 * function: void POST_trip_point()
 * 
 * brief: Display the trip point
 * 
 * return: None
 *----------------------------------------------------------------
 *
 *  Run the set_trip_point function once
 *  
 *--------------------------------------------------------------*/
 void POST_trip_point(void)
 {
   if ( is_trace )
   {
    Serial.print("\r\nPOST trip point");
   }
   
   set_trip_point(20);              // Show the trip point once (20 cycles used for blinking values)
   set_LED(L('.', '.', '.'));        // Show test test Ending
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
//                                         0           1         2       3         4        5        6        7        8        9        10      11        12          13      14          15
const unsigned int volts_to_LED[] = { NOT_IN_SPEC,     1,    BLINK+1,    2,     BLINK+2,    3,    BLINK+3,    4,    BLINK+4,    5,    BLINK+5,    6,     BLINK+6,       7,   BLINK+7,  NOT_IN_SPEC, 0 };
const unsigned int mv_to_counts[] = {   CT(350),    CT(400), CT(450), CT(500),  CT(550), CT(600), CT(650), CT(700), CT(750), CT(800), CT(900), CT(1000), CT(1100), CT(1200), CT(1300),   CT(5000),  0 };

void set_trip_point
  (
  int pass_count                                            // Number of passes to allow before exiting (0==infinite)
  )
{
  unsigned long start_time;                                 // Starting time of average loop 
  unsigned long sample;                                     // Counts read from ADC
           bool blinky;                                     // Blink the LEDs on an over flow
  bool          not_in_spec;                                // Set to true if the input is close to the limits
  bool          stay_forever;                               // Stay forever if called with pass_count == 0;
  unsigned int  sensor_status;                              // OR of the sensor bits that have tripped
  
  if ( is_trace )                                           // Infinite number of passes?
  {
    Serial.print("\r\nSetting trip point. Type ! of cycle power to exit\r\n");
  }
  blinky = 0;
  not_in_spec = true;                                       // Start off by assuming out of spec
  sensor_status = 0;                                        // No sensors have tripped
  stay_forever = false;
  if (pass_count == 0 )                                     // A pass count of 0 means stay
  {
    stay_forever = true;                                    // For a long time
  }
  arm_counters();                                           // Arm the flip flops for later
  enable_interrupt(true);                                   // Arm the face sensor
  face_strike = false;
  
/*
 * Loop if not in spec, passes to display, or the CAL jumper is in
 */
  while ( not_in_spec                                       // Out of tolerance
          ||   ( stay_forever )                             // Passes to go
          ||   ( CALIBRATE )                                // Held in place by DIP switch
          ||   (pass_count != 0))                           // Wait here for N cycles
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
        Serial.print("\r\nOut Of Spec: "); Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
      }
      blink_fault(VREF_OVER_UNDER);
      pass_count = 0;                                       // Stay in this function forever
      not_in_spec = true;                                   // Show we are out of spec
      continue;
    }

 /*
  * In spec, display the trip level on the LEDs
  */
    if ( CAL_LOW )                                          // Low Calibration 
    {
        sample *= 3;
        sample /= 2;
    }
        
    if ( CAL_HIGH )                                      // Set scale if the high range
    {
       sample *= 2;
       sample /= 3;
    }
    
 /*
  * Determine what band it belongs to 
  */
   i = 0;
   while (volts_to_LED[i] != 0)
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
   blinky = !blinky;                      // Toggle the blink
   ch = volts_to_LED[i];
   if ( ch & BLINK )                      // Blink bit on?
   {
     if ( blinky )                        // Time to blink?
     {
       ch = 0;                            // No, then off
     }
     ch &= ~BLINK;                        // Keep only the LED state
   }
   
   not_in_spec = ch & NOT_IN_SPEC;
   
   if ( not_in_spec )                     // Out of spec?
   {
     ch = 0b010;                          // Flash x - * - x
     if ( blinky )                        // or    * - x - *
     {
       ch = 0b101; 
     }
   }

   set_LED(ch & 4, ch & 2, ch & 1);

/*
 * Got to the end.  See if we are going to do this for a fixed time or forever
 */
    switch (Serial.read())
    {
      case '!':                       // ! waiting in the serial port
        Serial.print("\r\nExiting calibration\r\n");
        return;

      case 'X':
      case 'x':                       // X Cancel
        sensor_status = 0;
        arm_counters();               // Reset the latch state
        enable_interrupt(1);          // Turn on the face strike interrupt
        face_strike = false;          // Reset the face strike count
        enable_interrupt(1);          // Turning it on above creates a fake interrupt with a disable
        break;

      default:
        break;
    }

   if ( stay_forever )
   {
      Serial.print("\r\nV_Ref: "); Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
      show_sensor_status(is_running());
   }
   else
   {
     if ( pass_count != 0 )             // Set for a finite loop?
     {
        pass_count--;                   // Decriment count remaining
        if ( pass_count == 0 )          // And bail out when zero
        {
          return;
        }
      }
   }
   delay(ONE_SECOND/5);
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
  
  set_LED((1 << cycle) & 1, (1 << cycle) & 2, (1 << cycle) & 4);
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
    location = compute_hit(0x0F, &record, true);
    sensor_status = 0xF;        // Fake all sensors good
    send_score(&record, shot_number, sensor_status);
    shot_number++;
    delay(ONE_SECOND/2);        // Give the PC program some time to catch up
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

/*----------------------------------------------------------------
 *
 * function: show_sensor_status()
 *
 * brief:    Show which sensor flip flops were latched
 *
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 * 
 * The sensor state NESW or .... is shown for each latch
 * The clock values are also printed
 *   
 *--------------------------------------------------------------*/

void show_sensor_status(unsigned int sensor_status)
{
  unsigned int i;
  
  Serial.print(" Latch:");

  for (i=N; i<=W; i++)
  {
    if ( sensor_status & (1<<i) )   Serial.print(nesw[i]);
    else                            Serial.print(".");
  }

  Serial.print(" Timers:");

  for (i=N; i<=W; i++)
  {
    if ( timer_value[i] != 0 )      Serial.print(nesw[i]);
    else                            Serial.print(".");
  }

  Serial.print(" Face Strike:");
  Serial.print(face_strike);

  if ( ((sensor_status & 0x0f) == 0x0f)
    && (face_strike) )
  {
    Serial.print(" PASS");
  }    

/*
 * All done, return
 */

  return;
}
