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
#include "esp_timer.h"

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
#include "IMU.h"
#include "NTP.h"
#include "server.h"
#include "client.h"

/*
 *  Variables
 */
time_count_64_t keep_alive_timer; // TCPIP keep alive timer

/*
 * Function Prototypes
 */
extern void gpio_init(void);

/*
 *  External variables
 */
extern FIFO_raw_t      sample_raw_read[];
extern time_count_64_t sync_time_remaining;

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
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT APPLICATON enabled");))
#endif
#if TRACE_COMMUNICATION
  is_trace |= DLT_COMMUNICATION; // Enable application tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT COMMUNICATION enabled");))
#endif
#if TRACE_DIAGNOSTICS
  is_trace |= DLT_DIAG;          // Enable diagnostics tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT DIAGNOSTICS enabled");))
#endif
#if TRACE_DEBUG
  is_trace |= DLT_DEBUG;         // Enable debug tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT DEBUG enabled");))
#endif
#if TRACE_PAUSE
  is_trace |= DLT_PAUSE;         // Enable verbose messages
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT PAUSE enabled");))
#endif
#if TRACE_VERBOSE
  is_trace |= DLT_VERBOSE;       // Enable verbose messages
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT VERBOSE enabled");))
#endif

  /*
   * Set the variables
   */

  board_revision = board_version();
  /*
   *  Setup the hardware
   */
  gpio_init();                                   // Setup the hardware
  vTaskDelay(10);                                // Let the hardware settle
  if ( gpio_get_level(SWITCH_GPIO) == 0 )        // If the button is held down
  {
    factory_nonvol(true);                        // Force a re-init
  }
  serial_io_init();                              // Setup the console for debug message
  read_nonvol();                                 // Read in the settings
  if ( (json_x_dotdot_offset | json_theta_dot_offset) == 0 )
  {
    run_state |= IN_NO_CAL;                      // The board is not calibrated
  }

  json_distance_to_target = 10.0;
  json_NTP_period         = NETWORK_TIME_PERIOD; // Reset the watchdog

  BMI270_init(BMI270_CS);                        // Initialize the BMI270 accelerometer
  WiFi_station_init();                           // Connect to an SSID
  WiFi_client_init();                            // Initialize the WiFi client

  /*
   *  Set up the long running timers
   */
  ft_timer_new(&keep_alive_timer, KEEP_ALIVE_TIME_PERIOD, &send_keep_alive, "Keep alive"); // keepalive timer
  ft_timer_new(&sync_time_remaining, json_NTP_period, NULL, "sync_time_remaining");        // Sync to the target (start at 30 seconds)

  /*
   * Run the power on self test
   */

  /*
   * Ready to go
   */
  show_echo();
  serial_flush(ALL); // Get rid of everything

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
 * This is the main control loop for the trace module.
 *
 * It is a simple polling loop that checks the various inputs
 * and executes the task.
 *
 * IMPORANT.
 *
 * No one operation should exceed 500ms.
 *
 *---------------------------------------------------------------*/
int FIFO_pull = 0;            // How many FIFO pulls have we done

time_count_64_t FIFO_time_us; // Last pull in microseconds

void trace_loop(void *arg)
{
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "trace_loop()");))
  run_state |= IN_OPERATION;

  NTP_test();

  while ( 1 )
  {
    if ( gpio_get_level(BMI270_INTERRUPT) == 0 )
    {
      BMI270_pull_FIFO();
      FIFO_pull++;
      FIFO_time_us = NTP_time_us();
    }
    vTaskDelay(TICK_50ms);
  }

  return;                   // Never get here
}

void trace_statistics(void) // Display the FIFO diagnostics
{
  SEND(CONSOLE, sprintf(_xs, "\"FIFO time\": %.2f", ((real_t)FIFO_time_us) / 1000000.0);)
  SEND(CONSOLE, sprintf(_xs, "\"FIFO pull\": %d", FIFO_pull);)
  SEND(CONSOLE, sprintf(_xs, "\"FIFO check\": %d", (int)(((real_t)FIFO_time_us / 1000000.0) * SAMPLE_RATE / RAW_FRAME_COUNT));)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_push_button
 *
 * @brief: Monitor the push button for action
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The push button supports two modes of operation,
 *
 * SHORT_PUSH --
 * LONG_PUSH  -- Recalibrate the trace
 * HOLD_PUSH  --
 *
 * We get here every 100ms from the synchronous foreground
 *
 *---------------------------------------------------------------*/
#define SHORT_PUSH 5                      // 1/2 second press
#define LONG_PUSH  20                     // 2 second press
#define HOLD_PUSH  50                     // Long hold

void trace_push_button(void)
{
  static int time_tick = 0;

  if ( gpio_get_level(SWITCH_GPIO) == 0 ) // If the button is held down
  {
    time_tick++;                          // Increment the timer
  }
  else                                    // Button not held
  {
    if ( time_tick != 0 )                 // The button has just been released
    {
      printf("time_tick %d\r\n", time_tick);
      if ( time_tick < SHORT_PUSH )       // Too short to be recognized
      {
        time_tick = 0;
      }
      if ( time_tick < LONG_PUSH )        // Short push
      {
        time_tick = 0;
      }
      else if ( time_tick < HOLD_PUSH )   // Long push
      {
        run_state |= IN_FIFO_FILLING;     // Reset the FIFO
        vTaskDelay(ONE_SECOND);
        BMI270_find_zero(false);          // Long push, automatically save
        time_tick = 0;
      }
    }
  }

  if ( time_tick >= HOLD_PUSH )
  {
    time_tick = 0;
  }

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_health_monitor
 *
 * @brief:  Watch what is going on and update states
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * We get here every 1000ms from the synchronous foreground
 *
 *---------------------------------------------------------------*/

void trace_health_monitor(void)
{
  /*
   *  Check to see if we are connected to the target
   */
  IF_NOT(CLIENT_CONNECTED)
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Reconnecting to target");))
    WiFi_client_init(); // Try to make a new connection
  }

  /*
   * Check to see how long it's been since we got a time update
   */
  if ( NTP_ttg() )
  {
    NTP_ask();
  }

  /*
   * All done, return
   */
  return;
}
