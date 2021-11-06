
/*----------------------------------------------------------------
 * 
 * freETarget        
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *-------------------------------------------------------------*/
#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "EEPROM.h"
#include "nonvol.h"
#include "mechanical.h"
#include "diag_tools.h"
#include "esp-01.h"

this_shot record;  

double       s_of_sound;                // Speed of sound
unsigned int shot = 0;                  // Shot counter
bool         face_strike = false;       // Miss indicator
bool         is_trace = false;          // TRUE if trace is enabled

const char* names[] = { "TARGET",                                                                                           //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR",                                                            // 26
                        0};
                  
const char nesw[]   = "NESW";                  // Cardinal Points
const char to_hex[] = "0123456789ABCDEF";      // Quick Hex to ASCII

static long tabata(bool reset_time, bool reset_cycles); // Tabata state machine

/*----------------------------------------------------------------
 * 
 * function: setup()
 * 
 * brief: Initialize the board and prepare to run
 * 
 * return: None
 * 
 *--------------------------------------------------------------*/

void setup(void) 
{    
/*
 *  Setup the serial port
 */
  Serial.begin(115200);
  AUX_SERIAL.begin(115200); 
  DISPLAY_SERIAL.begin(115200); 

/*
 *  Set up the port pins
 */
  init_gpio();  
  init_sensors();
  init_analog_io();

  switch ( read_DIP() & 0x0f )              // Execute a power up routine if we have to
  {
    case 1:
      set_trip_point(0);                    // Calibrate
      break;

    case 0x0f:
      factory_nonvol(false);                // Force a factory nonvol reset
      break;

    default:
      break;
  }

  randomSeed( analogRead(V_REFERENCE));   // Seed the random number generator
  is_trace = VERBOSE_TRACE;               // Set the trace based on the DIP switch

/*
 * Initialize the WiFi if available
 */
   esp01_init();                           // Prepare the WiFi channel if installed

/*
 * Initialize variables
 */
  read_nonvol();
  tabata(true, true);                     // Reset the Tabata timers

/*
 * Run the power on self test
 */
  POST_version(PORT_ALL);                 // Show the version string on all ports
  show_echo(0);
  POST_LEDs();                            // Cycle the LEDs
  while ( (POST_counters() == false)      // If the timers fail
              && !is_trace)               // and not in trace mode (DIAG jumper installed)
  {
    Serial.print(T("\r\nPOST_2 Failed\r\n"));// Blink the LEDs
    blink_fault(POST_COUNT_FAILED);       // and try again
  }
  POST_trip_point();                      // Show the trip point
  

/*
 * Ready to go
 */
  set_LED_PWM(json_LED_PWM);
  return;
}

/*----------------------------------------------------------------
 * 
 * function: loop()
 * 
 * brief: Main control loop
 * 
 * return: None
 * 
 *----------------------------------------------------------------
 */
#define SET_MODE      0         // Set the operating mode
#define ARM       (SET_MODE+1)  // State is ready to ARM
#define WAIT      (ARM+1)       // ARM the circuit
#define AQUIRE    (WAIT+1)      // Aquire the shot
#define REDUCE    (AQUIRE+1)    // Reduce the data
#define WASTE     (REDUCE+1)    // Wait for the shot to end
#define SEND_MISS (WASTE+1)     // Got a trigger, but was defective

unsigned int state = SET_MODE;
unsigned long now;              // Interval timer 
unsigned long power_save;       // Power save timer
unsigned int sensor_status;     // Record which sensors contain valid data
unsigned int location;          // Sensor location 
unsigned int i, j;              // Iteration Counter
int ch;
unsigned int shot_number;
unsigned int shot_time;         // Shot time captured from Tabata timer

