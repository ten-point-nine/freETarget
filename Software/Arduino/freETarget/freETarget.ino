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
#include "timer.h"

shot_record_t record[SHOT_STRING];      //Array of shot records
unsigned int this_shot;                 // Index into the shot array
unsigned int last_shot;                 // Last shot processed.

double        s_of_sound;               // Speed of sound
unsigned int  shot = 0;                 // Shot counter
unsigned int  face_strike = 0;          // Miss Face Strike interrupt count
unsigned int  is_trace = 0;             // Turn off tracing
unsigned long rapid_on = 0;             // Duration of rapid fire event
unsigned int  rapid_count = 0;          // Number of shots to be expected in Rapid Fire
unsigned int  tabata_state;             // Tabata state
unsigned int  shot_number;              // Shot Identifier
static  long  keep_alive;               // Keep alive timer


const char* names[] = { "TARGET",                                                                                           //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR",                                                            // 26
                        0};
                  
const char nesw[]   = "NESW";                  // Cardinal Points
const char to_hex[] = "0123456789ABCDEF";      // Quick Hex to ASCII
static void bye(void);                         // Say good night Gracie
static long tabata(bool reset_time);           // Tabata state machine
static bool discard_tabata(void);              // TRUE if the shot should be discarded 

#define TABATA_OFF          0         // No tabata cycles at all
#define TABATA_REST         1         // Tabata is doing nothing (typically 60 seconds)
#define TABATA_WARNING      2         // Time the warning LED is on (typically 2 seconds)
#define TABATA_DARK         3         // Time the warning LED is off before the shot (typically 2 seconds)
#define TABATA_ON           4         // Time the TABATA lED is on (typically 5 seconds)

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
  POST_version();                         // Show the version string on all ports
  is_trace = INIT_TRACE;

  
/*
 *  Set up the port pins
 */
  init_gpio();  
  set_LED('*', '.', '.');                 // Hello World
  
  init_sensors();
  init_analog_io();
  init_timer();
  
  randomSeed( analogRead(V_REFERENCE));   // Seed the random number generator
    
  if ( CALIBRATE )
  {
    set_trip_point(0);                    // Are we calibrating?
  }
  
/* 
 *  Read the persistent storage
 */
  set_LED('.', '*', '.');                 // Hello World
  read_nonvol();
  keep_alive = millis();
  
/*
 * Initialize variables
 */
   set_LED('.', '.', '*');                // Hello World
   tabata(true);                          // Reset the Tabata timers
  
/*
 * Run the power on self test
 */
  set_LED('*', '.', '*');                 // Hello World
  while ( (POST_counters() == false)      // If the timers fail
              && !DLT(DLT_CRITICAL))      // and not in trace mode (DIAG jumper installed)
  {
    Serial.print(T("POST_2 Failed\r\n"));// Blink the LEDs
    blink_fault(POST_COUNT_FAILED);       // and try again
  }
  
/*
 * Initialize the WiFi if available
 */
   set_LED('*', '*', '.');                 // Hello World
   esp01_init();                           // Prepare the WiFi channel if installed
   
/*
 * Ready to go
 */ 
  show_echo();
  set_LED('*', '*', '*');                 // Hello World
  
  if ( VERBOSE_TRACE )                    
  {
    is_trace = DLT_INFO;
  }
  else
  {
    is_trace = DLT_NONE;
  }
  
  set_LED_PWM(json_LED_PWM);
  POST_LEDs();                            // Cycle the LEDs
  set_LED(LED_READY);                     // to a client, then the RDY light is steady on
  while ( AVAILABLE )
  {
    GET();                                // Flush any garbage before we start up
  }
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
#define SET_MODE      0           // Set the operating mode
#define ARM        (SET_MODE+1)   // State is ready to ARM
#define WAIT       (ARM+1)        // ARM the circuit and wait for a shot
#define REDUCE     (WAIT+1)     // Reduce the data
#define FINISH     (REDUCE+1)     // Wait for the shot to end

unsigned int state = SET_MODE;
unsigned int old_state = ~SET_MODE;

unsigned long timer;                  // Interval timer
unsigned long power_save;             // Power save timer
unsigned int sensor_status;           // Record which sensors contain valid data
unsigned int location;                // Sensor location 

int ch;
char* loop_name[] = {"SET_MODE", "ARM", "WAIT", "AQUIRE", "REDUCE", "FINISH" };

