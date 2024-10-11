/*----------------------------------------------------------------
 * 
 * freETarget        
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *-------------------------------------------------------------*/
#include "esp_timer.h"
#include "driver\gpio.h"
#include "esp_random.h"
#include "stdio.h"
#include "math.h"
#include "nvs.h"
#include "mpu_wrappers.h"

#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "nonvol.h"
#include "mechanical.h"
#include "diag_tools.h"
#include "timer.h"
#include "token.h"
#include "serial_io.h"
#include "dac.h"
#include "pcnt.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "mfs.h"
/*
 *  Function Prototypes
 */
static unsigned int set_mode(void);     // Set the target running mode
static unsigned int arm(void);          // Arm the circuit for a shot
static unsigned int wait(void);         // Wait for the shot to arrive
static unsigned int reduce(void);       // Reduce the shot data
static bool discard_shot(void);         // In TabataThrow away the shot
extern void gpio_init(void);

/*
 *  Variables
 */
shot_record_t record[SHOT_SPACE];      //Array of shot records
unsigned int shot_in;                   // Index to the shot just received
unsigned int shot_out;                  // Index to the shot just sent to the PC (shot_out <= shot_in)

double        s_of_sound;               // Speed of sound
unsigned int  shot = 0;                 // Shot counter
unsigned int  face_strike = 0;          // Miss Face Strike interrupt count
unsigned int  is_trace = DLT_INFO | DLT_CRITICAL; // Default tracing

unsigned int  rapid_count = 0;          // Number of received shots
unsigned int  shot_number;              // Shot Identifier (1-100)
volatile unsigned long  in_shot_timer;  // Time inside of the shot window

static volatile unsigned long  keep_alive;        // Keep alive timer
static volatile unsigned long  tabata_timer;      // Free running state timer
       volatile unsigned long  power_save;        // Power save timer
static volatile unsigned long  rapid_timer;       // Timer used for rapid fire ecents 
       volatile unsigned long  LED_timer;         // Timer to reset LED status 

static enum 
{
  START = 0,              // 0 et the operating mode
  WAIT,                   // 1 ARM the circuit and wait for a shot
  REDUCE,                 // 2 Reduce the data and send the score
  DARK                    // 3 Go dark for a while
  } state;

static enum
{
  TABATA_OFF = 0,         // 0 No tabata cycles at all
  TABATA_REST,            // 1 Tabata is doing nothing (typically 60 seconds)
  TABATA_WARNING,         // 2 Time the warning LED is on (typically 2 seconds)
  TABATA_DARK,            // 3 Time the warning LED is off before the shot (typically 2 seconds)
  TABATA_ON               // 4 Time the TABATA lED is on (typically 5 seconds)
} tabata_state;

static enum {
  RAPID_OFF = 0,          // 0 No rapid fire cycles at all
  RAPID_WAIT,             // 1 Rapid fire is doing nothing (typically 60 seconds)
  RAPID_ON,               // 2 Time the RAPID lED is on (typically 5 seconds)
  RAPID_SEND              // 3 The event is over, send the results
} rapid_state;            // Rapid fire state

volatile unsigned int run_state = 0;              // Current operating state 

const char* names[] = { "TARGET",                                                                                           //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR",                                                            // 26
                        0};
                  
const char    to_hex[] = "0123456789ABCDEF";      // Quick Hex to ASCII

char   _xs[512];       // Holding buffer for sprintf

/*----------------------------------------------------------------
 * 
 * @function: freeETarget_init()
 * 
 * @brief: Initialize the board and prepare to run
 * 
 * @return: None
 * 
 *--------------------------------------------------------------*/

