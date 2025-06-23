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
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include <esp_wifi.h>

#define FREETARGET_C
#include "freETarget.h"
#include "helpers.h"
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
#include "http_client.h"
#include "http_services.h"
#include "OTA.h"

/*
 *  Variables
 */
unsigned int number_of_connections = 0; // How many people are connected to me?

                                        // Keep alive timer
time_count_t tabata_timer;            // Free running state timer
time_count_t rapid_timer;             // Timer used for rapid fire ecents
                                      // Timer to reset LED status
unsigned long go_dark     = 10l;      // Go dark for 10 seconds
unsigned long go_wait     = 3l;       // Wait for the PC to catchup
unsigned long all_done    = 0l;       // All finished
int           always_true = true;

static enum {
  START = 0,                          // 0 et the operating mode
  WAIT,                               // 1 ARM the circuit and wait for a shot
  REDUCE                              // 2 Reduce the data and send the score
} freETarget_state;

typedef struct
{
  volatile unsigned long *timer;      // Timer used to control state length
  char                   *status_LED; // Status LED output
  float                   LED_bright; // Brightness of target LED
  char                   *message;    // Message to be sent to PC
  bool                    in_shot;    // In as shot cycle
} rapid_state_t;

extern int isr_state;

volatile unsigned int run_state = 0;  // Current operating state

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
  is_trace  = DLT_INFO | DLT_CRITICAL;
#ifdef TRACE_APPLICATION
  is_trace |= DLT_APPLICATION;   // Enable application tracing
#endif
#ifdef TRACE_COMMUNICATION
  is_trace |= DLT_COMMUNICATION; // Enable application tracing
#endif
#ifdef TRACE_DIAGNOSTICS
  is_trace |= DLT_DIAG;          // Enable diagnostics tracing
#endif
#ifdef TRACE_DEBUG
  is_trace |= DLT_DEBUG;         // Enable debug tracing
#endif
#ifdef TRACE_SCORE
  is_trace |= DLT_SCORE;         // Enable score tracing
#endif
#ifdef TRACE_HTTP
  is_trace |= DLT_HTTP;          // Enable HTTP tracing
#endif
#ifdef TRACE_OTA
  is_trace |= DLT_OTA;           // Enable OTA tracing
#endif
#ifdef TRACE_HEARTBEAT
  is_trace |= DLT_HEARTBEAT;     // Enable heartbeat tracing
