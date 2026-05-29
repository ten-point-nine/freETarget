/*-------------------------------------------------------
 *
 * gpio.c
 *
 * General purpose GPIO driver
 *
 * ----------------------------------------------------*/
#include <string.h>
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "gpio_types.h"
#include "driver\gpio.h"
#include "esp_timer.h"

#include "trace.h"
#include "board_assembly.h"
#include "diag_tools.h"
#include "gpio.h"
#include "timer.h"
#include "json.h"
#include "timer.h"
#include "gpio.h"
#include "gpio_define.h"
#include "serial_io.h"

/*
 * function prototypes
 */

/*
 *  Typedefs
 */

/*
 * Variables
 */

/*-----------------------------------------------------
 *
 * @function: board_version()
 *
 * @brief:    Find out the board revision
 *
 * @return:   board version
 *
 *-----------------------------------------------------
 *
 * Read the board revisions and return the value
 *
 *-----------------------------------------------------*/
unsigned int board_version(void)
{
  return gpio_get_level(BD_REV);
}

/*-----------------------------------------------------
 *
 * @function: digital_test()
 *
 * @brief:    Exercise the GPIO digital ports
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Read in all of the digial ports and report the
 * results
 *
 *-----------------------------------------------------*/
void digital_test(void)
{
  SEND(ALL, sprintf(_xs, "\r\nDigital Inputs:");)
  /*
   * Read in the fixed digital inputs
   */
  while ( 1 )
  {
    if ( serial_available(ALL) != 0 )
    {
      break;
    }
    vTaskDelay(ONE_SECOND);
  }

  SEND(ALL, sprintf(_xs, _DONE_);)

  return;
}

/*-----------------------------------------------------
 *
 * @function: status_LED_timer()
 *
 * @brief:    Timer to drive the status LED
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * This timer is called every 100 ms and
 * uses the status_LED_mask to drive the status LED
 *
 * The status_LED_mask is determined by the current running
 * state.
 *
 *-----------------------------------------------------*/
typedef struct
{
  unsigned int state;                          //  Current Running State
  char        *mask;                           // Status mask associated with the state
} status_LED_t;

static status_LED_t states[] = {
    {IN_FATAL_ERROR,  LED_ERROR       }, // Blink the LEDs based on the state
    {IN_STARTUP,      LED_STARTUP     }, // This table is organized in highest
    {IN_FIFO_FILLING, LED_FIFO_FILLING}, // to lowest priority.
    {IN_REDUCTION,    LED_REDUCTION   },
    {IN_NO_CAL,       LED_NO_CAL      }, // Not calibrated.  Cannot start
    {IN_OPERATION,    LED_READY       }, // Operation, working, FIFO Data full
    {0,               0               }
};

void status_LED_timer(void)
{
  int                 i;
  static unsigned int status_LED_count = 0;    // Count of the number of times the timer has been called
  char               *status_LED_mask  = NULL; // Pattern to display

  /*
   *  Search the list
   */
  i = 0;
  while ( states[i].mask != 0 )
  {
    if ( (run_state & states[i].state) != 0 )
    {
      status_LED_mask = states[i].mask;
      break; // Remeber and exit when we find a match
    }
    i++;
  }

  gpio_set_level(STATUS_LED, *(status_LED_mask + (status_LED_count % 32)) == '*');

  status_LED_count++; // Increment the count

  return;
}
