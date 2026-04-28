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
