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
#include "diag_tools.h"
#include "gpio_types.h"
#include "json.h"
#include "serial_io.h"
#include "mfs.h"
#include "token.h"
#include "timer.h"

/*
 * Definitions
 */
#define FREQUENCY 1000ul                 // 1000 Hz
#define N_TIMERS  32                     // Keep space for 32 timers

#define MAX_WAIT_TIME 10                 // Wait up to 10 ms for the input to arrive
#define MAX_RING_TIME 50                 // Wait 50 ms for the ringing to stop

#define TICK_10ms   1                    // vTaskDelay in 10 ms
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
/*
 * Local Variables
 */
static volatile unsigned long *timers[N_TIMERS]; // Active timer list
static volatile unsigned long  shot_timer;       // Wait for the sound to hit all sensors
volatile unsigned long         ring_timer;       // Let the ring on the backstop end

static state isr_state;                          // What sensor state are we in

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
  DLT(DLT_INFO, SEND(sprintf(_xs, "freeETarget_timer_init()");))
  timer_init(TIMER_GROUP_0, TIMER_1, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);    // Start the timer at 0
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, ONE_MS); // Trigger on this value
  timer_enable_intr(TIMER_GROUP_0, TIMER_1);             // Interrupt associated with this interrupt
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, freeETarget_timer_isr_callback, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_1);
  timer_new(&ring_timer, 0);                             // Let the pellet trap stop ringing
  timer_new(&shot_timer, 0);                             // Let the sound propagate to the sensors
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
 * IDLE - No inputs are present
 * WAIT - Inputs are present, but we have to wait
 *        for all of the inputs to be present or
 *        timed out
 * DONE - We have read the counters but need to
 *        wait for the ringing to stop
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
  pin = is_running(); // Read in the RUN bits

  /*
   * Read the shot based on the ISR state
   */
  switch ( isr_state )
  {
    case PORT_STATE_IDLE:                                    // Idle, Wait for something to show up
      if ( pin != 0 )                                        // Something has triggered
      {
        shot_timer = MAX_WAIT_TIME;                          // Start the wait timer
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

  DLT(DLT_INFO, SEND(sprintf(_xs, "freeETarget_timers()");))

  /*
   *  Decrement the timers on a 10ms (100Hz) interval
   */
  while ( 1 )
  {
    for ( i = 0; i != N_TIMERS; i++ ) // Refresh the timers.  Decriment in 10ms increments
    {
      if ( (timers[i] != 0) && (*timers[i] != 0) )
      {
        (*timers[i])--;               // Decriment the timer
      }
    }
    vTaskDelay(TICK_10ms);
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
 * Synchronous tasks are called on a 10ms time band
 *
 * Unless you don't care about the time, always use
 * timer_new(&timer, ONE_SECOND * duration).
 *
 * IMPORTANT
 *
 * timers are NOT deleted when they expire.
 *
 *-----------------------------------------------------*/
void freeETarget_synchronous(void *pvParameters)
{
  unsigned int        cycle_count   = 0;
  unsigned int        toggle        = 0;
  static unsigned int old_run_state = 0;

  DLT(DLT_INFO, SEND(sprintf(_xs, "freeETarget_synchronous()");))

  while ( 1 )
  {

    /*
     *  10 ms band
     */
    token_cycle();
    multifunction_switch_tick();
    multifunction_switch();
    paper_drive_tick();

    /*
     *  100 ms band
     */
    if ( (cycle_count % BAND_100ms) == 0 )
    {
    }

    /*
     *  250 ms band
     */
    if ( (cycle_count % BAND_250ms) == 0 )
    {
    }

    /*
     *  500 ms band
     */
    if ( (cycle_count % BAND_500ms) == 0 )
    {
      toggle ^= 1;
      commit_status_LEDs(toggle);
      tabata_task();
      rapid_fire_task();
    }

    /*
     * 1000 ms band
     */
    if ( (cycle_count % BAND_1000ms) == 0 )
    {
      bye(false); // Dim the lights
      check_12V();
      send_keep_alive();
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
 * @function: timer_new()
 *            timer_delete()
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
int timer_new(volatile unsigned long *new_timer, // Pointer to new down counter
              unsigned long           duration   // Duration of the timer
)
{
  unsigned int i;

  if ( new_timer == NULL )
  {
    return 0;
  }

  for ( i = 0; i != N_TIMERS; i++ )    // Look through the space
  {
    if ( (timers[i] == 0)              // Got an empty timer slot
         || (timers[i] == new_timer) ) // or it already exists
    {
      timers[i]  = new_timer;          // Add it in
      *new_timer = duration;
      return 1;
    }
  }
  DLT(DLT_CRITICAL, SEND(sprintf(_xs, "No space for new timer");))

  return 0;
}

int timer_delete(volatile unsigned long *old_timer // Pointer to new down counter
)
{
  unsigned int i;

  if ( old_timer == 0 )
  {
    return 0;
  }

  *old_timer = 0;                   // Set the timer to zero

  for ( i = 0; i != N_TIMERS; i++ ) // Look through the space
  {
    if ( timers[i] == old_timer )   // Found the existing timer
    {
      timers[i] = 0;                // Remove the pointer
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

  SEND(sprintf(_xs, "\r\nTime test.  Press any key to exit\r\n");)

  while ( serial_available(ALL) == 0 )
  {
    time = esp_timer_get_time() / 1000;
    SEND(sprintf(_xs, "\r\n%ld.%ld s", time / 1000, time % 1000);)
    vTaskDelay(ONE_SECOND);
  }

  SEND(sprintf(_xs, _DONE_);)

  return;
}