#endif

  /*
   *  Setup the hardware
   */
  json_aux_mode = false; // Assume the AUX port is not used
  gpio_init();           // Setup the hardware
  serial_io_init();      // Setup the console for debug messages
  read_nonvol();         // Read in the settings
  serial_aux_init();     // Update the serial port if there is a change
  set_VREF();
  multifunction_init();  // Override the MFS if we have to

  /*
   * Put up a self test
   */
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
  ft_timer_new(&keep_alive, (unsigned long)json_keep_alive * ONE_SECOND * 60l);         // Keep alive timer
  ft_timer_new(&power_save, (unsigned long)(json_power_save) * (long)ONE_SECOND * 60L); // Power save timer
  ft_timer_new(&time_since_last_shot, HTTP_CLOSE_TIME * 60 * ONE_SECOND);               // 15 minutes since last shot

  /*
   * Run the power on self test
   */
  POST_counters();            // POST counters does not return if there is an error
  if ( check_12V() == false ) // Verify the 12 volt supply
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "12V supply not present");))
  }

  /*
   * Ready to go
   */
  show_echo();
  set_LED_PWM(json_LED_PWM);
  serial_flush(ALL);              // Get rid of everything
  shot_in         = 0;            // Clear out any junk
  shot_out        = 0;
  connection_list = 0;            // Nobody is connected yet
  reset_run_time();               // Reset the time of day
  time_to_go = 1000 * ONE_SECOND; // Infinite amount of time to start

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initialization complete");))

  if ( DIP_SW_A )                 // Switch A pressed
  {
    OTA_load();                   // Load in a new OTA
  }

  if ( DIP_SW_B )                 // Switch B pressed
  {
    OTA_rollback();               // Roll back to old software
  }

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
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "freeETarget_target_loop()");))

  set_status_LED(LED_READY);

  if ( json_pcnt_latency != 0 )              // If the second set of timers has been enabled
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Initializing PCNT high inputs");))
    gpio_init_single(PCNT_HI);               // Program the port
  }

  start_new_session(0);
  shot_number = 1;                           // Start counting shots at 1

  while ( 1 )
  {
    IF_IN(IN_SLEEP | IN_TEST | IN_FATAL_ERR) // If Not in operation,
    {
      run_state &= ~IN_OPERATION;            // Exit operation
      IF_IN(IN_FATAL_ERR)                    // Have we deteted a fatal error?
      {
        set_status_LED(LED_FATAL);           // but show something really wrong
      }
      vTaskDelay(ONE_SECOND);
      continue;
    }

    run_state |= IN_OPERATION;               // In operation

    /*
     * Cycle through the state machine
     */
    switch ( freETarget_state )
    {
      default:
      case START:                                                                                // Start of the loop
        DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "state: START");))
        power_save           = (unsigned long)json_power_save * (unsigned long)ONE_SECOND * 60L; //  Reset the timer
        time_since_last_shot = HTTP_CLOSE_TIME * 60l * ONE_SECOND;                               // 15 minutes since last shot
        set_mode(); // Set the mode for the next string of shot (ex Tabata or Rapid Fire)
        arm();      // Arm the circuit and check for errors
        set_status_LED(LED_READY);
        if ( (json_rapid_enable == false) && (json_tabata_enable == false) ) // If rapid fire is not enabled
        {
          set_status_LED(LED_RAPID_GREEN);                                   // Show that the target cannot be used
        }
        freETarget_state = WAIT;
        json_rapid_count = 0;
        DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "state: WAIT");))
        break;

      case WAIT:
        freETarget_state = wait();
        break;

      case REDUCE:
        DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "state: REDUCE");))
        reduce();
        freETarget_state = START;
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

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "set_mode()");))

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    record[i].face_strike = 100; // Disable face strikes
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
  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "arm()");))

  face_strike = 0;                                                       // Reset the face strike count
  stop_timers();
  arm_timers();                                                          // Arm the counters
  run_state |= IN_SHOT;
  shot_start = run_time_ms();                                            // Remember when we started

  sensor_status = is_running();                                          // and immediatly read the status
  if ( sensor_status == 0 )                                              // After arming, the sensor status should be zero
  {
    if ( (json_rapid_enable == false) && (json_tabata_enable == false) ) // If rapid fire is not enabled
    {
      set_status_LED(LED_RAPID_GREEN);                                   // Show that we are ready
    }

    return WAIT;                                                         // Fall through to WAIT
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
#define FORCE_PAPER_MOVE 10               // Force a paper move if we get 10 misses

unsigned int reduce(void)
{
  static unsigned int paper_shot     = 0; // Count of reduced shots
  static unsigned int paper_shot_out = 0; // Count of missed shots

  run_state |= IN_REDUCTION;
  /*
   * Loop and process the shots.  Possibly more than one shot
   */
  while ( shot_out != shot_in ) // Process the shots on the queue
  {
    DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "shot_in: %d,  shot_out:%d", shot_in, shot_out);))
    DLT(DLT_DEBUG, show_sensor_status(record[shot_out].sensor_status);)

    show_sensor_fault(record[shot_out].sensor_status);

    location = compute_hit(&record[shot_out]); // Compute the score

    /*
     *  Delay for a follow through
     */
    if ( location != MISS ) // Was it a miss or face strike?
    {
      prepare_score(&record[shot_out], shot_out, NOT_MISSED_SHOT);

      build_json_score(&record[shot_out], SCORE_USB);
      serial_to_all(_xs, CONSOLE);

      build_json_score(&record[shot_out], SCORE_TCPIP);
      serial_to_all(_xs, TCPIP);

      if ( (json_remote_modes & REMOTE_MODE_CLIENT) != 0 )
      {
        build_json_score(&record[shot_out], SCORE_TCPIP);
        http_native_request(json_remote_url, METHOD_POST, _xs, sizeof(_xs));
      }

      /*
       *  Advance the paper
       */
      if ( IS_DC_WITNESS || IS_STEPPER_WITNESS )                                 // Has the witness paper been enabled?
      {
        if ( (json_paper_eco == 0)                                               // PAPER_ECO turned off
             || (record[shot_out].radius < (json_paper_eco / 2))                 // Inside the black (radius)
             || (paper_shot_out > FORCE_PAPER_MOVE) )                            // Too many misses
        {
          paper_shot++;                                                          //
          DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "Radius: %4.2f/%d good shot: %d/%d", record[shot_out].radius, json_paper_eco / 2,
                                           paper_shot, json_paper_shot);))
          if ( (paper_shot >= json_paper_shot)                                   // Have met the number of good shots?
               || (paper_shot_out >= FORCE_PAPER_MOVE) )                         // Or we just shot too many bad ones?
          {
            if ( (json_rapid_enable == false) && (json_tabata_enable == false) ) // If rapid fire is not enabled
            {
              set_status_LED(LED_RAPID_RED);                                     // Show that the target cannot be used
            }
            paper_start();                                                       // Roll the paper
            paper_shot     = 0;                                                  // And start over
            paper_shot_out = 0;                                                  // Reset the outside shots
          }
        }
        else
        {
          paper_shot_out++;                                                      // Outside of the desired radius, keep track of the misses
          DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "Radius: %4.2f/%d bad shot: %d/%d", record[shot_out].radius, json_paper_eco / 2, paper_shot,
                                           json_paper_shot);))
        }
      }
    }
    else                                                                         // We have a miss
    {
      DLT(DLT_INFO, show_sensor_status(record[shot_out].sensor_status);)
      set_status_LED(LED_MISS);
      if ( json_send_miss != 0 )
      {
        prepare_score(&record[shot_out], shot_out, MISSED_SHOT);                 // Show a miss
      }
    }
    shot_out = (shot_out + 1) % SHOT_SPACE;                                      // Increment to the next shot
  }

  /*
   * All done, Exit to FINISH if the timer has expired
   */
  while ( ring_timer != 0 ) // Wait here to make sure the ringing has stopped
  {
    DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "ring_timer: %ld", ring_timer);))
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
 * the command {"SESSION": type}
 *
 * Session types are
 *  0 - Clear all existing sessions
 *  2 - Sighters
 *  4 - Score
 *  10 - Display all shots
 *  12 - Display sighters
 *  14 - Display score
 *
 *--------------------------------------------------------------*/
