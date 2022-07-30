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

shot_record record[20];                 //Array of shot records
unsigned int this_shot;                 // Index into the shot array

double       s_of_sound;                // Speed of sound
unsigned int shot = 0;                  // Shot counter
unsigned int face_strike = 0;           // Miss Face Strike interrupt count
bool         is_trace = false;          // TRUE if trace is enabled

const char* names[] = { "TARGET",                                                                                           //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR",                                                            // 26
                        0};
                  
const char nesw[]   = "NESW";                  // Cardinal Points
const char to_hex[] = "0123456789ABCDEF";      // Quick Hex to ASCII
static void bye(void);                         // Say good night Gracie
bool        target_hot;                        // TRUE if the target is active
static  long keep_alive;                       // Keep alive timer

static long tabata(bool reset_time);           // Tabata state machine

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
  
  randomSeed( analogRead(V_REFERENCE));   // Seed the random number generator
    
  if ( CALIBRATE )
  {
    set_trip_point(0);                    // Are we calibrating?
  }
/* 
 *  Read the persistent storage
 */
  read_nonvol();
  multifunction_init();
  keep_alive = millis();
  
/*
 * Initialize the WiFi if available
 */
   esp01_init();                           // Prepare the WiFi channel if installed

/*
 * Initialize variables
 */
   tabata(true);                          // Reset the Tabata timers
  
/*
 * Run the power on self test
 */
  POST_version();                         // Show the version string on all ports
  show_echo(0);
  POST_LEDs();                            // Cycle the LEDs
  while ( (POST_counters() == false)      // If the timers fail
              && !is_trace)               // and not in trace mode (DIAG jumper installed)
  {
    Serial.print(T("\r\nPOST_2 Failed\r\n"));// Blink the LEDs
    blink_fault(POST_COUNT_FAILED);       // and try again
  }

/*
 * Ready to go
 */
  set_LED_PWM(json_LED_PWM);
  set_LED(LED_READY);                   // to a client, then the RDY light is steady on
  target_hot = true;
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
#define WAIT       (ARM+1)        // ARM the circuit
#define AQUIRE     (WAIT+1)       // Aquire the shot
#define REDUCE     (AQUIRE+1)     // Reduce the data
#define FINISH     (REDUCE+1)     // Wait for the shot to end

unsigned int state = SET_MODE;
unsigned long timer;            // Interval timer
unsigned long follow_through;   // Follow through timer
unsigned long power_save;       // Power save timer
unsigned int sensor_status;     // Record which sensors contain valid data
unsigned int location;          // Sensor location 
unsigned int i, j;              // Iteration Counter
int ch;
unsigned int shot_number;