void freeETarget_init(void)
{    

  run_state = IN_STARTUP;

/*
 *  Setup the hardware
 */
  json_aux_port_enable = false;           // Assume the AUX port is not used
  gpio_init();                            // Setup the hardware
  serial_io_init();                       // Setup the console for debug messages
  read_nonvol();                          // Read in the settings
  serial_aux_init();                      // Update the serial port if there is a change
  POST_version();                         // Show the version string on all ports
  set_VREF();
  multifunction_init();                   // Override the MFS if we have to

  set_status_LED(LED_RAPID_RED_OFF);
  set_status_LED(LED_RAPID_GREEN_OFF);
  set_status_LED(LED_HELLO_WORLD);        // Hello World
  set_status_LED(LED_RAPID_RED);          // Red
  timer_delay(ONE_SECOND);
  set_status_LED(LED_RAPID_OFF);
  set_status_LED(LED_RAPID_GREEN);        // Green
  timer_delay(ONE_SECOND);
  set_status_LED(LED_OFF);
  set_status_LED(LED_RAPID_OFF);          // Off

  WiFi_init();

/*
 *  Set up the long running timers
 */
  timer_new(&keep_alive,    (unsigned long)json_keep_alive * ONE_SECOND * 60l); // Keep alive timer
  timer_new(&in_shot_timer, FULL_SCALE);                                  // Time inside of the shot window
  timer_new(&power_save,    (unsigned long)(json_power_save) * (long)ONE_SECOND * 60L);// Power save timer

/*
 * Run the power on self test
 */
  POST_counters();                // POST counters does not return if there is an error
  if ( check_12V() == false )     // Verify the 12 volt supply
  {
    DLT(DLT_INFO, SEND(sprintf(_xs, "12V supply not present");))
  }

/*
 * Ready to go
 */ 
  show_echo();
  set_LED_PWM(json_LED_PWM);
  serial_flush(ALL);                      // Get rid of everything
  shot_in = 0;                            // Clear out any junk
  shot_out = 0;
  DLT(DLT_INFO, SEND(sprintf(_xs, "Initialization complete");) )

/*
 * Start the tasks running
 */
  run_state &= ~IN_STARTUP;               // Exit startup 
  return;
}

/*----------------------------------------------------------------
 * 
 * @function: freeEtarget_task
 * 
 * @brief: Main control loop
 * 
 * @return: None
 * 
 *----------------------------------------------------------------
 */

unsigned int  sensor_status;          // Record which sensors contain valid data
unsigned int  location;               // Sensor location 

void freeETarget_target_loop(void* arg)
{

  DLT(DLT_INFO, SEND(sprintf(_xs, "freeETarget_target_loop()");))
  set_status_LED(LED_READY);

  shot_number = 1;                    // Start counting shots at 1

  while(1)
  {
    IF_IN( IN_SLEEP | IN_TEST)        // If Not in operation, 
    {
      run_state &= ~ IN_OPERATION;    // Exit operation
      vTaskDelay(ONE_SECOND);
      continue;
    }

    run_state |= IN_OPERATION;        // In operation 

/*
 * Cycle through the state machine
 */
    switch (state)
    {
      default:
        case START:                     // Start of the loop
        DLT(DLT_APPLICATION, SEND(sprintf(_xs, "state: START");))
        power_save = (unsigned long)json_power_save * (unsigned long)ONE_SECOND * 60L;  //  Reset the timer
        set_mode();
        arm();
        set_status_LED(LED_READY);
        state = WAIT;
        json_rapid_count = 0;
        DLT(DLT_APPLICATION, SEND( sprintf(_xs, "state: WAIT");))
        break;
    
      case WAIT:  
        state = wait();
        break;

      case REDUCE:  
        DLT(DLT_APPLICATION, SEND(sprintf(_xs, "state: REDUCE");))
        reduce();
        state = DARK;
        if ( json_rapid_enable == true )       // 
        {
          timer_new(&rapid_timer, ONE_SECOND * 10);
          state = DARK;
          DLT(DLT_APPLICATION, SEND(printf(_xs, "state: DARK");))
        }      
        else 
        {
          state = START;
        }
        break;

      case DARK:                                // Ghost state to say dark for 10 seconds
       if ( rapid_timer == 0 )
        {
          state = START;      
          rapid_state = RAPID_OFF;              // No longer in rapid fire
          json_rapid_enable = false;            // No longer enabled
        }
        break;
    }
/*
 * End of the loop. timeout till the next time
 */
    vTaskDelay(1);
  }
}

