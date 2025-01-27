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
#include "assert.h"

#include "freETarget.h"
#include "gpio.h"
#include "gpio_define.h"
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
 *  Variables
 */
shot_record_t record[SHOT_SPACE];                   // Array of shot records
unsigned int  shot_in;                              // Index to the shot just received
unsigned int  shot_out;                             // Index to the shot just sent to the PC (shot_out <= shot_in)

double       s_of_sound;                            // Speed of sound
unsigned int shot        = 0;                       // Shot counter
unsigned int face_strike = 0;                       // Miss Face Strike interrupt count
unsigned int is_trace    = DLT_INFO | DLT_CRITICAL; // Default tracing

unsigned int  shot_number;                          // Shot Identifier (1-100)
unsigned long shot_start;                           // Time when target was ready for shots

static volatile unsigned long keep_alive;           // Keep alive timer
static volatile unsigned long tabata_timer;         // Free running state timer
volatile unsigned long        power_save;           // Power save timer
static volatile unsigned long rapid_timer;          // Timer used for rapid fire ecents
volatile unsigned long        LED_timer;            // Timer to reset LED status
unsigned long                 go_dark     = 10l;    // Go dark for 10 seconds
unsigned long                 go_wait     = 3l;     // Wait for the PC to catchup
unsigned long                 all_done    = 0l;     // All finished
int                           always_true = true;

static enum {
  START = 0,                                        // 0 et the operating mode
  WAIT,                                             // 1 ARM the circuit and wait for a shot
  REDUCE                                            // 2 Reduce the data and send the score
} state;

static enum bye_state {
  BYE_BYE = 0,                                      // Wait for the timer to run out
  BYE_HOLD,                                         // Wait for the MFS to be pressed
  BYE_START                                         // Go back into service
};

typedef struct
{
  int                    *enable;                   // Signal used to initate a state transition
  volatile unsigned long *timer;                    // Timer used to control state length
  char                   *status_LED;               // Status LED output
  int                     LED_bright;               // Brightness of target LED
  char                   *message;                  // Message to be sent to PC
  bool                    in_shot;                  // In as shot cycle
} rapid_state_t;

extern int isr_state;

volatile unsigned int run_state = 0;                // Current operating state

const char *names[] = {"TARGET",                    //  0
                       "1",      "2",      "3",       "4",      "5",       "6",       "7",      "8",     "9",      "10", //  1
                       "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                          // 11
                       "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET",  "CUPID", "DUNDER",       // 18
                       "ODIN",   "WODEN",  "THOR",    "BALDAR",                                                          // 26
                       0};

const char to_hex[] = "0123456789ABCDEF"; // Quick Hex to ASCII

char _xs[LONG_TEXT];                      // Holding buffer for sprintf

/*
 *  Function Prototypes
 */
static unsigned int set_mode(void); // Set the target running mode
static unsigned int arm(void);      // Arm the circuit for a shot
static unsigned int wait(void);     // Wait for the shot to arrive
static unsigned int reduce(void);   // Reduce the shot data
extern void         gpio_init(void);
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
  json_aux_port_enable = false;    // Assume the AUX port is not used
  gpio_init();                     // Setup the hardware
  serial_io_init();                // Setup the console for debug messages
  read_nonvol();                   // Read in the settings
  serial_aux_init();               // Update the serial port if there is a change
  POST_version();                  // Show the version string on all ports
  set_VREF();
  multifunction_init();            // Override the MFS if we have to

  set_status_LED(LED_RAPID_RED_OFF);
  set_status_LED(LED_RAPID_GREEN_OFF);
  set_status_LED(LED_HELLO_WORLD); // Hello World
  set_status_LED(LED_RAPID_RED);   // Red
  timer_delay(ONE_SECOND);
  set_status_LED(LED_RAPID_OFF);
  set_status_LED(LED_RAPID_GREEN); // Green
  timer_delay(ONE_SECOND);
  set_status_LED(LED_OFF);
  set_status_LED(LED_RAPID_OFF);   // Off

  WiFi_init();

  /*
   *  Set up the long running timers
   */
  timer_new(&keep_alive, (unsigned long)json_keep_alive * ONE_SECOND * 60l);         // Keep alive timer
  timer_new(&power_save, (unsigned long)(json_power_save) * (long)ONE_SECOND * 60L); // Power save timer

  /*
   * Run the power on self test
   */
  POST_counters();            // POST counters does not return if there is an error
  if ( check_12V() == false ) // Verify the 12 volt supply
  {
    DLT(DLT_INFO, SEND(sprintf(_xs, "12V supply not present");))
  }

  /*
   * Ready to go
   */
  show_echo();
  set_LED_PWM(json_LED_PWM);
  serial_flush(ALL); // Get rid of everything
  shot_in  = 0;      // Clear out any junk
  shot_out = 0;
  DLT(DLT_INFO, SEND(sprintf(_xs, "Initialization complete");))

  /*
   * Start the tasks running
   */
  run_state &= ~IN_STARTUP; // Exit startup
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