void loop() 
{
  unsigned int i, j;              // Iteration Counter

/*
 * First thing, handle polled devices
 */
  esp01_receive();                // Accumulate input from the IP port.
  multifunction_switch();         // Read the switches
  tabata(false);                  // Update the Tabata state
  
/*
 * Take care of any commands coming through
 */
  if ( read_JSON() )
  {
    power_save = millis();        // Reset the power down timer if something comes in
  }

/*
 * Take care of the TCPIP keep alive
 */
 if ( (json_keep_alive != 0)
    && ((millis()- keep_alive)/ 1000) > json_keep_alive )           // Time in seconds
 {
    send_keep_alive();
    keep_alive = millis();
 }

/*
 * Take care of the low power mode
 */
  if ( (json_power_save != 0 ) 
       && (((millis()-power_save) / 1000 / 60) >= json_power_save) )  // Time in minutes
  {
    bye();                              // Dim the lights
    power_save = millis();              // Came back. 
    state = SET_MODE;                   // Reset everything just in case
  }

/*
 * Cycle through the state machine
 */
  if ( (state != old_state) 
      && DLT(DLT_CRITICAL) )
  {
    Serial.print(T("Loop State: ")); Serial.print(loop_name[state]);;
  } 
  old_state = state;
  
  switch (state)
  {
/*
 *  Check for special operating modes
 */
  default:
  case SET_MODE:    // Start of the loop
    state = set_mode();
    break;

  case ARM:         // Arm the circuit
    state = arm();
    break;

  case WAIT:        // Wait for the shot to appear
    state = wait();
    break;

  case REDUCE:     // Reduce the result of one or more shots
    state = reduce();
    break;

  case FINISH:     // Finish up the cycle by movint the witness paper
    state = finish();
    break;
  }
  
/*
 * End of the loop. return
 */
  return;
}

/*----------------------------------------------------------------
 * 
 * function: set_mode()
 * 
 * brief: Set up the modes for the next string of shots
 * 
 * return: Exit to the ARM state
 * 
 *----------------------------------------------------------------
 *
 * The shot cycle is about to start.
 * 
 * This initializes the variables.
 * 
 * The illumination LED is set depending on whether or not 
 * the Tabata or Rapid fire feature is enabled
 *
 *--------------------------------------------------------------*/
 unsigned int set_mode(void)
 {
  unsigned int i;

  if ( VERBOSE_TRACE )
  {
    is_trace = DLT_DIAG;
  }
  rapid_on  = 0;                    // Turn off the timer
  power_save = millis();            // Start the power saver time
    
  for (i=0; i != SHOT_STRING; i++)
  {
    record[i].face_strike = 100;    // Disable the shot record
  }

  if ( json_tabata_enable || json_rapid_enable ) // If the Tabata or rapid fire is enabled, 
  {
    set_LED_PWM_now(0);             // Turn off the LEDs
    set_LED(LED_TABATA_ON);         // Just turn on X
    json_rapid_enable = 0;          // Disable the rapid fire
  }                                 // Until the session starts
  else
  {
    set_LED_PWM(json_LED_PWM);      // Keep the LEDs ON
  }

/*
 * Proceed to the ARM state
 */
  return ARM;                      // Carry on to the target
 }
    
/*----------------------------------------------------------------
 * 
 * function: arm()
 * 
 * brief:  Arm the circuit and check for errors
 * 
 * return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 * 
 *----------------------------------------------------------------
 *
 * The shot cycle is about to start.
 * 
 * This initializes the variables.
 * 
 * The illumination LED is set depending on whether or not 
 * the Tabata or Rapid fire feature is enabled
 *
 *--------------------------------------------------------------*/
unsigned int arm(void)
{
  face_strike = 0;                  // Reset the face strike count
  if ( json_send_miss )
  {
    enable_face_interrupt();        // Turn on the face strike interrupt
  }

  stop_timers();
  arm_timers();                     // Arm the counters

  sensor_status = is_running();     // and immediatly read the status
  if ( sensor_status == 0 )         // After arming, the sensor status should be zero
  { 
    if ( DLT(DLT_CRITICAL) )
    {
      Serial.print(T("Waiting..."));
    }  
    enable_timer_interrupt();
    return WAIT;                   // Fall through to WAIT
  }

/*
 * The sensors are tripping, display the error
 */
  if ( sensor_status & TRIP_EAST  )
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

/*
 * Got an error, try to arm again
 */
  return ARM;
}
   