/*----------------------------------------------------------------
 * 
 * @function: set_mode()
 * 
 * @brief: Set up the modes for the next string of shots
 * 
 * @return: Exit to the ARM state
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

  DLT(DLT_APPLICATION, SEND(sprintf(_xs, "set_mode()");))

  for (i=0; i != SHOT_SPACE; i++)
  {
    record[i].face_strike = 100;    // Disable the shot record
  }

  if ( json_tabata_enable || json_rapid_enable ) // If the Tabata or rapid fire is enabled, 
  {
    set_LED_PWM_now(0);             // Turn off the LEDs
  }                                 // Until the session starts
  else
  {
    set_LED_PWM(json_LED_PWM);      // Keep the LEDs ON
  }

/*
 * Proceed to the ARM state
 */
  return WAIT;                      // Carry on to the target
 }
    
/*----------------------------------------------------------------
 * 
 * @function: arm()
 * 
 * @brief:  Arm the circuit and check for errors
 * 
 * @return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 * 
 *----------------------------------------------------------------
 *
 * The shot cycle is about to start.
 * 
 * This initializes the variables.
 * 
 *
 *--------------------------------------------------------------*/
unsigned int arm(void)
{
  DLT(DLT_APPLICATION, SEND(sprintf(_xs, "arm()");))

  face_strike = 0;                  // Reset the face strike count
  stop_timers();
  arm_timers();                     // Arm the counters

  sensor_status = is_running();     // and immediatly read the status
  if ( sensor_status == 0 )         // After arming, the sensor status should be zero
  { 
    return WAIT;                   // Fall through to WAIT
  }

/*
 * The sensors are tripping, display the error
 */
  set_diag_LED(find_sensor(sensor_status)->diag_LED, 5);

/*
 * Finished displaying the error so trying again
 */
  return START;
}
   
/*----------------------------------------------------------------
 * 
 * @function: wait()
 * 
 * @brief: Wait here for a shot to be fired
 * 
 * @return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 * 
 *----------------------------------------------------------------
 *
 * This loop is executed indefinitly until a shot is detected
 * 
 * Once the shot has been detected (or rapid fire complete) the
 * state machine calls reduce() to process the timers and send
 * the score.
 * 
 * IMPORTANT
 * 
 * Since the shots are buffered in a queue, it is possible for
 * the wait loop to be behind by more than one shot.
 * 
 *--------------------------------------------------------------*/
