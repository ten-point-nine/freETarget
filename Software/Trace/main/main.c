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

/*
 * Task Priorities
 */
#define BACKGROUND 0                // Lowest priority task,  background
#define POLLING    (BACKGROUND + 4) // Intermittent communications polling
#define NETWORK    (POLLING + 4)    // Intermittent network polling
#define TIMED      (NETWORK + 4)    // Scheduled tasks
#define MUST_RUN   (TIMED + 4)      // This task must run
#if ( MUST_RUN >= configMAX_PRIORITIES )
#error MUST_RUN set too high
#endif

#define K1 1024                     // Kilo Bytes
#define K2 (K1 * 2)                 // 2 Kilo Bytes
#define K4 (K1 * 4)                 // 4 Kilo Bytes
#define K6 (K1 * 6)                 // 6 Kilo Bytes
#define K8 (K1 * 8)                 // 8 Kilo Bytes

/*
 * Start up the tasks
 */
void app_main(void)
{
  /*
   *  Start trace
   */
  trace_init();

  /*
   * Everything is ready, start the threads.  Low task priority number == low priority
   */
  xTaskCreate(trace_target_loop, "trace_target_loop", K4, NULL, MUST_RUN, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(trace_timers, "trace_timer", K4, NULL, TIMED, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(trace_synchronous, "trace_synchronous", K4, NULL, TIMED, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(trace_json, "json_task", K6, NULL, BACKGROUND, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(WiFi_tcp_server_task, "WiFi_tcp_server", K4, NULL, NETWORK, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);
  // xTaskCreate(tcpip_accept_poll, "tcpip_accept_poll", K4, NULL, POLLING, NULL);
  serial_flush(ALL);
  vTaskDelay(TICK_10ms);

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "SN:%d Running\r\n", json_serial_number);))
  vTaskDelay(TICK_10ms);
  serial_flush(ALL);
}