/*----------------------------------------------------------------
 * 
 * function: wait()
 * 
 * brief: Wait here for a shot to be fired
 * 
 * return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 * 
 *----------------------------------------------------------------
 *
 * This loop is executed indefinitly until a shot is detected
 * 
 * In this loop check
 *    - State of the WiFi interface
 *
 *--------------------------------------------------------------*/
unsigned int wait(void)
{
/*
 * Monitor the WiFi and blink if WiFi is present but not connected
 * Set RDY to solid red if there is a connection to the PC client
 */
  if ( (esp01_is_present() == false) 
          || esp01_connected() )        // If the esp01 is not present, or connected
  {
    set_LED(LED_READY);                 // to a client, then the RDY light is steady on
  }
  else
  {
    if ( (millis() / 1000) & 1 )       // Otherwise blink the RDY light
    {
      set_LED(LED_READY);
    }
    else
    {
      set_LED(LED_OFF);
    }
  }

/*
 * Check to see if the time has run out if there is a rapid fire event in progress
 */
  if ( (json_rapid_enable == 1)         // The rapid fire timer has been turned on
          && (rapid_on != 0) )          // And there is a defined interval
  {
    if (millis() >= rapid_on)           // Do this until the timer expires
    {
      set_LED_PWM_now(0);

      if ( DLT(DLT_CRITICAL) )
      {
        Serial.print(T("Rapid fire complete"));
      }
      return FINISH;                   // Finish this rapid fire cycle
    }
    else
    {
      set_LED_PWM_now(json_LED_PWM);   // make sure the lighs are on
    }
  }

/*
 * See if any shots have arrived
 */
  if ( this_shot != last_shot )
  {
    return REDUCE;
  }
  
/*
 * All done, keep waiting
 */
  return WAIT;
}


/*----------------------------------------------------------------
 * 
 * function: reduce()
 * 
 * brief: Loop through one or more shots and present the score
 * 
 * return: Stay in the reduce state if a follow through timer is active 
 *         Jump to ARM state if more shots are ecpected
 * 
 *----------------------------------------------------------------
 *
 * This function runs in foreground and loops through the 
 * shot structure whenever there are shots in hardware to be
 * reduced.
 * 
 * In the case of rapid fire, the shot processing stops when
 * the requisite number of shots have been received.
 *
 *--------------------------------------------------------------*/
unsigned int reduce(void)
{
/*
 * See if any shots are allowed to be processed
 */
  if ( discard_shot() )                                       // Tabata is on but the shot is invalid
  {
    last_shot = this_shot;
    send_miss(&record[last_shot]);
    return FINISH;                                              // Throw out any shots while dark
  }
  
/*
 * Loop and process the shots
 */
  while (last_shot != this_shot )
  {   
    if ( DLT(DLT_CRITICAL) )
    {
      Serial.print(T("Reducing shot: ")); Serial.print(last_shot); Serial.print(T("\r\nTrigger: ")); 
      show_sensor_status(record[last_shot].sensor_status, &record[last_shot]);
    }

    location = compute_hit(&record[last_shot], false);          // Compute the score
    if ( location != MISS )                                     // Was it a miss or face strike?
    {
      if ( (json_rapid_enable == 0) && (json_tabata_enable = 0))// If in a regular session, hold off for the follow through time
      {
        delay(ONE_SECOND * json_follow_through);
      }
      send_score(&record[last_shot]);

      if ( (json_paper_time + json_step_time) != 0 )            // Has the witness paper been enabled?
      {
        if ( ((json_paper_eco == 0)                             // ECO turned off
            || ( sqrt(sq(record[this_shot].x) + sq(record[this_shot].y)) < json_paper_eco )) ) // Outside the black
        {
          drive_paper();                                        // to follow through.
        }
      } 
    }
    else
    {
      if ( DLT(DLT_CRITICAL) )
      {
        Serial.print(T("Shot miss...\r\n"));
      }
      blink_fault(SHOT_MISS);
      send_miss(&record[last_shot]);
    }

/* 
 *  Check to see if we should stop sending scores
 */
    if ( rapid_count != 0 )                                     // Decriment shots remaining
    {
      rapid_count--;                                            // in rapid fire
    }
    
/*
 * Increment and process the next
 */
    if ( (rapid_enable != 0) )                                  // In a rapid fire cycle
    {
      if ( rapid_count == 0 )                                   // And the shots used up?
      {
        last_shot = this_shot;                                  // Stop processing
        break;
      }
    }
    
    last_shot = (last_shot+1) % SHOT_STRING;
  }

/*
 * All done, Exit to FINISH if the timer has expired
 */
  if ( millis() >= rapid_on )
  {
    return FINISH;
  } 
  else
  {
    return WAIT;
  }
}
    
