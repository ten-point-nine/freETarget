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

#define TRACE_C
#include "trace.h"
#include "board_assembly.h"
#include "helpers.h"
#include "gpio.h"
#include "gpio_define.h"
#include "json.h"
#include "nonvol.h"
#include "mechanical.h"
#include "diag_tools.h"
#include "timer.h"
#include "serial_io.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "http_client.h"
#include "BMI270.h"

/*
 *  Variables
 */

/*
 * Function Prototypes
 */
extern void gpio_init(void);

/*----------------------------------------------------------------
 *
 * @function: trace_init()
 *
 * @brief: Initialize the board and prepare to run
 *
 * @return: None
 *
 *--------------------------------------------------------------*/

void trace_init(void)
{
  is_trace = DLT_FATAL | DLT_INFO | DLT_CRITICAL;
#if TRACE_APPLICATION
  is_trace |= DLT_APPICATION;    // Enable application tracing
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "DLT APPLICATON enabled");))
#endif
#if TRACE_COMMUNICATION
  is_trace |= DLT_COMMUNICATION; // Enable application tracing
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "DLT COMMUNICATION enabled");))
#endif
#if TRACE_DIAGNOSTICS
  is_trace |= DLT_DIAG;          // Enable diagnostics tracing
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "DLT DIAGNOSTICS enabled");))
#endif
#if TRACE_DEBUG
  is_trace |= DLT_DEBUG;         // Enable debug tracing
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "DLT DEBUG enabled");))
#endif
#if TRACE_VERBOSE
  is_trace |= DLT_VERBOSE;       // Enable verbose messages
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "DLT VERBOSE enabled");))
#endif

  /*
   * Set the variables
   */
  sample_in  = 0; // Set up the sample queue
  sample_out = 0;

  board_revision = board_version();
  /*
   *  Setup the hardware
   */
  gpio_init();            // Setup the hardware
  vTaskDelay(10);         // Let the hardware settle
  serial_io_init();       // Setup the console for debug message
  read_nonvol();          // Read in the settings
  BMI270_init(BMI270_CS); // Initialize the BMI270 accelerometer

  WiFi_init();

  /*
   *  Set up the long running timers
   */

  /*
   * Run the power on self test
   */
  /*
   * Ready to go
   */
  show_echo();
  serial_flush(ALL);         // Get rid of everything
  connection_list = CONSOLE; // The consule is always connected
  reset_run_time();          // Reset the time of day

  /*
   * Start the tasks running
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_loop
 *
 * @brief: Main control loop
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *
 *---------------------------------------------------------------*/
void trace_loop(void *arg)
{
  run_state &= ~IN_STARTUP;
  run_state |= IN_OPERATION;

  while ( 1 )
  {
    if ( gpio_get_level(SWITCH_GPIO) == 0 )
    {
      BMI270_find_zero();
      SEND(ALL, sprintf(_xs, _DONE_);)
    }

//    if ( gpio_get_level(BMI270_INTERRUPT) == 0 )
    {
      BMI270_pull_FIFO();
      vTaskDelay(100);
    }

    /*
     * End of the loop. timeout till the next time
     */
    vTaskDelay(TICK_10ms);
  }
}