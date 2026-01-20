/*-------------------------------------------------------
 *
 * file: timer_ISR.c
 *
 * Timer interrupt file
 *
 *-------------------------------------------------------
 *
 * The timer interrupt is used to generate internal timers
 * and poll the sensor inputs looking for shot detection
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/peripherals/timer.html
 *
 * ----------------------------------------------------*/

#include "stdbool.h"
#include "esp_timer.h"
#include "driver\timer.h"

#include "freETarget.h"
#include "helpers.h"
#include "diag_tools.h"
#include "gpio_types.h"
#include "json.h"
#include "serial_io.h"
#include "mfs.h"
#include "token.h"
#include "timer.h"
#include "dac.h"

/*
 * Definitions
 */
#define FREQUENCY 1000ul                 // 1000 Hz
#define N_TIMERS  32                     // Keep space for 32 timers

#define BAND_10ms   1                    // vTaskDelay in 10 ms
#define BAND_100ms  (TICK_10ms * 10)     // vTaskDelay in 100 ms
#define BAND_250ms  (TICK_10ms * 25)     // vTaskDelay in 250 ms
#define BAND_500ms  (TICK_10ms * 50)     // vTaskDelay in 500 ms
#define BAND_1000ms (TICK_10ms * 100)    // vTaskDelay in 1000 ms
#define BAND_60s    ((BAND_1000ms) * 60) // vTaskDelay in 1 minute

typedef enum
{
  PORT_STATE_IDLE = 0,                   // There are no sensor inputs
  PORT_STATE_WAIT,                       // Some sensor inputs are present
  PORT_STATE_TIMEOUT                     // Wait for the ringing to stop
} state;

typedef struct
{
  time_count_t cycle_time;               // How long between calls
  void (*f)(void);                       // Function to execute at the cycle time
} synchronous_task_t;

typedef struct
{
  time_count_t *run_time;                // Pointer to running timer
  void (*callback)(void);                // Function to execute when time hits zero
  char *name;
} run_time_clock_t;

/*
 * Local Variables
 */
static run_time_clock_t timers[N_TIMERS];   // Active timer list (allow only positive time)
static state            isr_state;          // What sensor state are we in
static time_count_t     base_time = 0;      // Base time to show elapsed time
time_count_t            time_to_go;         // Time remaining in event in seconds

static synchronous_task_t task_list[] = {
    {BAND_10ms,   token_cycle              }, // Check for token ring activity
    {BAND_10ms,   multifunction_switch_tick}, // Look for MFS changes
    {BAND_10ms,   multifunction_switch     },
    {BAND_10ms,   paper_drive_tick         }, // Drive the paper drive motor
    {BAND_500ms,  toggle_status_LEDs       }, // Blink the LEDs
    {BAND_500ms,  tabata_task              }, // Manage the Tabata timer
    {BAND_500ms,  rapid_fire_task          }, // Manage the rapid fire timer
    {BAND_1000ms, check_12V                }, // Monitor the 12V supply
    {BAND_1000ms, send_keep_alive          }, // Send a keep alive message
    {BAND_1000ms, check_new_connection     }, // Check for a new WiFi connection
    {BAND_60s,    watchdog                 }, // Watchdog monitor
    {0,           0                        }
};

/*
 *  Function Prototypes
 */
static bool IRAM_ATTR freeETarget_timer_isr_callback(void *args);

/*-----------------------------------------------------
 *
 * @function: freeETarget_timer_init
 *
 * @brief:    Initialize the timer interrupt
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * The FreeETarget software uses the FreeRTOS system calls
 * to generate the cycle times needed to run the software
 * Unfortunatly, the FreeRTOS cycle time is 10 ms which is
 * too slow (infrequent) to manage the shot sensors
 * correctly.  For this reason the sensor polling is done
 * by a 1 ms timer interrupt directly from the operating
 * system
 *
 *-----------------------------------------------------*/
#define TIMER_DIVIDER (16)                   //  Hardware timer clock divider
#define TIMER_SCALE   (1000 / TIMER_DIVIDER) // convert counter value to seconds
#define ONE_MS        (80 * TIMER_SCALE)     // 1 ms timer interrupt

const timer_config_t config = {
    .clk_src     = RMT_CLK_SRC_APB,
    .divider     = TIMER_DIVIDER,
    .counter_dir = TIMER_COUNT_UP,
    .counter_en  = TIMER_PAUSE,
    .alarm_en    = TIMER_ALARM_EN,
    .auto_reload = 1,
}; // default clock source is APB

void freeETarget_timer_init(void)
{
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "freeETarget_timer_init()");))
  timer_init(TIMER_GROUP_0, TIMER_1, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);    // Start the timer at 0
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, ONE_MS); // Trigger on this value
  timer_enable_intr(TIMER_GROUP_0, TIMER_1);             // Interrupt associated with this interrupt
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, freeETarget_timer_isr_callback, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_1);
  shot_in  = 0;
  shot_out = 0;

  /*
   *  Timer running. return
   */
  return;
}

