/******************************************************************************
 *
 * http_test
 *
 * Test modules for HTTP operation
 *
 ******************************************************************************
 *

 *
 * *****************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_http_server.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "serial_io.h"
#include "freETarget.h"
#include "http_client.h"
#include "WiFi.h"
#include "compute_hit.h"
#include "diag_tools.h"
#include "http_client.h"
#include "http_server.h"
#include "json.h"
#include "nonvol.h"

/*
 * Private Functions
 */

/*
 * Definitions
 */
#define TO_IP(x) ((int)x) & 0xff, ((int)x >> 8) & 0xff, ((int)x >> 16) & 0xff, ((int)x >> 24) & 0xff

#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
/*****************************************************************************
 *
 * @function: http_DNS_test
 *
 * @brief:    Use the DNS sofware to find an IP address
 *
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * This issues a DNS request for google.com and prints the reply
 *
 *******************************************************************************/
static char test_URL[128];

void http_DNS_test(void)
{
  char str_c[128];

  printf("%s ", json_remote_url);
  if ( json_remote_url[0] != 0 )
  {
    strcpy(test_URL, json_remote_url);
  }
  else
  {
    strcpy(test_URL, "google.com");
  }

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "http_DNS_test(%s)", test_URL);))

  /*
   * Make sure we ares setup correctly
   */
  if ( json_wifi_ssid[0] == 0 )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "\r\nWiFi must be attached to gateway");))
    return;
  }

  /*
   *  Go look for the remote address
   */
  if ( WiFi_get_remote_IP(test_URL) == 0 )
  {
    SEND(ALL, sprintf(_xs, "DNS lookup failed");)
  }
  else
  {
    WiFi_remote_IP_address(&str_c);
    SEND(ALL, sprintf(_xs, "\r\nThe IP address of %s is %s", test_URL, str_c);)
  }

  /*
   * Exit the test
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*****************************************************************************
 *
 * @function: http_send_to_server_test
 *
 * @brief:    Send a payload to a server
 *
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * This sends a payload to the remote server
 *
 * Test Vectors
 *
 * {"REMOTE_URL":"http://freetarget:80"}
 * {"REMOTE_URL":"http://192.168.86.82"}
 * {"REMOTE_URL":"http://google.com"}
 * {"REMOTE_URL":"google.com"}
 *
 *******************************************************************************/
static char test_payload[] = "Hello World";

void http_send_to_server_test(void)
{
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, " http_send_to_server_test(%s)", test_payload);))

  /*
   * Make sure we ares setup correctly
   */
  if ( json_wifi_ssid[0] == 0 )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "\r\nWiFi should be attached to gateway");))
  }

  /*
   *  Send the payload
   */
  http_rest_with_url(json_remote_url, METHOD_PUT, test_payload);

  /*
   * Exit the test
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, _DONE_);))
  return;
}

/*****************************************************************************
 *
 * @function: http_server_test
 *
 * @brief:    Turn on the server and wait for requests
 *
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * Initialize the server and then wait for stuff to come in
 *
 *******************************************************************************/
void http_server_test(void)
{
  static void *server = NULL;

  /*
   * Start the server for the first time
   */
  server = start_webserver(DEFAULT_HTTP_PORT);

  /*
   * Stay here until someone removes the server
   */
  while ( server )
  {
    vTaskDelay(ONE_SECOND / 4);
  }
}

#endif
