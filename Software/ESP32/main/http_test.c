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
static char test_URL[] = "google.com";

void http_DNS_test(void)
{
  char str_c[32];

  DLT(DLT_INFO, SEND(sprintf(_xs, "http_DNS_test(%s)", test_URL);))

  /*
   * Make sure we ares setup correctly
   */
  if ( json_wifi_ssid[0] == 0 )
  {
    SEND(sprintf(_xs, "\r\nWiFi must be attached to gateway");)
    return;
  }

  /*
   *  Go look for the remote address
   */
  if ( WiFi_get_remote_IP(test_URL) == 0 )
  {
    SEND(sprintf(_xs, "DNS lookup failed");)
  }
  else
  {
    WiFi_remote_IP_address(&str_c);
    SEND(sprintf(_xs, "\r\nThe IP address of %s is %s", test_URL, str_c);)
  }

  /*
   * Exit the test
   */
  DLT(DLT_INFO, SEND(sprintf(_xs, _DONE_);))
  return;
}
#endif
