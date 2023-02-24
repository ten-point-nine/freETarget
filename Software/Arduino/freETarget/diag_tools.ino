
/*----------------------------------------------------------------
 *
 * diag_tools.ino
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#include "freETarget.h"
#include "gpio.h"

const char* which_one[4] = {"North:", "East:", "South:", "West:"};

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define GRID_SIDE 25                              // Should be an odd number
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))

static void show_analog_on_PC(int v);
static void unit_test(unsigned int mode);
static bool sample_calculations(unsigned int mode, unsigned int sample);
static void log_sensor(int sensor);

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
  double       volts;               // Reference Voltage
  unsigned int i;
  char         ch;
  unsigned int sensor_status;       // Sensor running inputs
  unsigned long sample;             // Sample used for comparison
  unsigned int random_delay;        // Random sampe time
  bool         pass;
  unsigned long start_time;         // Running time
  shot_record_t shot;               // Shot history
  unsigned char s[128];             // Text buffer
  
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
/*
 * Test 0, Display the help
 */
    default:                // Undefined, show the tests
    case T_HELP:                
      Serial.print(T("\r\n 1 - Digital inputs"));
      Serial.print(T("\r\n 2 - Counter values (external trigger)"));
      if ( revision() >= REV_220 )
      {
        Serial.print(T("\r\n 3 - Counter values (internal trigger)"));
      }
      Serial.print(T("\r\n 4 - Oscilloscope"));
      Serial.print(T("\r\n 5 - Oscilloscope (PC)"));
      Serial.print(T("\r\n 6 - Advance paper backer"));
      Serial.print(T("\r\n 7 - Spiral Unit Test"));
      Serial.print(T("\r\n 8 - Grid calibration pattern"));
      Serial.print(T("\r\n 9 - One time calibration pattern"));
      Serial.print(T("\r\n 8 - Grid calibration pattern"));
      if ( revision() >= REV_220 )
      {
        Serial.print(T("\r\n10 - Aux port passthrough"));
      }
      Serial.print(T("\r\n11 - Calibrate")); 
      Serial.print(T("\r\n12 - Transfer loopback"));
      Serial.print(T("\r\n13 - Serial port test"));
      Serial.print(T("\r\n14 - LED brightness test"));
      Serial.print(T("\r\n15 - Face strike test"));
      Serial.print(T("\r\n16 - WiFi test"));
      Serial.print(T("\r\n17 - Dump NonVol"));
      Serial.print(T("\r\n18 - Send sample shot record"));
      Serial.print(T("\r\n19 - Show WiFi status"));
      Serial.print(T("\r\n20 - Send clock out of all serial ports"));
      Serial.print(T("\r\n21 - Log North Sensor"));
      Serial.print(T("\r\n22 - Log East Sensor"));
      Serial.print(T("\r\n23 - Log South Sensor"));
      Serial.print(T("\r\n24 - Log West Sensor"));
      Serial.print(T("\r\n25 - Test Push Buttons"));
      Serial.print(T("\r\n26 - Unit Test speed_of_sound()"));
      Serial.print(T("\r\n"));
      break;

/*
 * Test 1, Display GPIO inputs
 */
    case T_DIGITAL: 
      digital_test();
      break;