unsigned int wait(void)
{

/*
 * Check to see if the time has run out if there is a rapid fire event in progress
 */
  if ( json_rapid_enable == true )
  {
    if ( rapid_state == RAPID_SEND )
    {
      DLT(DLT_APPLICATION, SEND(sprintf(_xs, "Rapid fire complete");))
      return REDUCE;                   // Finish this rapid fire cycle
    }
    return WAIT;
  }

/*
 * See if any shots have arrived
 */
  if ( shot_in != shot_out )
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
 * @function: reduce()
 * 
 * @brief: Loop through one or more shots and present the score
 * 
 * @return: Stay in the reduce state if a follow through timer is active 
 *         Jump to ARM state if more shots are ecpected
 * 
 *----------------------------------------------------------------
 *
 * This function runs in foreground and loops through the 
 * shot structure whenever there are shots in hardware to be
 * reduced.
 * 
 * In the case of Rapid Fire, the Rapid fire loop will hold all
 * of the shots until the time runs out our all of the shots have
 * been made.
 * 
 * Sample Settings
 * 
 * {"PAPER_ECO":0,  "PAPER_SHOT": 0}
 * {"PAPER_ECO":10, "PAPER_SHOT": 5}
 * 
 *--------------------------------------------------------------*/
unsigned int reduce(void)
{
  static unsigned int paper_shot = 0;             // Count of reduced shots
         unsigned int missing_shots = 0;          // Rapid fire shots that were not shot

/*
 * See if any shots are allowed to be processed
 */
  if ( discard_shot() )                          // Tabata is on but the shot is invalid
  {
    DLT(DLT_APPLICATION, SEND(sprintf(_xs, "Discarding shot");))
    shot_out = shot_in;
    send_miss(&record[shot_out], shot_out);
    return START;                                // Throw out any shots while dark
  }
  
/*
 * Loop and process the shots
 */
  while (shot_out != shot_in )                    // Process the shots on the queue
  {   
    DLT(DLT_DEBUG, SEND(sprintf(_xs, "shot_in: %d,  shot_out:%d", shot_in, shot_out);))
    DLT(DLT_DEBUG, show_sensor_status(record[shot_out].sensor_status);)

    show_sensor_fault(record[shot_out].sensor_status);
    
    location = compute_hit(&record[shot_out]);                 // Compute the score

/*
 *  Delay for a follow through
 */
    if ( location != MISS )                                     // Was it a miss or face strike?
    {
      if ( (json_rapid_enable == false) && (json_tabata_enable = false))// If in a regular session, hold off for the follow through time
      {
        vTaskDelay(ONE_SECOND * json_follow_through);
      }
      send_score(&record[shot_out], shot_out);

/*
 *  Advance the paper
 */
      if ( IS_DC_WITNESS || IS_STEPPER_WITNESS )                                // Has the witness paper been enabled?
      {
        DLT(DLT_DEBUG, SEND(sprintf(_xs, "paper_shot: %d,  json_paper_shot:%d, rapid_count:%d, rapid_state: %d", paper_shot, json_paper_shot, rapid_count, rapid_state );))
        DLT(DLT_DEBUG, SEND(sprintf(_xs, "shot_xs:    %4.2f,  shot_ys:%4.2f, paper_eco:%d", record[shot_out].xs, record[shot_out].ys, json_paper_eco);))

        if ( ((json_paper_eco == 0)                                             // PAPER_ECO turned off
              || ( sqrt(sq(record[shot_out].xs) + sq(record[shot_out].ys)) < (json_paper_eco / 2) )) ) // Inside the black (radius)
        {
          paper_shot++;
          DLT(DLT_DEBUG, SEND(sprintf(_xs, "Good shot: %d/%d", paper_shot, json_paper_shot);))
          if ( ((json_paper_shot == 0) && (rapid_state == RAPID_OFF))           // Paper not limited, and not a rapid sequnce
                || ((json_paper_shot != 0 ) && (paper_shot >= json_paper_shot)) // Or we have reached the required number opf hits?
                || ((json_rapid_count != 0 ) && (paper_shot >= rapid_count) ) ) // Or rapid fire has finished
          {
            paper_start();                                                      // Roll the paper
            paper_shot = 0;                                                     // And start over
          }
        }
      } 
    }
    else
    {
      DLT(DLT_APPLICATION, SEND(sprintf(_xs, "Shot miss...\r\n");))
      set_status_LED(LED_MISS);
      send_miss(&record[shot_out], shot_out);                                 // Show a miss
    }
    shot_out = (shot_out+1) % SHOT_SPACE;                                     // Increment to the next shot
  }

/*
 *  Take care of the special case where the actual shots are LESS than programmed for rapid fire
 */
  if (json_rapid_enable == true)
  {
    while ( rapid_count != json_rapid_count )                                 // And shots were incomplete
    {
      send_miss(&record[shot_out], shot_out);
      shot_out = (shot_out+1) % SHOT_SPACE;
      rapid_count++;
    }
  }

/*
 * All done, Exit to FINISH if the timer has expired
 */
  while ( ring_timer != 0 )           // Wait here to make sure the ringing has stopped
  {
    DLT(DLT_DEBUG, SEND(sprintf(_xs, "ring_timer: %ld", ring_timer);))
    vTaskDelay(10);
  }

  if ( tabata_timer == 0 )
  {
    return START;
  } 
  else
  {
    return WAIT;
  }
}

/*----------------------------------------------------------------
 * 
 * @function: start_new_session()
 * 
 * @brief: Reset and start a new session from the beginning
 * 
 * @return: None
 * 
 *----------------------------------------------------------------
 *
 * The target can be commanded to start a new session by receiving
 * the command {"START"}
 * 
 * This function resets the various counters and pointers back
 * to the beginning
 *
 *--------------------------------------------------------------*/
void start_new_session(void)
{
  unsigned int i;

  DLT(DLT_APPLICATION, SEND(sprintf(_xs, "start_new_session()");))
/*
 *  Clear the shot information
 */
  shot_out = 0;
  shot_in = 0;
    
  for (i=0; i != SHOT_SPACE; i++)
  {
    record[i].is_valid = false;
  }

/*
 *  All done, return
 */
  return;
}
/*----------------------------------------------------------------
 * 
 * @function: tabata_enable
 * 
 * @brief:    Start or stop a tabata session
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * {"TABATA_WARN_ON": 1, "TABATA_WARN_OFF":5, "TABATA_ON":7, "TABATA_REST":30, "TABATA_ENABLE":1}
 * {"TABATA_WARN_ON": 5, "TABATA_WARN_OFF":2, "TABATA_ON":7, "TABATA_REST":45, "TABATA_ENABLE":1}
 * {"TABATA_ENABLE":0}
 * 
 *--------------------------------------------------------------*/

void tabata_enable
(
  int enable     // Rapid fire enable state
)
{
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

  DLT(DLT_APPLICATION, 
  {
    if ( enable )
    {
      SEND(sprintf(_xs, "Starting Tabata.  Time: %d", json_tabata_on);)
    }
    else
    {
      SEND(sprintf(_xs, "Tabata disabled");)
    }
  }
  )

/*
 * All done, return
 */
  return;
 }

/*----------------------------------------------------------------
 * 
 * @function: tabata
 * 
 * @brief:   Implement a Tabata timer for training
 * 
 * @return:  Time that the current TABATA state == TABATA_ON
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
 * {"MFS_HOLD_C":18, "MFS_HOLD_D":20, "MFS_SELECT_CD":22}
 * 
 *-------------------------------------------------------------*/
void tabata_task(void)
{
  IF_NOT(IN_OPERATION) return;

/*
 * Exit if rapid fire has not been enabled
 */
  if ( json_tabata_enable == 0 )        // Reset the timer
  {
    timer_delete(&tabata_timer);      
    tabata_state = TABATA_OFF;
    return;
  }

/*
 *  Execute the Tabata state machine
 */    
  switch (tabata_state)
  {
    case (TABATA_OFF):                  // First time through after enable
      timer_new(&tabata_timer, json_tabata_on * ONE_SECOND);
      set_LED_PWM_now(0);               // Turn off the lights
      set_status_LED(LED_TABATA_OFF);
      SEND(sprintf(_xs, "{\"TABATA_REST\":%d}\r\n", json_tabata_on);)
      tabata_state = TABATA_REST;
      break;
        
    case (TABATA_REST):                 // OFF, wait for the time to expire
      if (tabata_timer == 0)            // Don't do anything unless the time expires
      {
        timer_new(&tabata_timer, json_tabata_warn_on * ONE_SECOND);
        set_LED_PWM_now(json_LED_PWM);  //     Turn on the lights
        set_status_LED(LED_TABATA_WARN);
        SEND(sprintf(_xs, "{\"TABATA_WARNING\":%d}\r\n", json_tabata_warn_on);)
        tabata_state = TABATA_WARNING;
      }
      break;
        
    case (TABATA_WARNING):                  // Warning time in seconds
      if ( (tabata_timer % 50) == 0 )       // Blink the LEDs during the warning
      {
        if ( ((tabata_timer / 50) & 1) == 0 )
        {
          set_LED_PWM_now(0);
        }
        else
        {
        set_LED_PWM_now(json_LED_PWM);
        }
      }
      if (tabata_timer == 0)         // Don't do anything unless the time expires
      {
        timer_new(&tabata_timer, json_tabata_warn_off * ONE_SECOND);
        set_LED_PWM_now(0);             // Turn off the lights
        set_status_LED(LED_TABATA_OFF);
        SEND(sprintf(_xs, "{\"TABATA_DARK\":%d}\r\n", json_tabata_warn_off);)
        tabata_state = TABATA_DARK;
      }
      break;
      
    case (TABATA_DARK):                   // Dark time in seconds
      if (tabata_timer == 0 )             // Don't do anything unless the time expires
      {
        in_shot_timer = FULL_SCALE;       // Set the timer on
        timer_new(&tabata_timer, json_tabata_on * ONE_SECOND);
        set_LED_PWM_now(json_LED_PWM);    // Turn on the lights
        set_status_LED(LED_TABATA_ON);
        SEND(sprintf(_xs, "{\"TABATA_ON\":%d}\r\n", json_tabata_on);)
        tabata_state = TABATA_ON;
      }
      break;
      
    case (TABATA_ON):                     // Keep the LEDs on for the tabata time
      if ( tabata_timer == 0 )            // Don't do anything unless the time expires
      {
        timer_new(&tabata_timer, (long)(json_tabata_rest - json_tabata_warn_on - json_tabata_warn_off) * ONE_SECOND);
        SEND(sprintf(_xs, "{\"TABATA_OFF\":%d}\r\n", (json_tabata_rest - json_tabata_warn_on - json_tabata_warn_off));)
        set_LED_PWM_now(0);             // Turn off the LEDs
        set_status_LED(LED_TABATA_OFF);
        tabata_state = TABATA_REST;
      }
      break;
    }

 /* 
  * All done.  Return the current time if in the TABATA_ON state
  */
    return;
}

/*----------------------------------------------------------------
 * 
 * @function: discard_shot
 * 
 * @brief:    Determine if the shot is outside of the valid time
 * 
 * @return:   TRUE if the shot is not allowed
 * 
 *----------------------------------------------------------------
 *  
 * In rapid fire or Tabata, a shot is only valid if the feature is
 * enabled, and there is time on the clock.
 * 
 *--------------------------------------------------------------*/

static bool discard_shot(void)
{

  if ( (json_tabata_enable != 0)                // Tabata cycle
    && ( tabata_state != TABATA_ON ) )          // Lights not on
  {
    shot_out = shot_in;          // Discard new shots
    return true;
  }
  
  return false;
}

/*----------------------------------------------------------------
 * 
 * @function: rapid_fire
 * 
 * @brief:    Start or stop a rapid fire session
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This is a state machine to control the LEDs and timers for
 * the rapid fire event.
 *  
 * At the end of the cycle, the state RAPID_SEND is used top send
 * a message to the wait loop to begin processing the shots
 * 
 * Test Message
 * {"RAPID_TIME":20, "RAPID_COUNT":10, "RAPID_WAIT":5, "RAPID_ENABLE":1}
 * {"RAPID_TIME":10, "RAPID_COUNT":10, "RAPID_WAIT":0,  "RAPID_ENABLE":1}
 * {"RAPID_TIME":20, "RAPID_COUNT":5,  "RAPID_WAIT":0,  "RAPID_ENABLE":1}
 * {"RAPID_TIME":30, "RAPID_COUNT":5,  "RAPID_WAIT":5,  "RAPID_ENABLE":1}
 * {"MFS_HOLD_C":18, "MFS_HOLD_D":20, "MFS_SELECT_CD":22, "RAPID_TIME":60, "RAPID_COUNT":5,  "RAPID_WAIT":0,  "RAPID_ENABLE":1}
 * 
 *--------------------------------------------------------------*/
#define RANDOM_INTERVAL 100    // 100 signals random time, %10 is the duration

void rapid_fire_task(void)
{
  static int new_shot = 0;      // A new shot has arrived

  IF_NOT(IN_OPERATION) return;

/*
 * Exit if Rapid fire has not been enabled
 */
  if ( json_rapid_enable == false )        // Reset the state machine
  {
    rapid_state = RAPID_OFF;
    return;
  }

/*
 * Take care of the rapid fire state
 */
  DLT(DLT_APPLICATION, SEND(sprintf(_xs, "Rapid State: %d,  rapid_timer: %ld,  shot_in: %d,  shot_out: %d", rapid_state, rapid_timer, shot_in, shot_out);))
  
  switch (rapid_state)
  {
    case (RAPID_OFF):                   // The tabata is not enabled
      set_status_LED(LED_RAPID_OFF);
      if ( json_rapid_enable != false ) // Just switched to enable. 
      {
        timer_new(&rapid_timer, json_rapid_wait * ONE_SECOND);
        set_LED_PWM_now(0);             // Turn off the lights
        set_status_LED(LED_RAPID_GREEN_WARN);
        SEND(sprintf(_xs, "{\"RAPID_WAIT\":%d}\r\n", (int)json_rapid_wait);)
        rapid_state = RAPID_WAIT;
        rapid_count = 0;                // No shots received yet

      } 
      break;
        
    case (RAPID_WAIT):                   // Keep the LEDs on for the tabata time
      if ( rapid_timer == 0 )            // Don't do anything unless the time expires
      {
        timer_new(&rapid_timer, json_rapid_time * ONE_SECOND);
        SEND(sprintf(_xs, "{\"RAPID_ON\":%d}\r\n", (int)json_rapid_time);)
        set_LED_PWM_now(json_LED_PWM);   // Turn on the LEDs
        set_status_LED(LED_RAPID_GREEN);
        rapid_state = RAPID_ON;

      }
      break;

    case (RAPID_ON):                    // Keep the LEDs on for the tabata time
      if ( rapid_timer == 0 )           // Don't do anything unless the time expires
      {
        SEND(sprintf(_xs, "{\"RAPID_OFF\":0}\r\n");)
        set_LED_PWM_now(0);             // Turn off the LEDs
        rapid_state = RAPID_SEND;       // Ran out of time, start sending
        set_status_LED(LED_RAPID_GREEN_OFF);
        set_status_LED(LED_RAPID_RED);
      }
      else
      {
        if (new_shot != shot_in)        // Has a shot arrived?
        {
          stop_timers();                // Reset thet timers
          arm_timers();                 // and start over
          DLT(DLT_DEBUG, SEND(sprintf(_xs, "Bang!");))
          new_shot = shot_in;
          rapid_count++;
          if ( rapid_count == json_rapid_count ) // Ran out of shots,
          {
            rapid_timer = 0;            // shut down and 
            rapid_state = RAPID_SEND;   // send
            SEND(sprintf(_xs, "{\"RAPID_OFF\":0}\r\n");)
            set_LED_PWM_now(0);             // Turn off the LEDs
            set_status_LED(LED_RAPID_RED);
          }
        }
      }
      break;

    case (RAPID_SEND):                  // Compute the score and send it
      break;
    }

 /* 
  * All done.  
  */
    return;
 }

 /*----------------------------------------------------------------
 * 
 * @function: bye
 * 
 * @brief:    Go into power saver
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This function allows the user to remotly shut down the unit
 * when not in use.
 * 
 * This is called every second from the synchronous scheduler
 * 
 *--------------------------------------------------------------*/
static enum bye_state {
  BYE_BYE = 0,      // Wait for the timer to run out
  BYE_HOLD,         // Wait for the MFS to be pressed
  BYE_START         // Go back into service
};


void bye
(
  unsigned int force_bye    // Set to true to force a shutdown
)         
{
  static int bye_state = BYE_BYE;

/*
 * The BYE function does not work if we are a token ring.
 */
  if ( force_bye == 0 )                 // Regular 
  {
    if ( (json_token != TOKEN_NONE)     // Skip if token ring enabled
      || (json_power_save == 0)         // Power down has not been enabled
      || (power_save != 0) )            // Power down has not run out
    {
      bye_state = BYE_BYE;
      return;
    }
  }

  switch (bye_state)
  {
    case BYE_BYE:                     // Say Good Night Gracie!
      SEND(sprintf(_xs, "{\"GOOD_BYE\":0}");)
      json_tabata_enable = false;     // Turn off any automatic cycles 
      json_rapid_enable = false;
      set_LED_PWM(0);                 // Going to sleep 
      set_status_LED(LED_BYE);
      serial_flush(ALL);              // Purge the com port
      run_state &= ~IN_OPERATION;     // Take the system out of operating mode
      run_state |= IN_SLEEP;          // Put it to sleep 
      bye_state = BYE_HOLD;
      break;

    case BYE_HOLD:                    // Loop waiting for something to happen
      if ( (DIP_SW_A )                // Wait for the switch to be pressed
        || (DIP_SW_B )                // Or the switch to be pressed
        || ( serial_available(ALL) != 0)// Or a character to arrive
        || ( is_running() != 0) )     // Or a shot arrives
      {
        bye_state = BYE_START;        // wait for the swich to be released
      }                               // turns up
      break;

    case BYE_START:
      if (  !(DIP_SW_A)               // Wait here for both switches to be released
        && !(DIP_SW_B) )
      {
        hello();
        bye_state = BYE_BYE;
      }
      break;
  }

/*
 * Loop for the next time
 */  
  return;
}

/*----------------------------------------------------------------
 * 
 * @function: hello
 * 
 * @brief:    Come out of power saver
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This function Puts the system back in service
 * 
 *--------------------------------------------------------------*/
void hello(void)
{
/*
 * Woken up again.  Turn things back on
 */  
  SEND(sprintf(_xs, "{\"Hello_World\":0}");)
  set_status_LED(LED_READY);
  set_LED_PWM_now(json_LED_PWM);
  timer_new(&power_save, json_power_save * (unsigned long)ONE_SECOND * 60L);
  run_state &= ~IN_SLEEP;       // Out of sleep and back in operation
  run_state |= IN_OPERATION;
  return;
}

/*----------------------------------------------------------------
 * 
 * @function: send_keep_alive
 * 
 * @brief:    Send a keep alive over the TCPIP
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This is called every second to send out the keep alive to the 
 * TCPIP server
 * 
 *--------------------------------------------------------------*/
void send_keep_alive(void)
{
  static int keep_alive_count = 0;
  static int keep_alive = 0;

  if ( (json_keep_alive != 0)
      && (keep_alive == 0) )              // Time in seconds
  {
    sprintf(_xs, "{\"KEEP_ALIVE\":%d}", keep_alive_count++);
    serial_to_all(_xs, TCPIP);
    timer_new(&keep_alive, (unsigned long)json_keep_alive * ONE_SECOND);
  }

  return;
}


/*----------------------------------------------------------------
 * 
 * @function: polled_target_test
 * 
 * @brief:    Abbreviated state machine to test the target aquisition
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This arms the target and waits for a shot to be fired.  Once 
 * the shot has been received, it is displayed on the console for
 * analysis.
 * 
 * This function polls the sensors to make sure that the 
 * 
 *--------------------------------------------------------------*/
 void polled_target_test(void)
 {

  int i;
  int running;                          // Copy of the is_running state

  SEND(sprintf(_xs, "\r\nPolled target shot test\r\n");)
  freeETarget_timer_pause();             // Kill the background timer interrupt

/*
 * Stay here watching the counters
 */
  while (1)
  {
    arm_timers();
    SEND(sprintf(_xs, "\r\nArmed\r\n");)
    while(is_running() != 0xff)
    {
      running = is_running();
      SEND(sprintf(_xs, "\r\nis_running: %02X", running);)
      
      for (i=0; i != 8; i++)
      {
        if (running & (1<<i))
        {
          SEND(sprintf(_xs, " %s ", find_sensor(1<<i)->long_name );)
        }
      }
    }
    stop_timers();
    vTaskDelay(10);
  }

/*
 * Nothing more to do
 */
  set_VREF();
  freeETarget_timer_start();        // Turn on the timers again
  return;
 }

 /*----------------------------------------------------------------
 * 
 * @function: interrupt_target_test
 * 
 * @brief:    Abbreviated state machine to test the target aquisition
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 * This arms the target and waits for a shot to be fired.  Once 
 * the shot has been received, it is displayed on the console for
 * analysis.
 * 
 * This function polls the sensors to make sure that the 
 * 
 *--------------------------------------------------------------*/
extern int isr_state;
 void interrupt_target_test(void)
 {

  int i;

  SEND(sprintf(_xs, "\r\nInterrupt target shot test: this: %d last %d\r\n", shot_in, shot_out);)

/*
 * Stay here watching the counters
 */
  while (1)
  {
    while ( shot_in != shot_out )    // While we have a queue o shots
    {
      SEND(sprintf(_xs, "\r\n");)
      for (i=0; i != 8; i++)
      {
        SEND(sprintf(_xs, "%s:%5d  ", find_sensor(1<<i)->long_name, record[shot_out].timer_count[i]);)
      }
      shot_out = (shot_out+1) % SHOT_SPACE;
    }
    vTaskDelay(1);
  }

/*
 * Nothing more to do
 */
  return;
 }

/*----------------------------------------------------------------
 * 
 * @function: diag_LED
 * 
 * @brief:    Return the diagnostics LED belonging to the running bit
 * 
 * @return:   diag_LED
 * 
 *----------------------------------------------------------------
 *
 * Information about a sensor is saved in the structure s[].
 * 
 * This function inspects the structure looking for the bit
 * corresponding to the bit set in the run latch
 * 
 *--------------------------------------------------------------*/
sensor_ID_t* find_sensor
(
  unsigned int run_mask       // Run mask to look for a match
)
{
  unsigned int i;

/*
 *  Loop throught the sensors looking for a matching run mask
 */
  for (i=N; i <= W; i++ )
  {
    if ( (run_mask & s[i].low_sense.run_mask) != 0 )
    {
      return &s[i].low_sense;
    }

    if ( (run_mask & s[i].high_sense.run_mask) != 0  )
    {
      return &s[i].high_sense;
    }
  }

/*
 * Not found, return null
 */
  return LED_READY;
}
