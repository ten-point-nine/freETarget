/*----------------------------------------------------------------
 *
 * timed_fire.c
 *
 * Specific timed fire event
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
#include "board_assembly.h"
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
#include "calibrate.h"

/*
 *  Variables
 */

// Keep alive timer
time_count_t tabata_timer; // Free running statsecondse timer
time_count_t event_timer;  // Timer used for rapid fire ecents
time_count_t event_timer;  // Rapid fire or Tabata timer

                           // Timer to reset LED status
real_t go_dark     = 10l;     // Go dark for 10
real_t go_wait     = 3l;      // Wait for the PC to catchup
real_t grace_time  = 30;      // ISSF grace time for in flight shots (30 x 10ms)
real_t all_done    = 0l;      // All finished
int    always_true = 0;       // Force exit condition to be true

typedef struct
{
  real_t *timer;              // Timer used to control state length (from json.c)
  int     time_scale;         // Scaling factor for the timer (e.g. ONE_SECOND or TEN_MILLISECONDS)
  char   *status_LED;         // Status LED output
  int     LED_bright;         // Brightness of target LED
  char   *message;            // Message to be sent to PC
  int     in_shot;            // In a shot cycle
  int    *exit_condition;     // Pointer to the exit condition (0 == exit)
  int     zero;               // Where to go next if the exit condition is zero
  int     not_zero;           // Where to go if the exit condition is non-zero
} rapid_state_t;

typedef struct
{
  char          *event;       // What event are we shooting
  rapid_state_t *rapid_state; // State machine for this event
  int            relay;       // How many shots in the relay
  void (*start_up)(void);     // Function to call to start the relay
} course_of_fire_t;

/*
 * Function Prototypes
 */

/*----------------------------------------------------------------
 *
 * @function: timed_fire
 *
 * @brief:    Start or stop a timed fire session
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This is the function that carries out the timing for a timed
 * fire event.
 *
 * The function waits for json_rapid_enable to start the timed fire event.
 * When that happens, the state machine is initialized and the correct
 * state machine begins sequencing.
 *
 * The state begins by setting the timeduration for that state.  When
 * the time runs out, the state machine transitions to the next state.
 * At that time, any special work that is needed in that state is
 * perfomed and the state machine continues to the next state.
 *
 * When the last state is reached, the timed fire event is concluded.
 *
 * Once this function has been executed, the lights will be turned off
 * in anticipation of the next event.
 *
 * rapid_state_ISSF - Standard rapid fire sequence, ex 5 shots, one target
 * rapid_state_sport - Standard sport fire sequence, ex 1 shot into one target five times
 * tabata_rapid_state - Standard tabata training sequence
 *
 * ------------------------------------------------------------*/

#define LED_DARK 0
#define LED_ON   1
#define LED_RAMP 2

const rapid_state_t rapid_state_ISSF[] = {
    {&all_done,        ONE_SECOND, LED_RAPID_OFF,  LED_DARK, "RAPID_IDLE",    0,       &always_true, 1, 1}, // 0 Do nothing
    {&go_wait,         ONE_SECOND, LED_RAPID_OFF,  LED_DARK, "RAPID_ENABLED", 0,       &always_true, 2, 2}, // 1 Wait for json_rapid_enable
    {&json_rapid_wait, ONE_SECOND, LED_RAPID_WARN, LED_ON,   "RAPID_WAIT",    0,       &always_true, 3, 3}, // 2 Warn the shooter the event is enabled
    {&json_rapid_time, ONE_SECOND, LED_RAPID_ON,   LED_ON,   "RAPID_ON",      IN_SHOT, &always_true, 4, 4}, // 3 Turn the timer on for the event
    {&grace_time,      TICK_10ms,  LED_RAPID_OFF,  LED_DARK, "RAPID_GRACE",   IN_SHOT, &always_true, 5, 5}, // 4 Delay for the grace period
    {&all_done,        ONE_SECOND, LED_RAPID_OFF,  LED_DARK, "ALL_DONE",      0,       &always_true, 0, 0}  // 5 Event finished, turn off
};

static real_t adjusted_rapid_wait;                                                                 // Time interval removing grace period
static int    shot_string;                                                                         // How many shots are we expecting
static int    cycle_count;                                                                   // How many times do I need to repeat this

