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

/*
 *  Variables
 */
unsigned int number_of_connections = 0; // How many people are connected to me?

                                        // Timer to reset LED status
int go_dark     = 10l;               // Go dark for 10
int go_wait     = 3l;                // Wait for the PC to catchup
int all_done    = 0l;                // All finished
int always_true = true;

extern int isr_state;

volatile unsigned int run_state = 0; // Current operating state

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
  run_state = IN_STARTUP;
  is_trace  = DLT_INFO | DLT_CRITICAL;
#if TRACE_APPLICATION
  is_trace |= DLT_APPLICATION;   // Enable application tracing
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
   *  Setup the hardware
   */
  gpio_init();      // Setup the hardware
  serial_io_init(); // Setup the console for debug message
  read_nonvol();    // Read in the settings

  /*
   * Put up a self test
   */
  WiFi_init();

  /*
   *  Set up the long running timers
   */
  trace_timer_init(); // Start the timer interrupt to manage the timers
  // ft_timer_new(&keep_alive, (time_count_t)json_keep_alive * ONE_SECOND, send_keep_alive, "keep alive");                 // Keep alive
  // timer ft_timer_new(&power_save, (time_count_t)(json_power_save) * (time_count_t)ONE_SECOND * 60L, &bye_tick, "power save"); // Power
  // save timer
  //  ft_timer_new(&time_since_last_shot, HTTP_CLOSE_TIME * 60 * ONE_SECOND, NULL, "time since last shot"); // 15 minutes since last shot

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
  run_state &= ~IN_STARTUP; // Exit startup
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_task
 *
 * @brief: Main control loop
 *
 * @return: None
 *
 *----------------------------------------------------------------
 */

unsigned int sensor_status;    // Record which sensors contain valid data
unsigned int location;         // Sensor location

void trace_target_loop(void *arg)
{
  while ( 1 )
  {
    run_state |= IN_OPERATION; // We are in operation
    /*
     * End of the loop. timeout till the next time
     */
    vTaskDelay(TICK_10ms);
  }
}