void freeETarget_timer_pause(void) // Stop the timer
{
  timer_pause(TIMER_GROUP_0, TIMER_1);
  return;
}

void freeETarget_timer_start(void) // Start the timer
{
  timer_start(TIMER_GROUP_0, TIMER_1);
  return;
}

void show_timers(void)             // Show the current timers
{
  unsigned int i;

  SEND(ALL, sprintf(_xs, "\r\nCurrent Timers:\r\n");)
  for ( i = 0; i != N_TIMERS; i++ )
  {
    if ( timers[i].run_time != 0 ) // Valid timer
    {
      SEND(ALL, sprintf(_xs, "  %s: %ld\r\n", timers[i].name, *timers[i].run_time);)
    }
  }
  return;
}

/*-----------------------------------------------------
 *
 * @function: freeETarget_timer_isr_callback
 *
 * @brief:    High speed synchronous task
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * This task is called every 1 ms from the timer
 * interrupt
 *
 * Timer 1 samples the inputs and when all of the
 * sendor inputs are present, the counters are
 * read and made available to the software
 *
 * There are three data aquisition states
 *
 * IDLE    - Wait for a shot to arrive
 * WAIT    - Inputs are present, but we have to wait
 *           for all of the sensors to be present or
 *           timed out
 * TIMEOUT - We have read the counters but need to
 *           wait for the ringing to stop
 *
 *
 *-----------------------------------------------------*/
static bool IRAM_ATTR freeETarget_timer_isr_callback(void *args)
{
  BaseType_t   high_task_awoken = pdFALSE;
  unsigned int pin;                                       // Value read from the port

  IF_NOT(IN_OPERATION) return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR

  /*
   * Decide what to do if based on what inputs are present
   */
  pin = is_running() & RUN_MASK; // Read in the RUN bits

  /*
   * Read the shot based on the ISR state
   */
  switch ( isr_state )
  {
    case PORT_STATE_IDLE:                                    // Idle, Wait for something to show up
      if ( pin != 0 )                                        // Something has triggered
      {
        shot_timer = MAX_WAIT_TIME;                          // The wait timer makes sure all sensors are triggered
        isr_state  = PORT_STATE_WAIT;                        // Got something wait for all of the sensors tro trigger
      }
      break;

    case PORT_STATE_WAIT:                                    // Something is present, wait for all of the inputs
      if ( (pin == RUN_MASK)                                 // We have all of the inputs
           || (shot_timer == 0) )                            // or ran out of time.  Read the timers and restart
      {
        aquire();                                            // Read the counters
        ring_timer = json_min_ring_time * ONE_SECOND / 1000; // Reset the ring timer
        isr_state  = PORT_STATE_TIMEOUT;                     // and wait for the all clear
      }
      break;

    case PORT_STATE_TIMEOUT:                                 // Wait for the ringing to stop
      if ( ring_timer == 0 )
      {
        stop_timers();                                       // Clear the flipflops
        arm_timers();                                        // Arm the timers for the next shot
        isr_state = PORT_STATE_IDLE;                         // The ringing has stopped
      }
      break;
  }

  /*
   * Return from interrupts
   */
  return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

/*-----------------------------------------------------
 *
 * @function: freeETarget_timers
 *
 * @brief:    Update the free running timers
 *
 * @return:   Never
 *
 *-----------------------------------------------------
 *
 * This task runs every 10ms.
 *
 * The free running timers are decrimented and when they
 * hit zero, the individual timer is deleted
 *
 *-----------------------------------------------------*/
void freeETarget_timers(void *pvParameters)
{
  unsigned int i;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "freeETarget_timers()");))

  /*
   *  Decrement the timers on a 10ms (100Hz) interval
   */
  while ( 1 )
  {
    IF_NOT(IN_STARTUP)                       // Dont run the timers if we are in startup
    {
      for ( i = 0; i != N_TIMERS; i++ )      // Refresh the timers.  Decriment in 10ms increments
      {
        if ( timers[i].run_time != 0 )       // The timer has a valid pointer
        {
          if ( *timers[i].run_time > 0 )     // And is non-sero
          {
            (*timers[i].run_time)--;         // Decriment the timer

            if ( *timers[i].run_time <= 0 )  // Timer has expired
            {
              *timers[i].run_time = 0;       // Set the timer to zero
              if ( timers[i].callback != 0 ) // If there is a function to call
              {
                timers[i].callback();        // Call the function
              }
            }
          }
        }
      }
      vTaskDelay(TICK_10ms);
    }
  }
  /*
   * Never get here
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: freeETarget_synchronous
 *
 * @brief:    Synchronous task scheduler
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * This task runs every 10ms.
 *
 * When called, the task list is polled and when the
 * time is a multiple of the cycle time, the function
 * is called,
 *
 *-----------------------------------------------------*/
