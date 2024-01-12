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
#include "driver\timer.h"
#include "freETarget.h"
#include "diag_tools.h"
#include "gpio_types.h"
#include "json.h"

/*
 * Definitions
 */
#define FREQUENCY 1000ul                        // 1000 Hz
#define N_TIMERS       32                       // Keep space for 32 timers
#define PORT_STATE_IDLE 0                       // There are no sensor inputs
#define PORT_STATE_WAIT 1                       // Some sensor inputs are present, but not all
#define PORT_STATE_DONE 2                       // All of the inmputs are present

#define MAX_WAIT_TIME   10                      // Wait up to 10 ms for the input to arrive
#define MAX_RING_TIME   50                      // Wait 50 ms for the ringing to stop

/*
 * Local Variables
 */
static volatile unsigned long* timers[N_TIMERS];  // Active timer list
       unsigned int isr_state;                    // What sensor state are we in 
static volatile unsigned long isr_timer;          // Interrupt timer 

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
#include "freETarget.h"
#include "timer.h"
#include "mfs.h"
#include "token.h"

#define TIMER_DIVIDER         (16)                    //  Hardware timer clock divider
#define TIMER_SCALE           (1000/ TIMER_DIVIDER)   // convert counter value to seconds
#define ONE_MS                (80 * TIMER_SCALE)      // 1 ms timer interrupt

  timer_config_t config = {
      .clk_src      = RMT_CLK_SRC_APB,
      .divider      = TIMER_DIVIDER,
      .counter_dir  = TIMER_COUNT_UP,
      .counter_en   = TIMER_PAUSE,
      .alarm_en     = TIMER_ALARM_EN,
      .auto_reload  = 1,
  }; // default clock source is APB

void freeETarget_timer_init(void)
{
  DLT(DLT_CRITICAL); 
  printf("freeETarget_timer_init()");
  timer_init(TIMER_GROUP_0, TIMER_1, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);                   // Start the timer at 0
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, ONE_MS);                // Trigger on this value
  timer_enable_intr(TIMER_GROUP_0, TIMER_1);                            // Interrupt associated with this interrupt
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, freeETarget_timer_isr_callback, NULL, 0);
  timer_start(TIMER_GROUP_0, TIMER_1);
  timer_new(&isr_timer, 0);
  this_shot = 0;

/*
 *  Timer running. return
 */
  return;
}

void freeETarget_timer_pause(void)                                        // Stop the timer
{
  timer_pause(TIMER_GROUP_0, TIMER_1);
  return;
}

void freeETarget_timer_start(void)                                        // Start the timer
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
  BaseType_t high_task_awoken = pdFALSE;
  unsigned int pin;                             // Value read from the port

  if ( run_state & IN_TEST )
  {
/*
 * Decide what to do if based on what inputs are present
 */
    pin = is_running();                         // Read in the RUN bits

/*
 * Read the timer hardware based on the ISR state
 */
    switch (isr_state)
    {
      case PORT_STATE_IDLE:                       // Idle, Wait for something to show up
        if ( pin != 0 )                           // Something has triggered
        { 
          isr_timer = MAX_WAIT_TIME;              // Start the wait timer
          isr_state = PORT_STATE_WAIT;            // Got something wait for all of the sensors tro trigger
        }
        break;
          
      case PORT_STATE_WAIT:                       // Something is present, wait for all of the inputs
        if ( (pin == RUN_MASK)                    // We have all of the inputs
            || (isr_timer == 0) )                 // or ran out of time.  Read the timers and restart
         { 
          aquire();                               // Read the counters
          isr_timer = json_min_ring_time;         // Reset the timer
          isr_state = PORT_STATE_DONE;            // and wait for the all clear
        }
        break;
      
      case PORT_STATE_DONE:                       // Waiting for the ringing to stop
        if ( pin != 0 )                           // Something got latched
        {
          isr_timer = json_min_ring_time;
          stop_timers();                          // Reset and try later
        }
        else
        {
          if ( isr_timer == 0 )                   // Make sure there is no rigning
          {
            arm_timers();                         // and arm for the next time
            isr_state = PORT_STATE_IDLE;          // and go back to idle
          } 
        }
        break;
    }
  }

/*
 * Return from interrupts
 */
  return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
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
 * Synchronous tasks are called on a time band
 * 
 * 
 *-----------------------------------------------------*/
#define TICK    1       // 1 Tick = 10 ms
#define BAND_10ms       (TICK * 1)
#define BAND_100ms      (TICK * 10)
#define BAND_500ms      (TICK * 50)
#define BAND_1000ms     (TICK * 100)

void freeETarget_synchronous
(
  void *pvParameters
)
{
  unsigned int cycle_count = 0;
  unsigned int toggle =0;
  unsigned int i;

  while (1)
  {
/*
 *  10 ms band
 */
//    token_cycle();
    for (i=0; i != N_TIMERS; i++)  // Refresh the timers
    {
      if ( (timers[i] != 0)
        && ( *timers[i] != 0 ) )
      {
        (*timers[i])--;
      }
    }

/*
 *  500 ms band
 */
    if ( (cycle_count  %  BAND_500ms) == 0 )
    {
      commit_status_LEDs( toggle );
      toggle ^= 1;
      multifunction_switch();
      tabata_task();
      rapid_fire_task();
    }

/*
 * 1000 ms band
 */
    if ( (cycle_count % BAND_1000ms) == 0 )
    {
      bye();                                           // Dim the lights  
      send_keep_alive();
    }

/*
 * All done, prepare for the next cycle
 */
    cycle_count++;
    vTaskDelay(BAND_10ms);                           // Delay 10ms
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
 * The timers shall be static variables, otherwise they
 * will overflow the available space every time they are
 * instantiated.
 * 
 * Calling timer_new with the same timer address will 
 * overwrite the previous timer value with the new one.
 * timer_new can be called any number of times with the
 * same timer addess without creating a problem
 * 
 *-----------------------------------------------------*/
unsigned long timer_new
(
  volatile unsigned long* new_timer, // Pointer to new down counter
           unsigned long  duration   // Duration of the timer
)
{
  unsigned int i;

  if ( new_timer == NULL )
  {
    return 0;
  }

  for (i=0;  i != N_TIMERS; i++ )   // Look through the space
  {
    if ( (timers[i] == 0)           // Got an empty timer slot
      || (timers[i] == new_timer) ) // or it already exists
    {
      timers[i] = new_timer;        // Add it in
      *new_timer = duration;
      return 1;
    }
  }
  if ( DLT(DLT_CRITICAL) )
  {
    printf("No space for new timer");
  }
  
  return 0;
}

unsigned long timer_delete
(
  volatile unsigned long* old_timer   // Pointer to new down counter
)
{
  unsigned int i;

  for (i=0;  i != N_TIMERS; i++ )   // Look through the space
  {
    if ( timers[i] == old_timer )   // Found the existing timer
    {
      timers[i] = 0;                // Add it in
      return 1;
    }
  }

  return 0;
  
}