unsigned int sensor_status; // Record which sensors contain valid data
unsigned int location;      // Sensor location

void freeETarget_target_loop(void *arg)
{
  DLT(DLT_INFO, SEND(sprintf(_xs, "freeETarget_target_loop()");))

  set_status_LED(LED_READY);

  if ( json_pcnt_latency != 0 )              // If the second set of timers has been enabled
  {
    DLT(DLT_INFO, SEND(sprintf(_xs, "Initializing PCNT high inputs");))
    gpio_init_single(PCNT_HI);               // Program the port
  }

  shot_number = 1;                           // Start counting shots at 1

  while ( 1 )
  {
    IF_IN(IN_SLEEP | IN_TEST | IN_FATAL_ERR) // If Not in operation,
    {
      run_state &= ~IN_OPERATION;            // Exit operation
      set_status_LED(LED_FATAL);             // but show something really wrong
      vTaskDelay(ONE_SECOND);
      continue;
    }

    run_state |= IN_OPERATION;               // In operation

    /*
     * Cycle through the state machine
     */
    switch ( state )
    {
      default:
      case START:                                                                      // Start of the loop
        DLT(DLT_APPLICATION, SEND(sprintf(_xs, "state: START");))
        power_save = (unsigned long)json_power_save * (unsigned long)ONE_SECOND * 60L; //  Reset the timer
        set_mode();
        arm();
        set_status_LED(LED_READY);
        state            = WAIT;
        json_rapid_count = 0;
        DLT(DLT_APPLICATION, SEND(sprintf(_xs, "state: WAIT");))
        break;

      case WAIT:
        state = wait();
        break;

      case REDUCE:
        DLT(DLT_APPLICATION, SEND(sprintf(_xs, "state: REDUCE");))
        reduce();
        state = START;
        break;
    }
    /*
     * End of the loop. timeout till the next time
     */
    vTaskDelay(TICK_10ms);
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

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    record[i].face_strike = 100; // Disable the shot record
  }

  if ( json_tabata_enable )      // If the Tabata or rapid fire is enabled,
  {
    set_LED_PWM_now(0);          // Turn off the LEDs
  } // Until the session starts
  else
  {
    set_LED_PWM(json_LED_PWM); // Keep the LEDs ON
  }

  /*
   * Proceed to the WAITing state
   */
  return WAIT; // Carry on to the target
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

  face_strike = 0;                   // Reset the face strike count
  stop_timers();
  arm_timers();                      // Arm the counters
  run_state |= IN_SHOT;
  shot_start = esp_timer_get_time(); // Remember when we started

  sensor_status = is_running();      // and immediatly read the status
  if ( sensor_status == 0 )          // After arming, the sensor status should be zero
  {
    return WAIT;                     // Fall through to WAIT
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
  static unsigned int paper_shot = 0; // Count of reduced shots
  float               radius;

  run_state |= IN_REDUCTION;
  /*
   * Loop and process the shots.  Possibly more than one shot
   */
  while ( shot_out != shot_in ) // Process the shots on the queue
  {
    DLT(DLT_DEBUG, SEND(sprintf(_xs, "shot_in: %d,  shot_out:%d", shot_in, shot_out);))
    DLT(DLT_DEBUG, show_sensor_status(record[shot_out].sensor_status);)

    show_sensor_fault(record[shot_out].sensor_status);

    location = compute_hit(&record[shot_out]); // Compute the score

    /*
     *  Delay for a follow through
     */
    if ( location != MISS ) // Was it a miss or face strike?
    {
      send_score(&record[shot_out], shot_out, NOT_MISSED_SHOT);

      /*
       *  Advance the paper
       */
      if ( IS_DC_WITNESS || IS_STEPPER_WITNESS )                           // Has the witness paper been enabled?
      {
        radius = sqrt(sq(record[shot_out].xs) + sq(record[shot_out].ys));
        if ( ((json_paper_eco == 0)                                        // PAPER_ECO turned off
              || radius < (json_paper_eco / 2)) )                          // Inside the black (radius)
        {
          paper_shot++;
          DLT(DLT_DEBUG, SEND(sprintf(_xs, "Radius: %4.2f/%d good shot: %d/%d", radius, json_paper_eco / 2, paper_shot, json_paper_shot);))
          if ( (json_paper_shot == 0) || (paper_shot >= json_paper_shot) ) // Or we have reached the required number opf hits?
          {
            paper_start();                                                 // Roll the paper
            paper_shot = 0;                                                // And start over
          }
        }
        else
        {
          DLT(DLT_DEBUG, SEND(sprintf(_xs, "Radius: %4.2f/%d bad shot: %d/%d", radius, json_paper_eco / 2, paper_shot, json_paper_shot);))
        }
      }
    }
    else                                                    // We have a miss
    {
      DLT(DLT_INFO, show_sensor_status(record[shot_out].sensor_status);)
      set_status_LED(LED_MISS);
      send_score(&record[shot_out], shot_out, MISSED_SHOT); // Show a miss
    }
    shot_out = (shot_out + 1) % SHOT_SPACE;                 // Increment to the next shot
  }

  /*
   * All done, Exit to FINISH if the timer has expired
   */
  while ( ring_timer != 0 ) // Wait here to make sure the ringing has stopped
  {
    DLT(DLT_DEBUG, SEND(sprintf(_xs, "ring_timer: %ld", ring_timer);))
    vTaskDelay(10);
  }

                            /*
                             * Finished reduction and going back into the shot
                             */
  run_state &= ~IN_REDUCTION;

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
 * This function deliberatly forces an assert that reboots the
 * processor from the beginning.
 *
 * if connected by WiFi, the TCPIP connection must be
 * disconnected and connected again.
 *
 *--------------------------------------------------------------*/
void start_new_session(void)
{
  unsigned char ch;

  SEND(sprintf(_xs, "\r\nThis will reset the board\r\n");)

  if ( prompt_for_confirm() == true )
  {
    SEND(sprintf(_xs, "\n\rResetting board\r\n");)
    assert(0);
  }

  SEND(sprintf(_xs, "\r\nRestart cancelled\r\n");)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: tabata_task
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
 * Tabata differs from Rapid Fire in that Tabata will run
 * indefinitly
 *
 * Test JSON
 * {"TABATA_WARN_ON": 1, "TABATA_WARN_OFF":5, "TABATA_ON":7, "TABATA_REST":30, "TABATA_ENABLE":1}
 * {"TABATA_WARN_ON": 5, "TABATA_WARN_OFF":2, "TABATA_ON":7, "TABATA_REST":30, "TABATA_ENABLE":1}
 * {"MFS_HOLD_C":18, "MFS_HOLD_D":20, "MFS_SELECT_CD":22}
 *
 *-------------------------------------------------------------*/
const rapid_state_t tabata_state[] = {
    //  State transition     Time in state     Status LEDs      target LED  Message       IN_SHOT
    {&json_tabata_enable, &json_tabata_warn_on,  LED_RAPID_GREEN_WARN, 0, "TABATA_WARN",     false}, // Wait for json_tabata_enable
    {&always_true,        &json_tabata_warn_off, LED_RAPID_OFF,        0, "TABATA_WARN_OFF", false}, // Turn the timer on for the event
    {&always_true,        &json_tabata_on,       LED_RAPID_GREEN,      1, "TABATA_ON",       true }, // Wait for json_tabata_enable
    {&always_true,        &json_tabata_rest,     LED_RAPID_RED,        0, "TABATA_REST",     false}, // Turn the timer on for the event
    {&always_true,        &all_done,             LED_RAPID_OFF,        0, "TABATA_START",    false}  // End of state machine
};

void tabata_task(void)
{
  static unsigned int state_machine = 0;                                                        // State machine index

  IF_NOT(IN_OPERATION) return;

  /*
   * Exit if Rapid fire has not been enabled
   */
  if ( json_tabata_enable == false ) // Reset the state machine
  {
    state_machine = 0;
    return;
  }

  /*
   *  Execute the state machine
   */
  if ( *tabata_state[state_machine].enable != 0 )                             // Is the transistion to the next state available?
  {
    if ( tabata_timer == 0 )                                                  // Time to go to the next state?
    {
      state_machine++;                                                        // Next state
      SEND(sprintf(_xs, "{\"%s\": %ld}", tabata_state[state_machine].message, *tabata_state[state_machine].timer);)

      if ( *tabata_state[state_machine].timer == 0 )                          // Reached the end of the state machine
      {
        state_machine = 0;                                                    // Go back to the beginning
      }
      timer_new(&tabata_timer, (*tabata_state[state_machine].timer) * ONE_SECOND);
      set_status_LED(tabata_state[state_machine].status_LED);
      set_LED_PWM_now(tabata_state[state_machine].LED_bright * json_LED_PWM); // Turn off the lights // TRUE if the lights are on

      if ( tabata_state[state_machine].in_shot == true )                      // Is the target ready to accept a shot?
      {
        run_state |= IN_SHOT;                                                 // Yes, set it so
        shot_start = esp_timer_get_time();                                    // and remember this time
      }
      else
      {
        run_state &= ~IN_SHOT;
      }
    }
  }

  /*
   * All done.
   */
  return;
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
 * Test Message
 * {"RAPID_TIME":20, "RAPID_COUNT":10, "RAPID_WAIT":5, "RAPID_ENABLE":1}
 * {"RAPID_TIME":10, "RAPID_COUNT":10, "RAPID_WAIT":0,  "RAPID_ENABLE":1}
 * {"RAPID_TIME":20, "RAPID_COUNT":5,  "RAPID_WAIT":0,  "RAPID_ENABLE":1}
 * {"RAPID_TIME":30, "RAPID_COUNT":5,  "RAPID_WAIT":5,  "RAPID_ENABLE":1}
 * {"MFS_HOLD_C":18, "MFS_HOLD_D":20, "MFS_SELECT_CD":22, "RAPID_TIME":10, "RAPID_COUNT":5,  "RAPID_WAIT":5,  "RAPID_ENABLE":1}
 *
 *--------------------------------------------------------------*/                                       // 100 signals random time, %10 is the duration

const rapid_state_t rapid_state[] = {
    //  State transition     Time in state     Status LEDs    target LED  Message IN_SHOT
    {&json_rapid_enable, &go_wait,         LED_RAPID_OFF,        0, "RAPID_ENABLED", false}, // Wait for json_rapid_enable
    {&always_true,       &go_wait,         LED_RAPID_GREEN_WARN, 0, "RAPID_WAIT",    false}, // Warn the shooter the event is enabled
    {&always_true,       &go_wait,         LED_RAPID_OFF,        0, "RAPID_DARK",    false}, // Warn the shooter the event is enabled
    {&always_true,       &json_rapid_time, LED_RAPID_GREEN,      1, "RAPID_ON",      true }, // Turn the timer on for the event
    {&always_true,       &go_dark,         LED_RAPID_RED,        0, "RAPID_OFF",     false}, // Event finished, turn off
    {&always_true,       &all_done,        LED_RAPID_OFF,        1, "SLOW_FIRE",     true }  // End of state machine
};

void rapid_fire_task(void)
{
  static unsigned int state_machine = 0;                                      // State machine index
  static unsigned int rapid_count;                                            //  Number of shots received during rapid fire

  IF_NOT(IN_OPERATION) return;

  /*
   * Exit if Rapid fire has not been enabled
   */
  if ( json_rapid_enable == false ) // Are we disabled?
  {
    state_machine = 0;              // Reset the state machine
    rapid_timer   = 0;              // Reset the timer just in case
    return;
  }

  /*
   *  Execute the state machine
   */
  if ( *rapid_state[state_machine].enable != 0 )                               // Is the transistion to the next state available?
  {
    if ( rapid_timer == 0 )                                                    // Time to go to the next state?
    {
      state_machine++;                                                         // Next state
      SEND(sprintf(_xs, "{\"%s\": %ld}", rapid_state[state_machine].message, *rapid_state[state_machine].timer);)

      if ( *rapid_state[state_machine].timer == 0 )                            // Reached the end of the state machine
      {
        set_status_LED(LED_RAPID_OFF);                                         // Turn off the Lights
        set_LED_PWM_now(json_LED_PWM);                                         // Turn off the lights

        while ( rapid_count < json_rapid_count )                               // Send incomplete as misses
        {
          record[rapid_count].shot_time = 0;                                   // Fake the time
          send_score(&record[rapid_count], rapid_count, MISSED_SHOT);          // and show as a miss
          rapid_count = (rapid_count + 1) % SHOT_SPACE;
        }

        state_machine     = 0;                                                 // Go back to the beginning
        rapid_timer       = 0;                                                 // Reset the timer
        json_rapid_enable = 0;                                                 // Turn off the rapid cycle
        json_rapid_count  = 0;                                                 // No more shots expected
      }
      else
      {
        timer_new(&rapid_timer, (*rapid_state[state_machine].timer) * ONE_SECOND);
        set_status_LED(rapid_state[state_machine].status_LED);
        set_LED_PWM_now(rapid_state[state_machine].LED_bright * json_LED_PWM); // Control the lights

        if ( rapid_state[state_machine].in_shot == true )                      // Remember the shot count when we go into
        {                                                                      // rapid fire so that we can detect misses
          rapid_count = shot_in;
          run_state |= IN_SHOT;                                                // See if we are expecting a shot
          shot_start = esp_timer_get_time();                                   // and remember this time
        }
        else
        {
          run_state &= ~IN_SHOT;
        }
      }
    }
  }

  /*
   * Set the IN_SHO if the target is ready
   */

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

void bye(unsigned int force_bye // Set to true to force a shutdown
)
{
  static int bye_state = BYE_BYE;

  /*
   * The BYE function does not work if we are a token ring.
   */
  if ( force_bye == 0 )             // Regular
  {
    if ( (json_token != TOKEN_NONE) // Skip if token ring enabled
         || (json_power_save == 0)  // Power down has not been enabled
         || (power_save != 0) )     // Power down has not run out
    {
      bye_state = BYE_BYE;
      return;
    }
  }

  switch ( bye_state )
  {
    case BYE_BYE:                          // Say Good Night Gracie!
      SEND(sprintf(_xs, "{\"GOOD_BYE\":0}");)
      json_tabata_enable = false;          // Turn off any automatic cycles
      json_rapid_enable  = false;
      set_LED_PWM(0);                      // Going to sleep
      set_status_LED(LED_BYE);
      serial_flush(ALL);                   // Purge the com port
      run_state &= ~IN_OPERATION;          // Take the system out of operating mode
      run_state |= IN_SLEEP;               // Put it to sleep
      bye_state = BYE_HOLD;
      break;

    case BYE_HOLD:                         // Loop waiting for something to happen
      if ( (DIP_SW_A)                      // Wait for the switch to be pressed
           || (DIP_SW_B)                   // Or the switch to be pressed
           || (serial_available(ALL) != 0) // Or a character to arrive
           || (is_running() != 0) )        // Or a shot arrives
      {
        bye_state = BYE_START;             // wait for the swich to be released
      } // turns up
      break;

    case BYE_START:
      if ( !(DIP_SW_A) // Wait here for both switches to be released
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
  run_state &= ~IN_SLEEP; // Out of sleep and back in operation
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
  static int keep_alive       = 0;

  if ( (json_keep_alive != 0) && (keep_alive == 0) ) // Time in seconds
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
  int running;               // Copy of the is_running state

  freeETarget_timer_pause(); // Kill the background timer interrupt

  /*
   * Stay here watching the counters
   */
  while ( 1 )
  {
    arm_timers();
    SEND(sprintf(_xs, "\r\nArmed\r\n");)
    while ( (is_running() & RUN_MASK) != RUN_MASK )
    {
      running = is_running();
      SEND(sprintf(_xs, "\r\nis_running: %02X", running);)

      for ( i = 0; i != 8; i++ )
      {
        if ( running & (1 << i) )
        {
          SEND(sprintf(_xs, " %s ", find_sensor(1 << i)->long_name);)
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
  freeETarget_timer_start(); // Turn on the timers again
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
void interrupt_target_test(void)
{

  int i;

  SEND(sprintf(_xs, "\r\nInterrupt target shot test: shot_in: %d   shot_out: %d\r\n", shot_in, shot_out);)

  /*
   * Stay here watching the counters
   */
  while ( 1 )
  {
    while ( shot_in != shot_out ) // While we have a queue o shots
    {
      SEND(sprintf(_xs, "\r\n");)
      for ( i = 0; i != 8; i++ )
      {
        SEND(sprintf(_xs, "%s:%5d  ", find_sensor(1 << i)->long_name, record[shot_out].timer_count[i]);)
      }
      shot_out = (shot_out + 1) % SHOT_SPACE;
    }
    vTaskDelay(TICK_10ms);
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
sensor_ID_t *find_sensor(unsigned int run_mask // Run mask to look for a match
)
{
  unsigned int i;

  /*
   *  Loop throught the sensors looking for a matching run mask
   */
  for ( i = N; i <= W; i++ )
  {
    if ( (run_mask & s[i].low_sense.run_mask) != 0 )
    {
      return &s[i].low_sense;
    }

    if ( (run_mask & s[i].high_sense.run_mask) != 0 )
    {
      return &s[i].high_sense;
    }
  }

  /*
   * Not found, return null
   */
  return LED_READY;
}

/*----------------------------------------------------------------
 *
 * @function: prompt_for_confirm
 *
 * @brief:    Display a propt and wait for a return
 *
 * @return:   TRUE if the confirmation is Yes
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/

bool prompt_for_confirm(void)
{
  unsigned char ch;

  SEND(sprintf(_xs, "\r\nConfirm Y/N?");)

  /*
   * Loop and wait for a confirmation
   */
  while ( 1 )
  {
    if ( serial_available(ALL) != 0 )
    {
      ch = serial_getch(ALL);
      switch ( ch )
      {
        case 'y':
        case 'Y':
          return true;

        case 'n':
        case 'N':
          return false;

        default:
          break;
      }
      vTaskDelay(ONE_SECOND / 10);
    }
  }
}
