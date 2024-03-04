/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freETarget.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"
#include "wifi.h"
#include "diag_tools.h"

void app_main(void)
{

/*
 *  Start FreeETarget
 */
    freeETarget_init();

/*
 * Everything is ready, start the threads.  Low task priority number == low priority
 */
   xTaskCreate(freeETarget_target_loop, "freeETarget_target_loop",   4096, NULL, 25, NULL);
   vTaskDelay(1);

   xTaskCreate(freeETarget_synchronous, "freeETarget_synchronous",   4096, NULL, 20, NULL);
   vTaskDelay(1);

   xTaskCreate(freeETarget_json,        "json_task",                 4096, NULL, 15, NULL);
   vTaskDelay(1);

   xTaskCreate(tcpip_socket_poll_0,     "tcpip_socket_poll_0",       4096, NULL,  5, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_1,     "tcpip_socket_poll_1",       4096, NULL,  5, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_2,     "tcpip_socket_poll_2",       4096, NULL,  5, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_socket_poll_3,     "tcpip_socket_poll_3",       4096, NULL,  5, NULL);
   vTaskDelay(1);

   xTaskCreate(WiFi_tcp_server_task,    "WiFi_tcp_server",           4096, NULL,  5, NULL);
   vTaskDelay(1);
   xTaskCreate(tcpip_accept_poll,       "tcpip_accept_poll",         4096, NULL,  2, NULL);
   vTaskDelay(1);

   freeETarget_timer_init();

   DLT(DLT_CRITICAL, printf("Running\r\n");)
}