/*----------------------------------------------------------------
 * 
 * function: finish()
 * 
 * brief: Finish up the shot cycle 
 * 
 * return: Retrn to the SET_MODE state
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
unsigned int finish(void)
{
 
/*
 * All done, return
 */
  return SET_MODE;
}

/*----------------------------------------------------------------
 * 
 * function: tabata_enable
 * 
 * brief:    Start or stop a tabata session
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * {"TABATA_WARN_ON": 1, "TABATA_WARN_OFF":5, "TABATA_ON":7, "TABATA_REST":30, "TABATA_ENABLE":1}
 * {"TABATA_WARN_ON": 2, "TABATA_WARN_OFF":2, "TABATA_ON":7, "TABATA_REST":45, "TABATA_ENABLE":1}
 * {"TABATA_ENABLE":0}
 *--------------------------------------------------------------*/
 
 void tabata_enable
  (
    unsigned int enable     // Rapid fire enable state
  )
 {
  char str[32];
  long random_wait;

/*
 * If enabled, set up the timers
 */
  if ( enable != 0 )
  {
    set_LED_PWM_now(0);                                           // Turn off the LEDs (maybe turn them on later)
  }
  else
  {
    set_LED_PWM_now(json_LED_PWM);                                // Turn on the LED to start the cycle
  }
  
  tabata_state = TABATA_OFF;                                      // Reset back to the beginning
  json_tabata_enable = enable;                                    // And enable

  if ( DLT(DLT_APPLICATION) )
  {
    if ( enable )
    {
      Serial.print(T("Starting Tabata.  Time: "));Serial.print(json_tabata_on);
    }
    else
    {
      Serial.print(T("Tabata disabled"));
    }
  }
  
/*
 * All done, return
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
 * Test JSON 
 * {"TABATA_WARN_ON": 1, "TABATA_WARN_OFF":5, "TABATA_ON":7, "TABATA_REST":30, "TABATA_ENABLE":1}
 * {"TABATA_WARN_ON": 2, "TABATA_WARN_OFF":2, "TABATA_ON":7, "TABATA_REST":45, "TABATA_ENABLE":1}
 * 
 *-------------------------------------------------------------*/

static unsigned long tabata_start;             // Time from start of the tabata timer
static unsigned long time_end;                 // Time when timer expires

static uint16_t old_tabata_state = ~TABATA_OFF;

static long tabata
  (
  bool reset_time                     // TRUE if starting timer
  )
{
  char s[32];
  unsigned int ms50;                 //50 ms timer
  
  ms50 = millis() / 50;
  
/*
 * Reset the variables based on the arguements
 */
   if ( (json_tabata_enable == 0) || reset_time )   // Reset the timer
   {
      tabata_state = TABATA_OFF;
   }

/*
 * Execute the state machine
 */
  if ( (old_tabata_state != tabata_state ) && DLT(DLT_APPLICATION) )
  {
    Serial.print(T("Tabata State: ")); Serial.print(tabata_state); Serial.print(T("  Duration:")); Serial.print(time_end / ONE_SECOND);
  }
  
  switch (tabata_state)
  {
      case (TABATA_OFF):                  // The tabata is not enabled
        if ( json_tabata_enable != 0 )    // Just switched to enable. 
        {
          time_end = millis() + (30 * ONE_SECOND);
          set_LED_PWM_now(0);             // Turn off the lights
          sprintf(s, "{\"TABATA_STARTING\":%d}\r\n", (30));
          output_to_all(s);
          tabata_state = TABATA_REST;
        }
        break;
        
      case (TABATA_REST):                // OFF, wait for the time to expire
        if (millis() >= time_end)        // Don't do anything unless the time expires
        {
          time_end = millis() + (json_tabata_warn_on * ONE_SECOND);
          set_LED_PWM_now(json_LED_PWM);  //     Turn on the lights
          sprintf(s, "{\"TABATA_WARN\":%d}\r\n", json_tabata_warn_on);
          output_to_all(s);
          tabata_state = TABATA_WARNING;
        }
        break;
        
   case (TABATA_WARNING):                 // Warning time in seconds
        if ( (ms50 & 1) == 0 )
        {
          if ( (ms50 & 2) == 0 )
          {
            set_LED_PWM_now(0);
          }
          else
          {
          set_LED_PWM_now(json_LED_PWM);
          }
        }
        if (millis() >= time_end)         // Don't do anything unless the time expires
        {
          time_end = millis() + (json_tabata_warn_off * ONE_SECOND);
          set_LED_PWM_now(0);             // Turn off the lights
          sprintf(s, "{\"TABATA_DARK\":%d}\r\n", json_tabata_warn_off);
          output_to_all(s);
          tabata_state = TABATA_DARK;
        }
        break;
      
    case (TABATA_DARK):                   // Dark time in seconds
        if (millis() >= time_end)         // Don't do anything unless the time expires
        {
          tabata_start = millis();
          time_end = millis() + (json_tabata_on * ONE_SECOND);
          set_LED_PWM_now(json_LED_PWM);           // Turn on the lights
          sprintf(s, "{\"TABATA_ON\":%d}\r\n", json_tabata_on);
          output_to_all(s);
          tabata_state = TABATA_ON;
        }
        break;
      
    case (TABATA_ON):                     // Keep the LEDs on for the tabata time
        if (millis() >= time_end)           // Don't do anything unless the time expires
        {
          time_end = millis() + ((long)(json_tabata_rest - json_tabata_warn_on - json_tabata_warn_off) * ONE_SECOND);
          sprintf(s, "{\"TABATA_OFF\":%d}\r\n", (json_tabata_rest - json_tabata_warn_on - json_tabata_warn_off));
          output_to_all(s);
          set_LED_PWM_now(0);             // Turn off the LEDs
          tabata_state = TABATA_REST;
        }
        break;
  }

 /* 
  * All done.  Return the current time if in the TABATA_ON state
  */
    old_tabata_state = tabata_state;
    return 0;
 }


/*----------------------------------------------------------------
 * 
 * function: tabata_time
 * 
 * brief:    Return the time from tabata on until now
 * 
 * return:  Tabata time
 * 
 *----------------------------------------------------------------
 *
 *  If the tabata timer is on, then return the time from the
 *  start of the tabata cycle until the present.
 *  
 *  If the tabata timer is off, then return the current time
 *  
 *--------------------------------------------------------------*/

unsigned long tabata_time(void)
{
  if ( tabata_state == TABATA_ON )
  {
    return millis()-tabata_start;                               // Bail out now
  }
  return millis();
}


/*----------------------------------------------------------------
 * 
 * function: discard_shot
 * 
 * brief:    Determine if the shot is outside of the valid time
 * 
 * return:   TRUE if the shot is not allowed
 * 
 *----------------------------------------------------------------
 *  
 *--------------------------------------------------------------*/

bool discard_shot(void)
{
  if ( (json_rapid_enable != 0)                 // Rapid Fire
      &&  (rapid_count == 0) )                  // No shots remaining
  {
    last_shot = this_shot;
    return true;                                // Discard any new shots    
  }

  if ( (json_tabata_enable != 0)                // Tabata cycle
    && ( tabata_state != TABATA_ON ) )          // Lights not on
  {
    last_shot = this_shot;                      // Discard new shots
    return true;
  }
  
  return false;
}

/*----------------------------------------------------------------
 * 
 * function: rapid_auto
 * 
 * brief:    Start or stop a rapid fire session
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * Turn the automatic rapid fire on or off
 * 
 * Test Message
 * {"RAPID_AUTO":1}
 * 
 *--------------------------------------------------------------*/
 void rapid_auto
  (
    unsigned int enable               // Automatic enable state
  )
 {
  rapid_enable(false);                // Rapid Enable is always turned off if Auto is ON
  
  if ( DLT(DLT_APPLICATION) )
  {
    if ( enable )
    {
      Serial.print(T("\Rapid Fire armed")); 
    }
    else
    {
      Serial.print(T("Rapid Fire dis-armed")); 
    }
  }
  
/*
 * All done, return
 */
  return;
 }


/*----------------------------------------------------------------
 * 
 * function: rapid_enable
 * 
 * brief:    Start or stop a rapid fire session
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * The end of the rapid fire session (rapid_on) is computed
 * by adding the user entered time {"RAPID_ON":xx) to the current
 * time to get the end time in milliseconds
 * 
 * Test Message
 * {"RAPID_TIME":20, "RAPID_COUNT":10, "RAPID_WAIT": 5, "RAPID_ENABLE":1}
 * {"RAPID_TIME":10, "RAPID_COUNT":10, "RAPID_AUTO":1, "RAPID_ENABLE":1}
 * 
 *--------------------------------------------------------------*/
 #define RANDOM_INTERVAL 100    // 100 signals random time, %10 is the duration

 void rapid_enable
  (
    unsigned int enable     // Rapid fire enable state
  )
 {
  char str[32];
  long random_wait;

/*
 * If enabled, set up the timers
 */
  if ( enable != 0 )
  {
    set_LED_PWM_now(0);                                           // Turn off the LEDs (maybe turn them on later)
     
    if ( json_rapid_wait != 0 )
    {      
      if  ( json_rapid_wait >= RANDOM_INTERVAL )                  // > Random Interval
      {
        random_wait = random(5, json_rapid_wait % 100);           // Use bottom two digits for the time  
        if ( DLT(DLT_APPLICATION) )
        {
          Serial.print(T("RAPID_RANDOM:")); Serial.print(random_wait);
        }
        delay(random_wait * ONE_SECOND);
      }
      else
      {
        if ( DLT(DLT_APPLICATION) )
        {
          Serial.print(T("RAPID_WAIT:")); Serial.print(random_wait);
        }
        delay(json_rapid_wait * ONE_SECOND);
      }
    }
    rapid_on = millis() + ((long)json_rapid_time * 1000L);        // Duration of the event in ms
    rapid_count = json_rapid_count;                               // Number of expected shots
    shot_number = 1;
  }
  else
  {
    rapid_on = 0;
  }
   
  set_LED_PWM_now(json_LED_PWM);                                  // Turn on the LED to start the cycle
  
  json_rapid_enable = enable;

  if ( DLT(DLT_APPLICATION) )
  {
    if ( enable )
    {
      Serial.print(T("RAPID_START: ")); Serial.print(json_rapid_time);
    }
    else
    {
      Serial.print(T("RAPID_END"));
    }
  }
  
/*
 * All done, return
 */
  return;
 }

 /*----------------------------------------------------------------
 * 
 * function: bye
 * 
 * brief:    Go into power saver
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This function allows the user to remotly shut down the unit
 * when not in use
 * 
 *--------------------------------------------------------------*/
void bye(void)
{
  char str[32];
/*
 * Say Good Night Gracie!
 */
  sprintf(str, "{\"GOOD_BYE\":0}");
  output_to_all(str);
  delay(ONE_SECOND);
  tabata_enable(false);             // Turn off any automatic cycles 
  rapid_enable(false);
  set_LED_PWM(LED_PWM_OFF);         // Going to sleep 
  
/*
 * Loop waiting for something to happen
 */ 
  while ( AVAILABLE )               // Purge the com port
  {
    GET();
  }
  
  while( (DIP_SW_A == 0)            // Wait for the switch to be pressed
        && (DIP_SW_B == 0)          // Or the switch to be pressed
        && ( AVAILABLE == 0)        // Or a character to arrive
        && ( is_running() == 0) )   // Or a shot arrives
  {
    esp01_receive();                // Keep polling the WiFi to see if anything 
  }                                 // turns up
  
  hello();                          // Back in action
  
  while ( (DIP_SW_A == 1)           // Wait here for both switches to be released
            || ( DIP_SW_B == 1 ) )
  {
    continue;
  }  
  
/*
 * Come out of sleep
 */
  return;
}

/*----------------------------------------------------------------
 * 
 * function: hello
 * 
 * brief:    Come out of power saver
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This function Puts the system back in service
 * 
 *--------------------------------------------------------------*/
void hello(void)
{
  char str[128];

  sprintf(str, "{\"Hello_World\":0}");
  output_to_all(str);
  
/*
 * Woken up again
 */  
  set_LED_PWM_now(json_LED_PWM);
  power_save = millis();                            // Reset the on time
  EEPROM.get(NONVOL_POWER_SAVE, json_power_save);   // and reset the power save time
  
  return;
}

/*----------------------------------------------------------------
 * 
 * function: send_keep_alive
 * 
 * brief:    Send a keep alive over the TCPIP
 * 
 * return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * When the keep alive expires, send a new one out and reset.
 * 
 * It is sent out to the USB port as a diagnostic check 
 * 
 *--------------------------------------------------------------*/
void send_keep_alive(void)
{
  char str[32];
  static int keep_alive_count = 0;

  if ( esp01_connected() )
  {
    sprintf(str, "{\"KEEP_ALIVE\":%d}", keep_alive_count++);
    output_to_all(str);
  }
  return;
}
