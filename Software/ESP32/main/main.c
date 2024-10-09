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

#include "freETarget.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"
#include "wifi.h"
#include "diag_tools.h"

/*
 * Task Priorities
 */
#define BACKGROUND 0                         // Lowest priority task,  background
#define POLLING    (BACKGROUND + 4)          // Intermittent communications polling
#define NETWORK    (POLLING + 4)             // Intermittent network polling
#define TIMED      (NETWORK + 4)             // Scheduled tasks
#define MUST_RUN   (TIMED + 4)               // This task must run
#if ( MUST_RUN >= configMAX_PRIORITIES)
#error MUST_RUN set too high 
#endif

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
   xTaskCreate(freeETarget_target_loop, "freeETarget_target_loop",   4096, NULL, MUST_RUN, NULL);
   vTaskDelay(1);

   xTaskCreate(freeETarget_timers,      "freeETarget_timer",         4096, NULL, TIMED, NULL);
   vTaskDelay(1);

   xTaskCreate(freeETarget_synchronous, "freeETarget_synchronous",   4096, NULL, TIMED, NULL);
   vTaskDelay(1);

   xTaskCreate(freeETarget_json,        "json_task",                 4096, NULL, BACKGROUND, NULL);
   vTaskDelay(1);

   xTaskCreate(WiFi_tcp_server_task,    "WiFi_tcp_server",           4096, NULL,  NETWORK, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_accept_poll,       "tcpip_accept_poll",         4096, NULL,  POLLING, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_0,     "tcpip_socket_poll_0",       4096, NULL,  POLLING, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_1,     "tcpip_socket_poll_1",       4096, NULL,  POLLING, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_2,     "tcpip_socket_poll_2",       4096, NULL,  POLLING, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_3,     "tcpip_socket_poll_3",       4096, NULL,  POLLING, NULL);
   vTaskDelay(1);

   freeETarget_timer_init();

   DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Running\r\n");))
}
