/*******************************************************************************
 *
 * main.c
 *
 * FreeETarget control loop
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

#include "freETarget.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"
#include "wifi.h"
#include "diag_tools.h"
#include "http_client.h"
#include "http_server.h"
#include "http_services.h"

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
   *  Start FreeETarget
   */
  freeETarget_init();

  /*
   * Everything is ready, start the threads.  Low task priority number == low priority
   */
  xTaskCreate(freeETarget_target_loop, "freeETarget_target_loop", K4, NULL, MUST_RUN, NULL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(freeETarget_timers, "freeETarget_timer", K4, NULL, TIMED, NULL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(freeETarget_synchronous, "freeETarget_synchronous", K4, NULL, TIMED, NULL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(freeETarget_json, "json_task", K6, NULL, BACKGROUND, NULL);
  vTaskDelay(TICK_10ms);

  xTaskCreate(WiFi_tcp_server_task, "WiFi_tcp_server", K4, NULL, NETWORK, NULL);
  vTaskDelay(TICK_10ms);
  xTaskCreate(tcpip_accept_poll, "tcpip_accept_poll", K4, NULL, POLLING, NULL);
  vTaskDelay(TICK_10ms);
  xTaskCreate(tcpip_socket_poll_0, "tcpip_socket_poll_0", K4, NULL, POLLING, NULL);
  vTaskDelay(TICK_10ms);
  xTaskCreate(tcpip_socket_poll_1, "tcpip_socket_poll_1", K4, NULL, POLLING, NULL);
  vTaskDelay(TICK_10ms);
  xTaskCreate(tcpip_socket_poll_2, "tcpip_socket_poll_2", K4, NULL, POLLING, NULL);
  vTaskDelay(TICK_10ms);
  xTaskCreate(tcpip_socket_poll_3, "tcpip_socket_poll_3", K4, NULL, POLLING, NULL);
  vTaskDelay(TICK_10ms);

  start_webserver(DEFAULT_HTTP_PORT); // Main port for the web server
  vTaskDelay(TICK_10ms);
  start_webserver(EVENT_HTTP_PORT);   // Control port for the web server
  vTaskDelay(TICK_10ms);

  freeETarget_timer_init();

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Running\r\n");))
}