void loop() 
{

/*
 * First thing, handle polled devices
 */
  esp01_receive();                // Accumulate input from the IP port.
  multifunction_switch(0);        // Read the switches
  tabata(false, false);
  
/*
 * Take care of any commands coming through
 */
  if ( read_JSON() )
  {
    now = micros();               // Reset the power down timer if something comes in
    set_LED_PWM(json_LED_PWM);    // Put the LED back on if it was off
  }

/*
 * Cycle through the state machine
 */
 
  switch (state)
  {

/*
 *  Check for special operating modes
 */
  default:
  case SET_MODE:
    is_trace |= VERBOSE_TRACE;        // Turn on tracing if the DIP switch is in place
    
    if ( CALIBRATE )
    {
      set_trip_point(0);             // Are we calibrating?
    }

    state = ARM;                      // Carry on to the target
    
    break;
/*
 * Arm the circuit
 */
  case ARM:
    arm_counters();
    enable_interrupt(json_send_miss); // Turn on the face strike interrupt
    face_strike = false;              // Reset the face strike count
    
    set_LED_PWM(json_LED_PWM);        // Keep the LEDs ON

    sensor_status = is_running();
    power_save = micros();            // Start the power saver time

    if ( sensor_status == 0 )
    { 
      if ( is_trace )
      {
        Serial.print(T("\r\n\nWaiting..."));
      }
      set_LED(SHOT_READY);   
      state = WAIT;             // Fall through to WAIT
    }
    else
    {
      if ( is_trace == false )
      {
        if ( sensor_status & TRIP_NORTH  )
        {
          Serial.print(T("\r\n{ \"Fault\": \"NORTH\" }"));
          set_LED(NORTH_FAILED);           // Fault code North
          delay(ONE_SECOND);
        }
        if ( sensor_status & TRIP_EAST  )
        {
          Serial.print(T("\r\n{ \"Fault\": \"EAST\" }"));
          set_LED(EAST_FAILED);           // Fault code East
          delay(ONE_SECOND);
        }
        if ( sensor_status & TRIP_SOUTH )
        {
          Serial.print(T("\r\n{ \"Fault\": \"SOUTH\" }"));
          set_LED(SOUTH_FAILED);         // Fault code South
          delay(ONE_SECOND);
        }
        if ( sensor_status & TRIP_WEST )
        {
          Serial.print(T("\r\n{ \"Fault\": \"WEST\" }"));
          set_LED(WEST_FAILED);         // Fault code West
          delay(ONE_SECOND);
        }
      }   
    }
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    if ( (json_power_save != 0 ) 
        && (((micros()-power_save) / 1000000 / 60) >= json_power_save) )
    {
      set_LED_PWM(0);                     // Dim the lights?
    }
    
    sensor_status = is_running();

    if ( face_strike )                    // Something hit the front
    {
      state = SEND_MISS;                  // Show it's a miss
      break;
    }

    if ( sensor_status != 0 )             // Shot detected
    {
      now = micros();                     // Remember the starting time
      record.shot_time = tabata(false, false);   // Capture the time into the shot
      set_LED(L('-', '*', '-'));          // No longer waiting
      state = AQUIRE;
    }

    break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    if ( (micros() - now) > (SHOT_TIME) ) // Enough time already
    { 
      stop_counters(); 
      state = REDUCE;                     // 3, 4 Have enough data to performe the calculations
    }
    break;

 
/*
 *  Reduce the data to a score
 */
  case REDUCE:   
    if ( is_trace )
    {
      Serial.print(T("\r\nTrigger: ")); 
      show_sensor_status(sensor_status);
      Serial.print(T("\r\nReducing..."));
    }
    set_LED(L('*', '*', '*'));                   // Light All
    location = compute_hit(sensor_status, &record, false);

    if ( ((json_tabata_cycles != 0) && ( record.shot_time == 0 ))         // Are we out of the tabata cycle time?
       || (timer_value[N] == 0) || (timer_value[E] == 0) || (timer_value[S] == 0) || (timer_value[W] == 0) ) // If any one of the timers is 0, that's a miss
    {
      state = SEND_MISS;
      break;
    }
    send_score(&record, shot_number, sensor_status);
    state = WASTE;
    shot_number++;                   
    break;
    

/*
 *  Wait here to make sure the RUN lines are no longer set
 */
  case WASTE:
    if ( (json_tabata_on == 0)                  // No tabata time enabled
      || (tabata(false, false) == 0) )          // Or outside of the tabata time
    {                                           // Advance the paper and wait.
      delay(ONE_SECOND/2);                      // Hang out for a second
      if ( (json_paper_time + json_step_time) != 0 )  // Has the witness paper been enabled
      {
        if ( (json_paper_eco == 0)                   // ECO turned off
          || ( sqrt(sq(record.x) + sq(record.y)) < json_paper_eco ) ) // And inside the black
        {
          drive_paper();
        }
      }
    }
    state = SET_MODE;
    break;

/*    
 * Show an error occured
 */
  case SEND_MISS:   
    state = SET_MODE;                         // Next time go to waste time 
    
    if ( is_trace )
    {
      Serial.print(T("\r\nFace Strike...\r\n"));
    } 

    face_strike = false;

    blink_fault(SHOT_MISS);
    
    if ( json_send_miss != 0)
    {
      send_miss(shot);
    }
    break;
    }

/*
 * All done, exit for now
 */
  return;
}