const rapid_state_t rapid_state_sport[] = {
    {&all_done,            ONE_SECOND, LED_RAPID_OFF,  LED_DARK, "SPORT_IDLE",    0,       &always_true, 1, 1}, // 0 Do nothing
    {&go_wait,             ONE_SECOND, LED_RAPID_OFF,  LED_DARK, "SPORT_ENABLED", 0,       &always_true, 2, 2}, // 1 Wait for json_rapid_enable
    {&json_rapid_wait,     ONE_SECOND, LED_RAPID_WARN, LED_ON,   "SPORT_WAIT_1",  0,       &always_true, 3, 3}, // 2 Warn the shooter
    {&json_rapid_time,     ONE_SECOND, LED_RAPID_ON,   LED_ON,   "SPORT_ON_1",    IN_SHOT, &always_true, 4, 4}, // 3 turn the timer on for the event
    {&grace_time,          TICK_10ms,  LED_RAPID_WARN, LED_ON,   "SPORT_GRACE_1", IN_SHOT, &always_true, 5, 5}, // 4 Delay for the grace period
    {&adjusted_rapid_wait, 1,          LED_RAPID_WARN, LED_ON,   "SPORT_WAIT_2",  0,       &cycle_count, 6, 3}, // 5 Warn the event is enabled
    {&all_done,            ONE_SECOND, LED_RAPID_WARN, 0,        "ALL_DONE",      0,       &always_true, 0, 0}  // 8 Event finished, turn off
};

const rapid_state_t tabata_rapid_state[] = {
    //                                                                                       next    T  F
    {&all_done,        ONE_SECOND, LED_TABATA_OFF,  LED_DARK, "TABATA_IDLE",  0,       &always_true, 1, 1}, // 0 Wait for json_tabata_enable
    {&json_rapid_wait, ONE_SECOND, LED_TABATA_WARN, LED_RAMP, "TABATA_BEGIN", 0,       &always_true, 2, 2}, // 1 Wait for json_tabata_enable
    {&json_rapid_time, ONE_SECOND, LED_TABATA_ON,   LED_ON,   "TABATA_ON",    IN_SHOT, &always_true, 3, 3}, // 2 Wait for json_tabata_enable
    {&grace_time,      TICK_10ms,  LED_TABATA_OFF,  LED_DARK, "TABATA_GRACE", IN_SHOT, &cycle_count, 4, 4}, // 3 Delay for the grace period
    {&all_done,        ONE_SECOND, LED_TABATA_OFF,  LED_DARK, "TABATA_DONE",  0,       &always_true, 0, 1}, // 4 Fake the rest period
};

static void start_rapid_fire(void)
{
  //  {"RAPID_COUNT":5, "RAPID_WAIT":7, "RAPID_TIME":50, "RAPID_ENABLE": 1 }
  real_t temp;

  temp = (real_t)json_rapid_time;             // Total event time
  temp -= json_rapid_count * json_rapid_wait; // Subtract the total wait time between shots
  temp /= json_rapid_count;                   // Calculate the average time per shot
  json_rapid_time = temp;                     // Update the rapid time with the calculated average per shot
  return;
}

static void start_sport_pistol(void)
{
  //  {"RAPID_COUNT":5, "RAPID_WAIT":7, "RAPID_TIME":50, "RAPID_ENABLE": 1 }
  real_t temp;

  temp = (real_t)json_rapid_time;             // Total event time
  temp -= json_rapid_count * json_rapid_wait; // Subtract the total wait time between shots
  temp /= json_rapid_count;                   // Calculate the average time per shot
  json_rapid_time = temp;                     // Update the rapid time with the calculated average per shot
  return;
}

static void start_tabata(void)
{
  return;
}

const course_of_fire_t course_of_fire[] = {
    {"RFP", &rapid_state_ISSF, 5, start_rapid_fire},
    {"SPP", &rapid_state_sport, 5, start_sport_pistol},
    {"TBT", &tabata_rapid_state, 0, start_tabata},
    {NULL, 0, 0}
};

