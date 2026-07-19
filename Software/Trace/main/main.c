/*******************************************************************************
 *
 * main.c
 *
 * trace control loop
 *
 *******************************************************************************
 *
 * Initialize the hardware and software
 *
 * Then setup all of the tasks and exit back to freeRTOS
 *
 ******************************************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_event.h"

#include "trace.h"
#include "helpers.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"
#include "wifi.h"
#include "diag_tools.h"
#include "http_client.h"
#include "NTP.h"
#include "client.h"
#include "server.h"

/*
 * Task Priorities
 */
#define BACKGROUND 0                          // Lowest priority task,  background
#define POLLING    (BACKGROUND + 4)           // Intermittent communications polling
#define NETWORK    (POLLING + 4)              // Intermittent network polling
#define TIMED      (NETWORK + 4)              // Scheduled tasks
#define MUST_RUN   (TIMED + 4) // This task must run
#if ( MUST_RUN >= configMAX_PRIORITIES )
#error MUST_RUN set too high
#endif

#define K1 1024                               // Kilo Bytes
#define K2 (K1 * 2)                           // 2 Kilo Bytes
#define K4 (K1 * 4)                           // 4 Kilo Bytes
#define K6 (K1 * 6)                           // 6 Kilo Bytes
#define K8 (K1 * 8)                           // 8 Kilo Bytes
StaticTask_t trace_loop_tcb;
StackType_t  trace_loop_stack[4096];

/*
 * Start up the tasks
 */
void app_main(void)
{
  run_state = IN_STARTUP; // Show we're in startup

  /*
   *  Start trace
   */
  trace_init();

  /*
   *  Start the client receive task
   */
  IF(STATION_ACTIVE | AP_ACTIVE)
  {
#if BUILD_CLIENT
    if ( xTaskCreate(client_recv, "client_recv", K6, NULL, POLLING, NULL) != pdPASS )
    {
      DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "client_recv()");))
    }
    vTaskDelay(TICK_10ms);
#endif

    /*
     *  Start the server tasks
     */
#if BUILD_SERVER
    if ( xTaskCreate(server_accept_poll, "server_accept_poll", K6, NULL, MUST_RUN, NULL) != pdPASS )
    {
      DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "server_accept_poll()");))
    }
    vTaskDelay(TICK_10ms);

    if ( xTaskCreate(server_receive_poll, "server_receive_poll", K6, NULL, MUST_RUN, NULL) != pdPASS )
    {
      DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "server_receive_poll()");))
    }
    vTaskDelay(TICK_10ms);
#endif
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "No WiFi mode active");))
  }

  /*
   *  Start the timer tasks
   */
  if ( xTaskCreate(trace_timers, "trace_timers", K4, NULL, TIMED, NULL) != pdPASS )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "trace_timers()");))
  }
  vTaskDelay(TICK_10ms);

  if ( xTaskCreate(trace_synchronous, "trace_synchronous", K6, NULL, TIMED, NULL) != pdPASS )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "trace_sunchronous()");))
  }
  vTaskDelay(TICK_10ms);

  /*
   *  Start the JSON processing task
   */
  if ( xTaskCreate(trace_json, "trace_json", K6, NULL, BACKGROUND, NULL) != pdPASS )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "trace_json()");))
  }
  vTaskDelay(TICK_10ms);

  /*
   *  Finally, start the trace loop
   */
  if ( xTaskCreate(trace_loop, "trace_loop", K6, NULL, MUST_RUN, NULL) != pdPASS )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to start %s", "trace_loop()");))
  }
  vTaskDelay(TICK_10ms);

  /*
   *  Indicate that the system is running
   */
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "SN:%d Running\r\n", json_serial_number);))
  vTaskDelay(TICK_10ms);
  serial_flush(ALL);
}