#define SESSION_PRINT 10                 // Display the session

void start_new_session(int session_type) //
{
  unsigned int i;

  DLT(DLT_APPLICATION, SEND(ALL, sprintf(_xs, "start_new_session(%d)", session_type);))

  switch ( session_type & (~SESSION_VALID) )
  {
    default:
    case SESSION_EMPTY:
      for ( i = 0; i != SHOT_SPACE; i++ )
      {
        record[i].session_type = SESSION_EMPTY;
      }
      shot_in  = 0;
      shot_out = 0;
      reset_run_time();
      time_to_go = 1000;
      break;

    case SESSION_SIGHT:     // Nothing to do
      time_to_go = 15 * 60; // 15 minute sighting timer
      reset_run_time();
      break;

    case SESSION_MATCH:
      time_to_go = 75 * 60;
      reset_run_time();
      break;

    case SESSION_PRINT + SESSION_EMPTY:
    case SESSION_PRINT + SESSION_SIGHT:
    case SESSION_PRINT + SESSION_MATCH:
      for ( i = 0; i != shot_out; i++ )
      {
        if ( ((record[i].session_type & SESSION_VALID) != 0)   // The session has valid data
             && (record[i].session_type % SESSION_PRINT) == (session_type % SESSION_PRINT) )
        {
          build_json_score(&record[i], SCORE_USB);             // Send out to the USB
          serial_to_all(_xs, ALL);

          if ( (json_remote_modes & REMOTE_MODE_CLIENT) != 0 ) // Send out to the server
          {
            build_json_score(&record[i], SCORE_TCPIP);
            http_native_request(json_remote_url, METHOD_POST, _xs, sizeof(_xs));
          }
        }
      }
      break;
  }

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
 * This is a synchronous task that is called every 500ms (1/2 second)
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
 * Test 10.9
 *
 *  {"TABATA_WARN_ON": 3, "TABATA_WARN_OFF":3, "TABATA_ON":12, "TABATA_REST":25, "TABATA_ENABLE":1}
 *  {"TABATA_ENABLE":0}
 *-------------------------------------------------------------*/
const rapid_state_t tabata_state[] = {
    // Time in state           Status LEDs     target LED  Message     IN_SHOT
    {&all_done,             LED_RAPID_OFF,        0,  "TABATA_IDLE",     false}, // 0 Wait for json_tabata_enable
    {&json_tabata_warn_on,  LED_RAPID_GREEN_WARN, -1, "TABATA_WARN",     false}, // 1 Wait for json_tabata_enable
    {&json_tabata_warn_off, LED_RAPID_OFF,        0,  "TABATA_WARN_OFF", false}, // 2 Turn the timer on for the event
    {&json_tabata_on,       LED_RAPID_GREEN,      1,  "TABATA_ON",       true }, // 3 Wait for json_tabata_enable
    {&json_tabata_rest,     LED_RAPID_RED,        0,  "TABATA_REST",     false}, // 4 Turn the timer on for the event
    {&all_done,             LED_RAPID_OFF,        0,  "TABATA_START",    false}  // 5 Start/End of state machine
};

void tabata_task(void)
{
  static int tabata_state_machine = 0;                                      // State machine index
  static int toggle               = 1;                                      // Toggle the lights

  IF_NOT(IN_OPERATION) return;

  /*
   * Exit if Tabata has not been enabled
   */
  if ( json_tabata_enable == false )
  {
    if ( tabata_state_machine != 0 )  // Reset the state machine
    {
      tabata_state_machine = 0;       // Reset the Tabata state machine (incremented on entry)
      run_state &= ~IN_SHOT;          // Take it out of a shot if it was in one
      freETarget_state = START;       // Force the freeTarget state machine back to start
      ft_timer_delete(&tabata_timer); // Delete the unused timer
      set_LED_PWM_now(json_LED_PWM);  // Turn the lights back on
    }
    return;
  }
  /*
   *  Execute the state machine
   */

  if ( tabata_timer == 0 )                                                           // Time to go to the next state?
  {
    tabata_state_machine++;                                                          // Next state
    SEND(ALL, sprintf(_xs, "{\"%s\": %ld}\r\n", tabata_state[tabata_state_machine].message, *tabata_state[tabata_state_machine].timer);)

    if ( *tabata_state[tabata_state_machine].timer == 0 )                            // Reached the end of the state machine
    {
      tabata_state_machine = 1;                                                      // Go back to the beginning
    }
    ft_timer_new(&tabata_timer, (*tabata_state[tabata_state_machine].timer) * ONE_SECOND);
    set_status_LED(tabata_state[tabata_state_machine].status_LED);
    if ( json_LED_PWM >= 0 )
    {
      set_LED_PWM_now(tabata_state[tabata_state_machine].LED_bright * json_LED_PWM); // Turn off the lights // TRUE if the lights are on
    }

    if ( tabata_state[tabata_state_machine].in_shot == true )                        // Is the target ready to accept a shot?
    {
      run_state |= IN_SHOT;                                                          // Yes, set it so
      shot_start = run_time_ms();                                                    // and remember this time
    }
    else
    {
      run_state &= ~IN_SHOT;
    }
  }
  else // Blink the lights (called every 500 ms)
  {
    if ( tabata_state[tabata_state_machine].LED_bright < 0 )
    {
      set_LED_PWM_now(toggle * json_LED_PWM);
      toggle ^= 1;
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
 * This is a synchronous task that is called every 500ms (1/2 second)
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
    // Time in state     Status LEDs    target LED  Message IN_SHOT
    {&all_done,        LED_RAPID_GREEN_WARN, 0,  "RAPID_IDLE",    false}, // 0 Do nothing
    {&go_wait,         LED_RAPID_OFF,        0,  "RAPID_ENABLED", false}, // 1 Wait for json_rapid_enable
    {&json_rapid_wait, LED_RAPID_GREEN_WARN, -1, "RAPID_WAIT",    false}, // 2 Warn the shooter the event is enabled
    {&go_wait,         LED_RAPID_OFF,        0,  "RAPID_DARK",    false}, // 3 Warn the shooter the event is enabled
    {&json_rapid_time, LED_RAPID_GREEN,      0,  "RAPID_ON",      true }, // 4 Turn the timer on for the event
    {&go_dark,         LED_RAPID_RED,        0,  "RAPID_OFF",     false}, // 5 Event finished, turn off
    {&all_done,        LED_RAPID_OFF,        1,  "SLOW_FIRE",     true }  // 6 End of state machine
};

void rapid_fire_task(void)
{
  static unsigned int rapid_state_machine = 0;                         // State machine index
  static unsigned int rapid_count;                                     //  Number of shots received during rapid fire

  IF_NOT(IN_OPERATION) return;

  /*
   * Exit if Rapid fire has not been enabled
   */

  if ( json_rapid_enable == false )
  {
    if ( rapid_state_machine != 0 )  // Reset the state machine
    {
      rapid_state_machine = 0;       // Reset the Tabata state machine (incremented on entry)
      run_state &= ~IN_SHOT;         // Take it out of a shot if it was in one
      freETarget_state = START;      // Force the freeTarget state machine back to start
      ft_timer_delete(&rapid_timer); // Delete the unused timer
      set_LED_PWM_now(json_LED_PWM); // Turn the lights back on
    }
    return;
  }

                                     /*
                                      *  Execute the state machine
                                      */

  if ( rapid_timer == 0 )                                                          // Time to go to the next state?
  {
    rapid_state_machine++;                                                         // Next state
    SEND(ALL, sprintf(_xs, "{\"%s\": %ld}", rapid_state[rapid_state_machine].message, *rapid_state[rapid_state_machine].timer);)

    if ( *rapid_state[rapid_state_machine].timer == 0 )                            // Reached the end of the state machine
    {
      set_status_LED(LED_RAPID_OFF);                                               // Turn off the Lights
      set_LED_PWM_now(json_LED_PWM);                                               // Turn off the lights

      while ( rapid_count < json_rapid_count )                                     // Send incomplete as misses
      {
        record[rapid_count].shot_time = 0;                                         // Fake the time
        prepare_score(&record[rapid_count], rapid_count, MISSED_SHOT);             // and show as a miss
        rapid_count = (rapid_count + 1) % SHOT_SPACE;
      }

      rapid_state_machine = 1;                                                     // Go back to the beginning
      rapid_timer         = 0;                                                     // Reset the timer
      json_rapid_enable   = 0;                                                     // Turn off the rapid cycle
      json_rapid_count    = 0;                                                     // No more shots expected
    }
    else
    {
      ft_timer_new(&rapid_timer, (*rapid_state[rapid_state_machine].timer) * ONE_SECOND);
      set_status_LED(rapid_state[rapid_state_machine].status_LED);
      set_LED_PWM_now(rapid_state[rapid_state_machine].LED_bright * json_LED_PWM); // Control the lights

      if ( rapid_state[rapid_state_machine].in_shot == true )                      // Remember the shot count when we go into
      {                                                                            // rapid fire so that we can detect misses
        rapid_count = shot_in;
        run_state |= IN_SHOT;                                                      // See if we are expecting a shot
        shot_start = run_time_ms();                                                // and remember this time
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
    SEND(ALL, sprintf(_xs, "\r\nArmed\r\n");)
    while ( (is_running() & RUN_MASK) != RUN_MASK )
    {
      running = is_running();
      SEND(ALL, sprintf(_xs, "\r\nis_running: %02X", running);)

      for ( i = 0; i != 8; i++ )
      {
        if ( running & (1 << i) )
        {
          SEND(ALL, sprintf(_xs, " %s ", find_sensor(1 << i)->long_name);)
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

  SEND(ALL, sprintf(_xs, "\r\nInterrupt target shot test: shot_in: %d   shot_out: %d\r\n", shot_in, shot_out);)

  /*
   * Stay here watching the counters
   */
  while ( 1 )
  {
    while ( shot_in != shot_out ) // While we have a queue o shots
    {
      SEND(ALL, sprintf(_xs, "\r\n");)
      for ( i = 0; i != 8; i++ )
      {
        SEND(ALL, sprintf(_xs, "%s:%5d  ", find_sensor(1 << i)->long_name, record[shot_out].timer_count[i]);)
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
 * @function: find_sensor()
 *
 * @brief:    Point to the sensor structure belonging to the run mask
 *
 * @return:   pointer to the sensor structure
 *
 *----------------------------------------------------------------
 *
 * Scan throught the sensor structure looking for the match to
 * the run mask.
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
