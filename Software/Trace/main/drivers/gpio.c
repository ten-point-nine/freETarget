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
 * @function: status_LED()
 *
 * @brief:    Set the status LED
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Save the 32 bit status and use it to drive the status LED
 *
 *-----------------------------------------------------*/
static unsigned int status_LED_mask = 0;

void set_status_LED(unsigned int status)
{
  status_LED_mask = status; // Save the status LED for later
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
 * This timer is called every 100 ms and uses the status_LED_mask to drive the status LED
 *
 *-----------------------------------------------------*/
void status_LED_timer(void)
{
  static unsigned int status_LED_count = 0; // Count of the number of times the timer has been called

  gpio_set_level(STATUS_LED, (status_LED_mask & (1 << (status_LED_count % 32))) !=
                                 0);        // Set the status LED based on the current bit in the working status LED mask

  status_LED_count++; // Increment the count
  
  return;
}