void timed_event_task(void)
{
  static unsigned int   first_shot;         //  Remember the index of the first shot
  static rapid_state_t *rapid_state = NULL; // What state table to use
  static unsigned int   rapid_index;        // Index of the current state
  static int            last_enable;        // Enabled state from last iteraton
  static int            last_in_shot;       // Is the system currently in a shot
  int                   i, j;               // Loop counters

  IF_NOT(IN_OPERATION) return;

                                            /*
                                             * Exit if Rapid fire has not been enabled
                                             */
  if ( (last_enable & (last_enable ^ json_rapid_enable)) == 1 ) // Go from enabled to disabled
  {
    DLT(DLT_RAPID_FIRE, SEND(CONSOLE, sprintf(_xs, "Timed fire ended");))
  }

  if ( json_rapid_enable == false )                             // Nothing to do if Rapid fire is not enabled
  {
    last_enable  = 0;
    last_in_shot = 0;
    return;
  }

  /*
   *  Look to see if we are starting a new timed fire sequence
   */
  if ( (json_rapid_enable & (last_enable ^ json_rapid_enable)) == 1 ) // Go from disabled to enabled
  {
    i = 0;
    while ( course_of_fire[i].event != NULL )
    {
      if ( contains(json_event, course_of_fire[i].event) )
      {
        rapid_state = course_of_fire[i].rapid_state;
        break;
      }
      i++;
    }

    if ( rapid_state == NULL )
    {
      i           = 0;
      rapid_state = course_of_fire[i].rapid_state;
    }

    event_timer         = ((int)*(rapid_state->timer)) * ONE_SECOND;
    rapid_index         = 0;                                           // Start at the beginning of the state machine
    first_shot          = shot_in;                                     // Remember where we started
    adjusted_rapid_wait = (json_rapid_wait * ONE_SECOND) - grace_time; // Corrected rapid wait time
    if ( course_of_fire[i].relay != 0 )                                // Number of expected shots
    {
      shot_string = json_rapid_repeat;                                 // Number of expected shots
    }
    else
    {
      shot_string = json_rapid_count;                                  // Fallback to the default number of expected shots
    }
    cycle_count = shot_string;                                         // Initialize the cycle count with the number of expected shots
    run_state |= IN_RAPID;

    if ( course_of_fire[i].start_up != NULL )
    {
      course_of_fire[i].start_up();
    }
    DLT(DLT_RAPID_FIRE, SEND(CONSOLE, sprintf(_xs, "Starting: %s, cycle_count: %d, Wait: %4.2f, On: %4.2f", course_of_fire[i].event,
                                              cycle_count, json_rapid_wait, json_rapid_time);))
  }

  last_enable = json_rapid_enable;

  /*
   *  Execute the state machine
   */
  if ( event_timer != 0 ) // Time has not yet elapsed
  {
    return;
  }

  /*
   *  Go to the next state in the rapid fire sequence
   */
  if ( *((rapid_state + rapid_index)->exit_condition) != 0 ) // Decrement the exit condition if it is not zero
  {
    (*(rapid_state + rapid_index)->exit_condition)--;
  }

  if ( *((rapid_state + rapid_index)->exit_condition) == 0 ) // Pick the next state based on the exit condition
  {
    rapid_index = (rapid_state + rapid_index)->zero;
  }
  else
  {
    rapid_index = (rapid_state + rapid_index)->not_zero;
  }

  /*
   *  Set the timer for the next state
   */
  event_timer = ((int)*(rapid_state + rapid_index)->timer) * (rapid_state + rapid_index)->time_scale; // New time

  switch ( (rapid_state + rapid_index)->LED_bright )                         // Set the LED brightness based on the state
  {
    case LED_RAMP:
      set_LED_PWM_now(10);                                                   // Ramp the lights on
      break;

    case LED_ON:
      set_LED_PWM_now(json_LED_PWM);                                         // Turn on the lights
      break;

    case LED_DARK:
      set_LED_PWM_now(0);                                                    // Turn off the lights
      break;
  }

  set_status_LED((rapid_state + rapid_index)->status_LED);                   // Set the red and green light s

  run_state = (run_state & ~IN_SHOT) | (rapid_state + rapid_index)->in_shot; // Update the IN_SHOT state

  if ( (IN_SHOT & (run_state & (last_in_shot ^ run_state))) != 0 )           // If the target has transitioned into a shot
  {
    for ( j = shot_in, i = 0; i != shot_string; i++ )
    {
      record[j].shot          = j;                                           // Fake a shot number
      record[j].sensor_status = 0;                                           // Clear th
      record[j].miss          = 1;                                           // Assume we miss
      record[j].shot_time     = run_time_ms();
      record[j].face_strike   = 0;                                           // Reset face strike
      record[j].x_mm          = DIAMETER;                                    // Reset x coordinate to be way off
      record[j].y_mm          = DIAMETER;                                    // Reset y coordinate to be way off
      j                       = (j + 1) % SHOT_SPACE;
    }
  }
  last_in_shot = run_state & IN_SHOT;

  /*
   *  Check to see if we have reached the end of the timed fire event
   */
  if ( rapid_index == 0 )
  {
    i = shot_string - (shot_in - first_shot);               // However many shots are missing
    if ( i != 0 )                                           // Did we shoot all of the shots?
    {
      for ( j = 0; j < shot_string; j++ )                   // Look through all of the shots
      {
        if ( record[first_shot].miss == 1 )                 // Is this a miss?
        {
          build_json_score(&record[first_shot], SCORE_USB); // Build the JSON for the miss
          serial_to_all(_xs, ALL);
          vTaskDelay(2);
          first_shot = (first_shot + 1) % SHOT_SPACE;       // Go onto the next one
        }
      }
    }
    json_rapid_enable = 0;                                  // No longer enabled
    run_state &= ~IN_RAPID;                                 //
  }

  DLT(DLT_RAPID_FIRE, SEND(CONSOLE, sprintf(_xs, "State: %s,  time: %ld, LED: \"%s\", IN_SHOT: %d", (rapid_state + rapid_index)->message,
                                            event_timer, (rapid_state + rapid_index)->status_LED, (rapid_state + rapid_index)->in_shot);))

  /*
   * All done.
   */
  return;
}