/*
 * Test 2, 3, Test the timing circuit
 */
    case T_TRIGGER:                       // Show the timer values (Wait for analog input)
      Serial.print(T("\r\nWaiting for Trigger\r\n"));
    case T_CLOCK:                        // Show the timer values (Trigger input)
      stop_timers();
      arm_timers();

      set_LED(L('*', '-', '-'));
      
      if ( test == T_CLOCK )
      {
        if ( revision() >= REV_220 )  
        {
          random_delay = random(1, 6000);   // Pick a random delay time in us
          Serial.print(T("\r\nRandom clock test: ")); Serial.print(random_delay); Serial.print(T("us. All outputs must be the same. "));
          trip_timers();
          delayMicroseconds(random_delay);  // Delay a random time
        }
        else
        {
          Serial.print(T("\r\nThis test not supported on this hardware revision"));
          break;
        }
      }
  
      while ( !is_running() )
      {
        continue;
      }
      sensor_status = is_running();       // Remember all of the running timers
      stop_timers();
      read_timers(&shot.timer_count[0]);

      if ( test == T_CLOCK )       // Test the results
      {
        sample = shot.timer_count[N];
        pass = true;

        for(i=N; i<=W; i++)
        {
          if ( shot.timer_count[i] != sample )
          {
            pass = false;                 // Make sure they all match
          }
        }

        if ( pass == true )
        {
          Serial.print(T(" PASS\r\n"));
        }
        else
        {
          Serial.print(T(" FAIL\r\n"));
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
      Serial.print(T("\r\nAdvancing backer paper ")); Serial.print(((json_paper_time) + (json_step_time)) * 10); Serial.print(T(" ms  ")); Serial.print(json_step_count); Serial.print(T(" steps"));
      drive_paper();
      Serial.print(T("\r\nDone"));
      break;

/*
 * Test 7, 8, 9, Generate test pattern for diagnosing software problems
 */
    case T_SPIRAL: 
      Serial.print(T("\r\nSpiral Calculation\r\n"));
      unit_test( T_SPIRAL );            // Generate a spiral
      break;

    case T_GRID:
      Serial.print(T("\r\nGrid Calculation\r\n"));
      unit_test( T_GRID);               // Generate a grid
      break;  
      
    case T_ONCE:
      Serial.print(T("\r\nSingle Calculation\r\n"));
      unit_test( T_ONCE);               // Generate a SINGLE calculation
      break;

/*
 * Test 10, Test the pass through connector
 */
    case T_PASS_THRU:
      Serial.print(T("\r\nPass through active.  Cycle power to exit\r\n"));
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
      break;

/*
 * Test 13
 */
    case T_SERIAL_PORT:
      Serial.print(T("\r\nArduino Serial Port: Hello World\r\n"));
      AUX_SERIAL.print(T("\r\nAux Serial Port: Hello World\r\n"));
      DISPLAY_SERIAL.print(T("\r\nDisplay Serial Port: Hello World\r\n"));
      break;

/* 
 *  Test 14
 */
    case T_LED:
      Serial.print(T("\r\nRamping the LED"));
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
      Serial.print(T(" Done\r\n"));
      break;

 /*
  * Test 15
  */
    case T_FACE:
      Serial.print(T("\r\nFace strike test\n\r"));
      face_strike = 0;                        // Reset the interrupt count
      sample = 0;
      enable_face_interrupt();
      ch = 0;
      
      while ( ch != '!' )
      {        
        if ( face_strike != 0 )
        {
          face_strike--;
          set_LED(L('*', '*', '*'));           // If something comes in, turn on all of the LEDs 
        }
        else
        {
          set_LED(L('.', '.', '.'));          // face_strike complete
        }
        if ( sample != face_strike )          // If there is a change, display it
        {
          if ( (face_strike % 20) == 0 )
          {
            Serial.print(T("\r\n"));
          }
          Serial.print(T(" S:")); Serial.print(face_strike);
          sample = face_strike;        
        }
        esp01_receive();                // Accumulate input from the IP port.
        ch = GET();
    }
    Serial.print(T("\r\nDone\n\r"));
    break;

 /*
  * TEST 16 WiFI
  */
   case T_WIFI:
    esp01_test();
   break;

/*
 * TEST 17 Dump NonVol
 */
   case T_NONVOL:
    dump_nonvol();
   break;

  
/*
 * Test 18 Sample shot value 
 */
  case T_SHOT:
    shot.x = 10;
    shot.y = 20;
    shot.shot_time = millis()/100;
    shot.shot_number = 1;
    send_score(&shot);
    send_miss(&shot);
    break;
    
 /*
  * TEST 19 WiFi Status
  */
   case T_WIFI_STATUS:
    esp01_status();
   break;
   
 /*
  * TEST 20 WiFi Broadcast
  */
   case T_WIFI_BROADCAST:
    sprintf(s, "Type ! to exit ");
    output_to_all(s); 
    ch = 0;
    while( ch != '!' )
    {
      i = millis() / 1000;
      if ( (i % 60) == 0 )
      {
        sprintf(s, "\r\n%d:%d ", i/60, i % 60);
        output_to_all(s);
      }
      else
      {
      sprintf(s, " %d:%d ", i/60, i % 60);
      output_to_all(s);
      }
      esp01_receive();                // Accumulate input from the IP port.
      ch = GET();
      delay(1000);
    }
    sprintf(s, "\r\nDone");
   break;
   
/*
 * Test 21 Log the input voltage levels on North
 */
  case T_LOG + 0:     // 20
  case T_LOG + 1:     // 21
  case T_LOG + 2:     // 22
  case T_LOG + 3:     // 23
    log_sensor(test - T_LOG);
    break;

/*
 * Test 25 Log the input voltage levels on North
 */
  case T_SWITCH:
    ch = 0;
    while ( ch != '!' )
    {
      set_LED( 1, DIP_SW_A, DIP_SW_B );   // Copy the switches
      esp01_receive();                    // Accumulate input from the IP port.
      ch = GET();
    }
    Serial.print(T("\n\rDone"));
    break;

/*
 * Test 26 Test speed_of_sound()
 */
  case T_S_OF_SOUND:
    sound_test();
    Serial.print(T("\n\rDone"));
    break;

  }
 
 /* 
  *  All done, return;
  */
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
 void POST_version(void)
 {
  int i;
  char str[64];

  sprintf(str, "\r\nfreETarget %s\r\n", SOFTWARE_VERSION);
  output_to_all(str);

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
  if ( DLT(DLT_CRITICAL) )
  {
    Serial.print(T("POST LEDs"));
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
 * function: void POST_counteres()
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
 #define POST_counteres_cycles 10 // Repeat the test 10x
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
  
  if ( DLT(INIT_TRACE) )
  {
    Serial.print(T("POST_counters()"));
  }

  test_passed = true;                 // And assume that it will pass
  
/*
 * Test 1, Arm the circuit and see if there are any random trips
 */
  stop_timers();                    // Get the circuit ready
  arm_timers();                     // Arm it. 
  delay(1);                           // Wait a millisecond  
  sensor_status = is_running();       // Remember all of the running timers
  if ( (sensor_status != 0) && DLT(INIT_TRACE) )
  {
    Serial.print(T("\r\nFailed Clock Test. Spurious trigger:")); show_sensor_status(sensor_status, 0);
    return false;                     // No point in any more tests
  }
  
/*
 * Loop and verify the opertion of the clock circuit using random times
 */
  for (i=0; i!= POST_counteres_cycles; i++)
  {
    
/*
 *  Test 2, Arm the circuit amd make sure it is off
 */
    stop_timers();                  // Get the circuit ready
    arm_timers();
    delay(1);                         // Wait for a bit
    
    for (j=N; j <= W; j++ )           // Check all of the counters
    {
      if ( (read_counter(j) != 0) && DLT(INIT_TRACE) )     // Make sure they stay at zero
      {
        Serial.print(T("Failed Clock Test. Counter free running:")); Serial.print(nesw[j]);
        test_passed =  false;         // return a failed test
      }   
    }
    
 /*
  * Test 3: Trigger the counter and make sure that all sensors are triggered
  */
    stop_timers();                  // Get the circuit ready
    arm_timers();
    delay(1);  
    random_delay = random(1, 6000);   // Pick a random delay time in us
    now = micros();                   // Grab the current time
    trip_timers();
    sensor_status = is_running();     // Remember all of the running timers

    while ( micros() < (now + random_delay ) )
    {
      continue;
    }
    
    stop_timers();
    if ( (sensor_status != 0x0F) && DLT(INIT_TRACE) )      // The circuit was triggered but not all
    {                                 // FFs latched
      Serial.print(T("Failed Clock Test. sensor_status:")); show_sensor_status(sensor_status, 0);
      test_passed = false;
    }

/*
 * Test 4: Stop the clock and make sure that the counts have stopped
 */
    random_delay *= 8;                // Convert to clock ticks
    for (j=N; j <= W; j++ )           // Check all of the counters
    {
      x  = read_counter(j);
      if ( (read_counter(j) != x) && DLT(INIT_TRACE) )
      {
        Serial.print(T("Failed Clock Test. Counter did not stop:")); Serial.print(nesw[j]); show_sensor_status(sensor_status, 0);
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
      
      if ( (x > CLOCK_TEST_LIMIT) && DLT(INIT_TRACE) )     // The time should be positive and within limits
      { 
        Serial.print(T("Failed Clock Test. Counter:")); Serial.print(nesw[j]); Serial.print(T(" Is:")); Serial.print(read_counter(j)); Serial.print(T(" Should be:")); Serial.print(random_delay); Serial.print(T(" Delta:")); Serial.print(x);
        test_passed = false;          // since there is delay  in
      }                               // Turning off the counters
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
   if ( DLT(DLT_CRITICAL) )
   {
    Serial.print(T("POST trip point"));
   }
   
   set_trip_point(20);              // Show the trip point once (20 cycles used for blinking values)
   set_LED(LED_RESET);               // Show test test Ending
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

static void start_over(void)    // Start the test over again
{
  stop_timers();
  arm_timers();                 // Reset the latch state
  enable_face_interrupt();      // Turn on the face strike interrupt
  face_strike = 0;              // Reset the face strike count
  enable_face_interrupt();      // Turning it on above creates a fake interrupt with a disable
  return;
}

void set_trip_point
  (
  int pass_count                                            // Number of passes to allow before exiting (0==infinite)
  )
{
  unsigned long start_time;                                 // Starting time of average loop 
  unsigned long sample;                                     // Counts read from ADC
           bool blinky;                                     // Blink the LEDs on an over flow
  bool          stay_forever;                               // Stay forever if called with pass_count == 0;
  unsigned int  sensor_status;                              // OR of the sensor bits that have tripped
  bool          pause;                                      // Stop the test
  unsigned int  i, j;                                       // Iteration Counter
  
  Serial.print(T("Setting trip point. Type ! of cycle power to exit\r\n"));
  blinky = 0;
  sensor_status = 0;                                        // No sensors have tripped
  stay_forever = false;
  if (pass_count == 0 )                                     // A pass count of 0 means stay
  {
    stay_forever = true;                                    // For a long time
  }
  arm_timers();                                             // Arm the flip flops for later
  enable_face_interrupt();                                  // Arm the face sensor
  disable_timer_interrupt();                                // Disarm the sensor input interrupt
  face_strike = 0;

/*
 * Set the PWM at 50%
 */
  json_vset_PWM = 128;
  set_vset_PWM(json_vset_PWM);
  EEPROM.put(NONVOL_vset_PWM, json_vset_PWM);

/*
 * Loop if not in spec, passes to display, or the CAL jumper is in
 */
  while ( ( stay_forever )                                  // Passes to go
          ||   ( CALIBRATE )                                // Held in place by DIP switch
          ||   (pass_count != 0))                           // Wait here for N cycles
  {
/*
 * Got to the end.  See if we are going to do this for a fixed time or forever
 */
    switch (Serial.read())
    {
      case '!':                       // ! waiting in the serial port
        Serial.print(T("\r\nExiting calibration\r\n"));
        return;

      case 'B':
      case 'b':                       // Blink the Motor Drive
        Serial.print(T("\r\nBlink Motor Drive\r\n"));
        paper_on_off(1);
        delay(ONE_SECOND);
        paper_on_off(0);
        break;
      
      case 'W':
      case 'w':                      // Test the WiFI
        Serial.print(T("\r\nTest WiFi"));
        esp01_test();
        break;
        
      case 'R':
      case 'r':                       // Reset Cancel
      case 'X':
      case 'x':                       // X Cancel
        start_over();
        break;
        
      default:
        break;
    }
    

   show_sensor_status(is_running(), 0);
   Serial.print(T("\n\r"));
   if ( stay_forever )
   {
      if ( (is_running() == 0x0f) && (face_strike != 0) )
      {
        start_over();
      }
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
  enable_timer_interrupt();
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
  
  Serial.print(T("{\"OSCOPE\": ")); Serial.print(o_scope);  Serial.print(T("\"}\r\n"));     // Display the trace as JSON

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
  Serial.print(T("\n{Ref:")); Serial.print(TO_VOLTS(analogRead(V_REFERENCE))); Serial.print(T("  "));
  
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
  Serial.print(T("}"));

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
    location = compute_hit(&record[0], true);
    sensor_status = 0xF;        // Fake all sensors good
    record[0].shot_number = shot_number++;
    send_score(&record[0]);
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
  shot_record_t shot;
  
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
    shot.timer_count[N] = RX(N, x, y);
    shot.timer_count[E] = RX(E, x, y);
    shot.timer_count[S] = RX(S, x, y);
    shot.timer_count[W] = RX(W, x, y);
    shot.timer_count[W] -= 200;              // Inject an error into the West sensor

    Serial.print(T("\r\nResult should be: "));   Serial.print(T("x:")); Serial.print(x); Serial.print(T(" y:")); Serial.print(y); Serial.print(T(" radius:")); Serial.print(radius); Serial.print(T(" angle:")); Serial.print(angle * 180.0d / PI);
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
    shot.timer_count[N] = RX(N, x, y);
    shot.timer_count[E] = RX(E, x, y);
    shot.timer_count[S] = RX(S, x, y);
    shot.timer_count[W] = RX(W, x, y);
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

    shot.timer_count[N] = RX(N, x, y);
    shot.timer_count[E] = RX(E, x, y);
    shot.timer_count[S] = RX(S, x, y);
    shot.timer_count[W] = RX(W, x, y);
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

void show_sensor_status
  (
  unsigned int   sensor_status,
  shot_record_t* shot
  )
{
  unsigned int i;
  
  Serial.print(T(" Latch:"));

  for (i=N; i<=W; i++)
  {
    if ( sensor_status & (1<<i) )   Serial.print(nesw[i]);
    else                            Serial.print(T("."));
  }

  if ( shot != 0 )
  {
    Serial.print(" Timers:");

    for (i=N; i<=W; i++)
    {
      Serial.print(T(" "));  Serial.print(nesw[i]); Serial.print(T(":")); Serial.print(shot->timer_count[i]); 
    }
  }
  
  Serial.print(T("  Face Strike:")); Serial.print(face_strike);
  
  Serial.print(T("  V_Ref:")); Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
  
  Serial.print(T("  Temperature:")); Serial.print(temperature_C());
  
  Serial.print(T("  WiFi:")); Serial.print(esp01_is_present());                           // TRUE if WiFi is available

  Serial.print(T("  Switch:"));
  
  if ( DIP_SW_A == 0 )
  {
    Serial.print(T("--"));
  }
  else
  {
    Serial.print(T("A1"));
  }
  Serial.print(T(" "));
  if ( DIP_SW_B == 0 )
  {
    Serial.print(T("--"));
  }
  else
  {
    Serial.print(T("B2"));
  }

  if ( ((sensor_status & 0x0f) == 0x0f)
    && (face_strike != 0) )
  {
    Serial.print(T(" PASS"));
    delay(ONE_SECOND);                // Wait for click to go away
  }    

/*
 * All done, return
 */

  return;
}

/*----------------------------------------------------------------
 *
 * function: log_sensor()
 *
 * brief:    Sample and display the North sensor value
 *
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 * 
 * The North sensor is sampled for one second and the maximum
 * for that sample is displayed.
 * 
 * The process repeats until the user types in a !
 *   
 *--------------------------------------------------------------*/
#define LOG_TIME  1000            // 1 second loop time

void log_sensor
  (
  int sensor                      // Sensor to be monitored
  )
{
  unsigned int i;
  unsigned long start;
  unsigned int max_cycle;         // Largest value this cycle
  unsigned int max_all;           // Largest value ever
  unsigned int sample;            // Sample read from ADC
  unsigned int sensor_status;     // Sensor running latch
  char         s[128];            // String Holder
  char         ch;                // Input character
  bool         is_new;            // TRUE if a change was found

  sprintf(s, "\r\nLogging %s Use X to reset,  ! to exit\r\n", which_one[sensor]);
  output_to_all(s);
  output_to_all(0);
  max_all =  0;
  arm_timers();
  
  while (1)
  {
/*
 * Loop for the sample time, picking up the analog voltage as quick as we can
 */
    start = LOG_TIME;                         // Pick up the starting time
    max_cycle = analogRead(channel[sensor]);
    is_new = false;
    while ( (--start) )                       // For Log Time, 
    {
      sample = analogRead(channel[sensor]);   // Read the ADC
      if ( sample > max_cycle )
      {
        max_cycle = sample;
        is_new = true;
      }
      if ( sample > max_all )
      {
        max_all = sample;
        is_new = true;
      }
    }

/*
 * If there is a change output the result
 */
    sensor_status = is_running();
    if ( is_new  || (sensor_status != 0) )
    {
      sprintf(s, "\r\n%s cycle:%d  max:%d is_running:", which_one[sensor], max_cycle, max_all);
      output_to_all(s);
      output_to_all(0);      

      s[1] = 0;
      for (i=N; i<=W; i++)
      {
        if ( sensor_status & (1<<i) )   s[0] = nesw[i];
        else                            s[0] = '.';
        output_to_all(s);
      }
      arm_timers();
    }

    while ( AVAILABLE )
    {
      ch = GET();
      switch ( ch )
      {
        case '!':
          sprintf(s, "\r\nDone");
          output_to_all(s);
          output_to_all(0);
          return;

        case 'x':
        case 'X':
          max_all = 0;
          max_cycle = 0;
          break;
      }
    }
  }

/*  
 *   We never get here
 */
  return;
} 

/*----------------------------------------------------------------
 *
 * function: do_dlt
 *
 * brief:    Check for a DLT log and print the time
 *
 * return:   TRUE if the DLT should be printed
 * 
 *----------------------------------------------------------------
 * 
 * is_trace is compared to the log level and if valid the
 * current time stamp is printed
 *   
 *--------------------------------------------------------------*/
bool do_dlt
  (
  unsigned int level
  )
{ 
  if ((is_trace) < (level))
  {
    return false;      // Send out if the trace is higher than the level 
  }
  Serial.print(T("\n\r")); Serial.print(micros()/1000000); Serial.print(T(".")); Serial.print(micros()%1000000); Serial.print(T(": "));
  return true;
}