/*----------------------------------------------------------------
 * 
 * function: tabata
 * 
 * brief:   Implement a Tabata timer for training
 * 
 * return:  Time that the current TABATA state == TABATA_ON
 * 
 *----------------------------------------------------------------
 *
 * A Tabata timer is used to train for specific durations or time
 * of events.  For example in shooting the target should be aquired
 * and shot within a TBD seconds.
 * 
 * This function turns on the LEDs for a configured time and
 * then turns them off for another configured time and repeated
 * for a configured number of cycles
 * 
 * OFF - 2 Second Warning - 2 Second Off - ON 
 * 
 * Test JSON {"TABATA_ON":7, "TABATA_REST":45, "TABATA_CYCLES":60}
 * 
 *--------------------------------------------------------------*/
static long     tabata_time;  // Internal timern in milliseconds
static uint16_t tabata_cycles;

#define TABATA_IDLE         0
#define TABATA_WARNING      1
#define TABATA_DARK         2
#define TABATA_ON           3
#define TABATA_DONE_OFF     4
#define TABATA_DONE_ON      5

#define TABATA_BLINK_ON    20         // Warning blink 20 x 100ms = 2 second
#define TABATA_BLINK_OFF   20         // Warning blink 20 x 100ms = 1 second

static uint16_t tabata_state = TABATA_IDLE;

static long tabata
  (
  bool reset_time,                    // TRUE if starting timer
  bool reset_cycles                   // TRUE if cycles the cycles
  )
{
  unsigned long now;                  // Current time in seconds
  
  if ( json_tabata_on == 0 )          // Exit if no cycles are enabled
  {
    return 0;                         // Invalid time
  }

  now = millis()/100;
  
/*
 * Reset the variables based on the arguements
 */
   if ( reset_time )                  // Reset the timer
   {
    tabata_time = now;
    tabata_state = TABATA_IDLE;
    set_LED_PWM_now(0);
   }
   if ( reset_cycles )                // Reset the number of cycles
   {
    tabata_cycles = 0;
    tabata_state  = TABATA_IDLE;
   }

/*
 * Execute the state machine
 */
  switch (tabata_state)
  {
    case (TABATA_IDLE):                 // OFF, wait for the time to expire
      if ( (now - tabata_time)/10 > (json_tabata_rest - ((TABATA_BLINK_ON + TABATA_BLINK_OFF)/10)) )
      {
        tabata_state = TABATA_WARNING;
        tabata_time = now;;
        tabata_cycles++;
        set_LED_PWM_now(json_LED_PWM);  //     Turn on the lights
      }
      break;
        
   case (TABATA_WARNING):               // 1 second warning
      if ( (now - tabata_time) > TABATA_BLINK_ON )
      {
        tabata_state = TABATA_DARK;
        tabata_time = now;
        set_LED_PWM_now(0);             // Turn off the lights
      }
      break;
      
    case (TABATA_DARK):                 // 2 second dark
      if ( (now - tabata_time) > TABATA_BLINK_OFF )
      {
        tabata_state = TABATA_ON;
        tabata_time = now;
        set_LED_PWM_now(json_LED_PWM);           // Turn on the lights
        
      }
      break;
      
    case (TABATA_ON):                 // Keep the LEDs on for the tabata time
      if ( (now - tabata_time) > json_tabata_on )
      {
        if ( (json_tabata_cycles != 0) && (tabata_cycles >= json_tabata_cycles) )
        {
          tabata_state = TABATA_DONE_OFF;
        }
        else
        {
          tabata_state = TABATA_IDLE;
        }
        tabata_time = now;
        set_LED_PWM_now(0);           // Turn off the LEDs
      }
      break;

  /*
   * We have run out of cycles.  Sit there and blink the LEDs
   */
  case (TABATA_DONE_OFF):                 //Finished.  Stop the game
      if ( (now - tabata_time) > TABATA_BLINK_OFF )
      {
        tabata_state = TABATA_DONE_ON;
        tabata_time = now;
        set_LED_PWM(json_LED_PWM);       // Turn off the LEDs
      }
      break;

  case (TABATA_DONE_ON):                 //Finished.  Stop the game
      if ( (now - tabata_time) > TABATA_BLINK_ON )
      {        
        tabata_state = TABATA_DONE_OFF;
        tabata_time = now;
        set_LED_PWM(0);                   // Turn off the LEDs
      }
      break;
  }

 /* 
  * All done.  Return the current time if in the TABATA_ON state
  */
    if ( tabata_state == TABATA_ON )
    {
      return tabata_time;
    }
    else
      return 0;
 }