void freeETarget_synchronous(void *pvParameters)
{
  unsigned int cycle_count   = 0;
  unsigned int old_run_state = 0;
  unsigned int i; // Index into the task list

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "freeETarget_synchronous()");))

  while ( 1 )
  {
    i = 0;
    while ( task_list[i].cycle_time != 0 ) // Cycle through the task list
    {
      if ( (cycle_count % task_list[i].cycle_time) == 0 )
      {
        task_list[i].f();                  // Call the function
      }
      i++;
    }

    /*
     * 60 second band
     */
    if ( ((cycle_count % BAND_60s) == 0)         // Sixty second timer
         || ((run_state ^ old_run_state) != 0) ) // or state change
    {
      heartbeat();
    }
    old_run_state = run_state;                   // Remember the state

    /*
     * All done, prepare for the next cycle
     */
    cycle_count++;
    vTaskDelay(TICK_10ms); // Delay 10ms
  }
}

/*-----------------------------------------------------
 *
 * @function: ft_timer_new()
 *            ft_timer_delete()
 *
 * @brief:    Add or remove timers
 *
 * @return:   TRUE if the operation was a success
 *
 *-----------------------------------------------------
 *
 *
 * These functions add or remove a timer from the active
 * timer list
 *
 * IMPORTANT
 *
 * The timers must be static variables, otherwise they
 * will overflow the available space every time they are
 * instantiated.
 *
 * Calling timer_new with the same timer address will
 * overwrite the previous timer value with the new one.
 * timer_new can be called any number of times with the
 * same timer addess without creating a problem
 *
 *-----------------------------------------------------*/
int ft_timer_new(time_count_t *new_timer, // Pointer to new down counter
                 long          duration,  // Duration of the timer
                 void *(callback)(),      // What to do when we hit zero
                 char *name               // Timer name
)
{
  unsigned int i;

  if ( new_timer == NULL )
  {
    return 0;
  }

  for ( i = 0; i != N_TIMERS; i++ )                    // Look through the space
  {
    if ( (timers[i].run_time == 0)                     // Got an empty timer slot
         || (timers[i].run_time == new_timer) )        // or it already exists
    {
      timers[i].run_time = new_timer;                  // Add it in
      timers[i].callback = callback;                   // Set the callback
      timers[i].name     = name;                       // Record the name
      *new_timer         = duration - (duration % 10); // Set the timer value (round to 10ms)
      return 1;
    }
  }
  DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "No space for new timer");))

  return 0;
}

int ft_timer_delete(time_count_t *old_timer) // Pointer to new down counter
{
  unsigned int i;

  if ( old_timer == 0 )                      // Null pointer, do nothing
  {
    return 0;
  }

  *old_timer = 0;                            // Set the timer to zero

  for ( i = 0; i != N_TIMERS; i++ )          // Look through the space
  {
    if ( timers[i].run_time == old_timer )   // Found the existing timer
    {
      timers[i].run_time = NULL;             // Remove the pointer
      timers[i].callback = NULL;             // Clear the callback
      return 1;
    }
  }

  /*
   *  The timer doesn't exist, return an error
   */
  return 0;
}

/*-----------------------------------------------------
 *
 * @function: show_time()
 *
 * @brief:    Print out the current time
 *
 * @return:   NONE
 *
 *-----------------------------------------------------
 *
 * Demonstration function to test the esp_timer.
 * Display the internal timer every second
 *
 *---------------------------------------------------*/
void show_time(void)
{
  long time;

  SEND(ALL, sprintf(_xs, "\r\nTime test.  Press any key to exit\r\n");)

  while ( serial_available(ALL) == 0 )
  {
    time = run_time_seconds();
    SEND(ALL, sprintf(_xs, "\r\n%ld.%ld s", time / 1000, time % 1000);)
    vTaskDelay(ONE_SECOND);
  }

  SEND(ALL, sprintf(_xs, _DONE_);)

  return;
}

/*-----------------------------------------------------
 *
 * @function: run_time_seconds()
 *            reset_run_time()
 *
 * @brief:    Return the run time in seconds
 *            Reset the timer to now
 *
 * @return:   time in seconds since reset
 *
 *-----------------------------------------------------
 *
 * Common timer function
 *
 *---------------------------------------------------*/
time_count_t run_time_seconds(void)
{
  return (esp_timer_get_time() - base_time) / 1000000;
}

time_count_t run_time_ms(void)
{
  return (esp_timer_get_time() - base_time) / 1000;
}

void reset_run_time(void)
{
  base_time = 0;
  return;
}