void loop() 
{
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
    set_LED_PWM(json_LED_PWM);    // Put the LED back on if it was off
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
  switch (state)
  {

/*
 *  Check for special operating modes
 */
  default:
  case SET_MODE:                      // Start of the loop
    is_trace |= VERBOSE_TRACE;        // Turn on tracing if the DIP switch is in place
    this_shot = 0;                    // Reset the shot counter
    json_rapid_on = 0;                // Reset the rapid timer if it was on
    power_save = millis();            // Start the power saver time
    
    for (i=0; i != sizeof(record)/sizeof(shot_record); i++)
    {
      record[i].face_strike = 100;    // Disable the shot record
    }

    if ( json_tabata_enable || json_rapid_enable ) // If the Tabata or rapid fire is enabled, 
    {
      set_LED_PWM_now(0);             // Turn off the LEDs
      set_LED(LED_TABATA_ON);         // Just turn on X
    }                                 // Until the session starts
    else
    {
      set_LED_PWM(json_LED_PWM);      // Keep the LEDs ON
    }

    state = ARM;                      // Carry on to the target
    break;
    
/*
 * Arm the circuit
 */
  case ARM:
    face_strike = 0;                  // Reset the face strike count
    enable_interrupt(json_send_miss); // Turn on the face strike interrupt
    arm_counters();                   // Arm the counters
    sensor_status = is_running();     // and immediatly read the status
    if ( sensor_status == 0 )         // After arming, the sensor status should be zero
    { 
      if ( is_trace )
      {
        Serial.print(T("\r\n\nWaiting..."));
      }
      state = WAIT;                   // Fall through to WAIT
    }
    else                              // Non-zero, false trip
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
    break;
    
/*
 * Wait for the shot.  If rapid fire is enabled, wait here for the duration of the session
 */
  case WAIT:
    if ( (esp01_is_present() == false) 
          || esp01_connected() )          // If the ESP01 is not present, or connected
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

    if ( json_rapid_on != 0 )             // The rapid fire timer has been turned on
    {
      set_LED_PWM_now(json_LED_PWM);      // make sure the lighs are on
      if (millis() >= json_rapid_on)      // Do this until the timer expires
      {
        set_LED_PWM_now(0);
        follow_through = millis();        // No follow through on rapid fire
        state = REDUCE;                   // Reduce the shot string
        if ( is_trace )
        {
          Serial.print(T("\n\rRapid fire complete"));
        }
        break;  
      }
    }

    sensor_status = is_running();
    if ( (sensor_status != 0 )            // Detected a shot
        || (face_strike >= json_face_strike)) // or omething hit the front
    {
      timer = micros() + SHOT_TIME;       // Figure out the end time
      set_LED(L('-', '*', '-'));          // No longer waiting
      state = AQUIRE;                     // Fake a shot and display it
    }

    break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    if ( micros() < timer )                             // Aquire time has not expired yet
    {
      break;
    }
    stop_timers();                                    // Stop the counters
    read_timers(&record[this_shot].timer_value[0]);   // Record this count
    record[this_shot].shot_time = tabata_time();      // Capture the time into the shot
    record[this_shot].face_strike = face_strike;      // Record if it's a face strike
    record[this_shot].sensor_status = sensor_status;  // Record the sensor status
    this_shot = (this_shot+1) % (sizeof(record)/sizeof(shot_record));  // Prepare for the next shot
    if ( (json_rapid_enable == 0) || (this_shot >= json_rapid_count) ) // Finished the current shot cycle? (rapid or single)
    {
      follow_through = millis() + (json_follow_through * ONE_SECOND); // Calculate the follow through delqy
      set_LED(LED_DONE);                              // Light All
      state = REDUCE;                                 // And since the shots are over, reduce.
    }
    else
    {
      state = ARM;                                    // More shots, re-arm and wait
    }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:
    if ( millis() < follow_through )        // Wait for the follow through timer to expire before sending score
    {
      break;
    }
    
    for ( i=0; i != this_shot; i++ )        // Loop and process the shots that have come in
    {   
      if ( is_trace )
      {
        Serial.print(T("\r\nReducing shot: ")); Serial.print(i);
        Serial.print(T("\r\nTrigger: ")); 
        show_sensor_status(record[i].sensor_status);
      }
      location = compute_hit(&record[i], false);          // Compute the score
      if ( location == MISS )                             // Was it a miss or face strike?
      {
        if ( is_trace )
        {
          Serial.print(T("\r\nFace Strike...\r\n"));
        } 
        blink_fault(SHOT_MISS);
        send_miss(&record[i], shot_number);
      }
      else
      {
        send_score(&record[i], shot_number);
      }   
      shot_number++;          
    }
    state = FINISH;
    break;
    

/*
 *  Wait here to make sure the follow through time is over
 */
  case FINISH:
    if ( (json_paper_time + json_step_time) != 0 )  // Has the witness paper been enabled?
    {
      if ( ((json_paper_eco == 0)                   // ECO turned off
        || ( sqrt(sq(record[this_shot].x) + sq(record[this_shot].y)) < json_paper_eco )) // Outside the black
          && (json_rapid_on == 0))                  // and not rapid fire
      {
        drive_paper();                              // to follow through.
      }
    }  
    state = SET_MODE;                               // Next time go to waste time 
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
 * Test JSON 
 * {"TABATA_WARN_ON": 2, "TABATA_WARN_OFF":2, "TABATA_ON":7, "TABATA_REST":45, "TABATA_ENABLE":1}
 * 
 *-------------------------------------------------------------*/
#define TABATA_OFF          0         // No tabata cycles at all
#define TABATA_REST         1         // Tabata is doing nothing (typically 60 seconds)
#define TABATA_WARNING      2         // Time the warning LED is on (typically 2 seconds)
#define TABATA_DARK         3         // Time the warning LED is off before the shot (typically 2 seconds)
#define TABATA_ON           4         // Time the TABATA lED is on (typically 5 seconds)

static unsigned long tabata_start;             // Time from start of the tabata timer
static unsigned long time_end;                 // Time when timer expires
static uint16_t tabata_state = TABATA_OFF;
static uint16_t old_tabata_state = ~TABATA_OFF;

static long tabata
  (
  bool reset_time                     // TRUE if starting timer
  )
{
  char s[32];

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
  if ( is_trace )
  {
    if (old_tabata_state != tabata_state )
    {
      Serial.print(T("\r\nTabata State: ")); Serial.print(tabata_state);
    }
  }
  
  switch (tabata_state)
  {
      case (TABATA_OFF):                 // The tabata is not enabled
        if ( json_tabata_enable != 0 )
        {
          time_end = millis() + (json_tabata_rest * ONE_SECOND);
          tabata_state = TABATA_REST;
        }
        break;
        
      case (TABATA_REST):                // OFF, wait for the time to expire
        if (millis >= time_end)          // Don't do anything unless the time expires
        {
          time_end = millis() + (json_tabata_warn_on * ONE_SECOND);
          tabata_state = TABATA_WARNING;
          set_LED_PWM_now(json_LED_PWM);  //     Turn on the lights
        }
        break;
        
   case (TABATA_WARNING):                 // Warning time in seconds
        if (millis >= time_end)           // Don't do anything unless the time expires
        {
          tabata_state = TABATA_DARK;
          time_end = millis() + (json_tabata_warn_off * ONE_SECOND);
          set_LED_PWM_now(0);             // Turn off the lights
        }
        break;
      
    case (TABATA_DARK):                   // Dark time in seconds
        if (millis >= time_end)           // Don't do anything unless the time expires
        {
          tabata_state = TABATA_ON;
          tabata_start = millis();
          time_end = millis() + (json_tabata_on * ONE_SECOND);
          set_LED_PWM_now(json_LED_PWM);           // Turn on the lights
          sprintf(s, "{\"TABATA\":1}\r\n");
          output_to_all(s);
        }
        break;
      
    case (TABATA_ON):                     // Keep the LEDs on for the tabata time
        if (millis >= time_end)           // Don't do anything unless the time expires
        {
          tabata_state = TABATA_REST;
          time_end = millis() + ((json_tabata_rest - json_tabata_warn_on - json_tabata_warn_off) * ONE_SECOND);
          sprintf(s, "{\"TABATA\":0}\r\n");
          output_to_all(s);
          set_LED_PWM_now(0);             // Turn off the LEDs
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
 * return:  Tabata time updated
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
 * function: rapid_start
 * 
 * brief:    Start a rapid fire session
 * 
 * return:   Time to end rapid fire session
 * 
 *----------------------------------------------------------------
 *
 * The end of the rapid fire session (json_rapid_on) is computed
 * by adding the user entered time {"RAPID_ON":xx) to the current
 * time to get the end time in milliseconds
 * 
 * Test Message
 * {"RAPID_ENABLE":1, "RAPID_COUNT":10, "RAPID_TIME":10}
 * 
 *--------------------------------------------------------------*/
 void rapid_start
  (
    unsigned int duration     // Duration in seconds
  )
 {
  if ( duration == 0 )
  {
    json_rapid_on = 0;
    json_rapid_enable = 0;
  }
  else
  {
    json_rapid_on = millis() + (duration * 1000);
  }

  if ( is_trace )
  {
    Serial.print(T("\n\rStarting Rapid Fire:")); Serial.print(json_rapid_on); Serial.print(T("  Shots:")); Serial.print(json_rapid_count);
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
  set_LED_PWM(LED_PWM_OFF);         // Going to sleep 
  delay(ONE_SECOND);
  
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
  target_hot = true;
  
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
