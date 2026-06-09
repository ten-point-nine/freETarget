/******************************************************************************
 *
 * WiFi.c
 *
 * WiFi Driver for FreeETarget
 *
 ******************************************************************************
 *
 * The WiFi driver supposts two modes of operation:
 *       1 - Access Point (AP) where the target provides the SSID
 *       2 - Station Mode (STA) where the target talks to a router with an SSID
 *
 * The two modes are differenciated when the SSID stored in the configuration
 * is defined (STA) or empty (AP).
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html
 * https://medium.com/@fatehsali517/how-to-connect-esp32-to-wifi-using-esp-idf-iot-development-framework-d798dc89f0d6
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/lwip.html
 *
 * MDNS documentation
 * https://docs.espressif.com/projects/esp-protocols/mdns/docs/latest/en/index.html
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

#include "trace.h"
#include "helpers.h"
#include "http_client.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"
#include "timer.h"

#define DEFAULT_IP         192, 168, 10, 9
#define PORT               1090
#define KEEPALIVE_IDLE     true
#define KEEPALIVE_INTERVAL 100
#define KEEPALIVE_COUNT    50
#define MAX_SOCKETS        4  // Allow for four sockets
#define AVAILABLE_SOCKET   -1 // The socket is unused

/*
 * Macros
 */
#define WIFI_CONNECTED_BIT                BIT0 // we are connected to the AP with an IP
#define WIFI_FAIL_BIT                     BIT1 // we failed to connect after the maximum amount of retries */
#define WIFI_MAX_RETRY                    3    // Try 3x
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

/*
 * Variables
 */
static wifi_config_t                WiFi_config;
static EventGroupHandle_t           s_wifi_event_group;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static int                          s_retry_num = 0;
static esp_netif_ip_info_t          ipInfo;        // IP Address of the access point
static esp_netif_t                 *sta_netif;     // Station configuration
static int                          client_socket; // Socket used to talk to the target

/*
 * Private Functions
 */
void      WiFi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t esp_base_mac_addr_get(uint8_t *mac);

/*
 * Definitions
 */
/*****************************************************************************
 *
 * @function: WiFi_reconnect()
 *
 * @brief:    Try and make a new WiFi connection
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * This function stops and de-initializes the current WiFi interface.
 *
 *******************************************************************************/
void WiFi_reconnect(void)
{
  char str_c[SHORT_TEXT];
  esp_wifi_stop();
  esp_wifi_start(); // Start the WiFi

  /*
   * Wait here for an event to occur
   */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  /*
   *  The target has connected to an access point
   */
  if ( bits & WIFI_CONNECTED_BIT )
  {
    WiFi_my_IP_address(str_c);
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Connected to AP SSID:  \"%s\"", json_wifi_ssid);))
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Using WiFi_IP_ADDRESS: \"%s\"", str_c);))
  }
  else if ( bits & WIFI_FAIL_BIT )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to connect to SSID:%s, password:%s", json_wifi_ssid, json_wifi_pwd);))
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Unexpectged WiFi event");))
  }
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_station_init()
 *
 * @brief:    Initialize the WiFi Interface as a station
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * This function initializes the WiFi to act as an station
 *
 * The target connects to an SSID and lets clients connect to it. For example
 * SSID = myHomeInternet
 *
 * See:
 *https://github.com/espressif/esp-idf/blob/v4.3/examples/wifi/getting_started/station/main/station_example_main.c
 *
 * Test
 *
 *
 *******************************************************************************/
void WiFi_station_init(void)
{
  char str_c[256];

  wifi_init_config_t WiFi_init_config = WIFI_INIT_CONFIG_DEFAULT();

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "WiFi_station_init()");))

  s_wifi_event_group = xEventGroupCreate();
  esp_netif_init();

  esp_event_loop_create_default();
  sta_netif = esp_netif_create_default_wifi_sta();

  esp_wifi_init(&WiFi_init_config); // Initialize the configuration
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, sta_netif, &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler, NULL, &instance_got_ip);

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "WiFi SSID: %s", json_wifi_ssid);))
  strcpy((char *)&WiFi_config.sta.ssid, json_wifi_ssid);
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "WiFi password: %s\r\n", json_wifi_pwd);))
  strcpy((char *)&WiFi_config.sta.password, json_wifi_pwd);

  if ( json_wifi_pwd[0] == 0 )
  {
    WiFi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
  }
  else
  {
    WiFi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
  }
  WiFi_config.sta.pmf_cfg.capable  = true;
  WiFi_config.sta.pmf_cfg.required = false;
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &WiFi_config);
  esp_wifi_start(); // Start the WiFi

  /*
   * Wait here for an event to occur
   */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  /*
   *  The target has connected to an access point
   */
  if ( bits & WIFI_CONNECTED_BIT )
  {
    WiFi_my_IP_address(str_c);
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Connected to AP SSID:  \"%s\"", json_wifi_ssid);))
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Using WiFi_IP_ADDRESS: \"%s\"", str_c);))
  }
  else if ( bits & WIFI_FAIL_BIT )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Failed to connect to SSID:%s, password:%s", json_wifi_ssid, json_wifi_pwd);))
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Unexpectged WiFi event");))
  }

  /*
   *  All done
   */
  return;
}

/*****************************************************************************
 *
 * @function:WiFi_event_handler
 *
 * @brief:   Manage events coming from the FreeRTOS event handler
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * The initialization determines if the target is a station
 * or an access point (AP) that provides the SSID to connect to.
 *
 * Once that is done the appropriate configuration is made and the target
 *enabled.
 *
 * IMPORTANT
 *
 * This function only relates to the WiFi connecting to the SSID.
 *
 *******************************************************************************/
void WiFi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

  /*
   * I am a station
   */
  if ( event_base == WIFI_EVENT )
  {
    if ( event_id == WIFI_EVENT_STA_START )        // Begin a connection to the SSID
    {
      esp_wifi_connect();
    }

    if ( event_id == WIFI_EVENT_STA_DISCONNECTED ) // End a connection to the SSID
    {
      if ( s_retry_num < WIFI_MAX_RETRY )
      {
        esp_wifi_connect();
        s_retry_num++;
      }
      else
      {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      }
    }
  }

  if ( event_base == IP_EVENT )
  {
    if ( event_id == IP_EVENT_STA_GOT_IP )
    {
      ipInfo.ip = event->ip_info.ip;
      DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Received IP: " IPSTR, IP2STR(&event->ip_info.ip));))
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }
  /*
   * I am an access point
   */
  if ( event_id == WIFI_EVENT_AP_STACONNECTED )
  {
    DLT(DLT_COMMUNICATION, SEND(CONSOLE, sprintf(_xs, "AP connected");))
  }

  if ( event_id == WIFI_EVENT_AP_STADISCONNECTED )
  {
    DLT(DLT_COMMUNICATION, SEND(CONSOLE, sprintf(_xs, "AP disconnected");))
  }

  /*
   * All done, return
   */
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_my_IP_address()
 *
 * @brief:    Return the IP address as a string
 *
 * @return:   TRUE if there is a valid IP address
 *
 ****************************************************************************/
#define TO_IP(x) ((int)x) & 0xff, ((int)x >> 8) & 0xff, ((int)x >> 16) & 0xff, ((int)x >> 24) & 0xff
bool WiFi_my_IP_address(char *s // Where to return the string
)
{
  sprintf(s, "%d.%d.%d.%d", TO_IP(ipInfo.ip.addr));

  if ( ipInfo.ip.addr == 0 )
    return false;

  return true;
}

#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
void WiFi_remote_IP_address(char *s // Where to return the string
)
{
  sprintf(s, "%d.%d.%d.%d", TO_IP(url_ip_address.u_addr.ip4.addr));
  return;
}
#endif

/*****************************************************************************
 *
 * @function: WiFi_MAC_address()
 *
 * @brief:    Return the MAC address as an array of bytes
 *
 * @return:   None
 *
 ****************************************************************************/
void WiFi_MAC_address(char *mac // Where to return the string
)
{
  esp_base_mac_addr_get((uint8_t *)mac);
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_tests
 *
 * @brief:    A varity of WiFi tests
 *
 * @return:   None
 *
 ****************************************************************************/

/*****************************************************************************
 *
 * @function: WiFi_AP_scan_test
 *
 * @brief:    Find the APs in the area
 *
 * @return:   None
 *
 ****************************************************************************
 *
 * The function looks for APs that it can find and reports the SSID and
 * the signal strenght.
 *
 * RSSIs closer to 0 are better
 * **************************************************************************/

void WiFi_AP_scan_test(void)
{
  wifi_ap_record_t ap_info[20]; // Space for 20 APs
  uint16_t         ap_count = 20;
  int              i;
  char             str_c[SHORT_TEXT];

  SEND(CONSOLE, sprintf(_xs, "\r\nCloser to 0 is stronger");)

  if ( esp_wifi_scan_start(NULL, true) != ESP_OK )
  {
    SEND(CONSOLE, sprintf(_xs, "\r\nFailed to scan for APs");)
    return;
  }

  if ( esp_wifi_scan_get_ap_records(&ap_count, ap_info) != ESP_OK )
  {
    SEND(CONSOLE, sprintf(_xs, "\r\nFailed to get AP records");)
    return;
  }

  for ( i = 0; i < ap_count; i++ )
  {
    SEND(CONSOLE, sprintf(_xs, "\r\nAP[%d] SSID: %s,  RSSI:%d,  ", i, ap_info[i].ssid, ap_info[i].rssi);)

    switch ( ap_info[i].authmode )
    {
      case WIFI_AUTH_OPEN:
        SEND(CONSOLE, sprintf(_xs, " Auth: Open");) break;
      case WIFI_AUTH_WEP:
        SEND(CONSOLE, sprintf(_xs, " Auth: WEP");) break;
      case WIFI_AUTH_WPA_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA_PSK");) break;
      case WIFI_AUTH_WPA2_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA2_PSK");) break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA_WPA2_PSK");) break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA2_ENTERPRISE");) break;
      case WIFI_AUTH_WPA3_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA3_PSK");) break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WPA2_WPA3_PSK");) break;
      case WIFI_AUTH_WAPI_PSK:
        SEND(CONSOLE, sprintf(_xs, " Auth: WAPI_PSK");) break;
      case WIFI_AUTH_MAX:
        SEND(CONSOLE, sprintf(_xs, " Auth: MAX");) break;

      default:
        SEND(CONSOLE, sprintf(_xs, " Auth: Unknown");) break;
    }

    SEND(CONSOLE, sprintf(_xs, "     {\"WIFI_SSID\":\"%s\", \"WIFI_PWD\":\"---\"}", ap_info[i].ssid);)
  }

  /*
   * All done, return
   */

  WiFi_my_IP_address(str_c);
  SEND(CONSOLE, sprintf(_xs, "\r\n\r\nConnected to AP SSID:  \"%s\"", json_wifi_ssid);)
  SEND(CONSOLE, sprintf(_xs, "   IP: \"%s\"", str_c);)
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_client_init
 *
 * @brief:    Start a client connection
 *
 * @return:   TRUE if the connection is succesful
 *
 ****************************************************************************
 *
 * Create a connection to the target
 *
 ***************************************************************************/
bool WiFi_client_init(void)
{
  struct sockaddr_in dest_addr;

  /*
   *  Check to see if we are already connected
   */
  IF(TARGET_CONNECTED)
  {
    return true;
  }

  /*
   *  Not connected, then connect
   */
  memset((void *)&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_len         = sizeof(dest_addr);
  dest_addr.sin_addr.s_addr = inet_addr(json_wifi_target_ip);
  dest_addr.sin_family      = AF_INET;
  dest_addr.sin_port        = lwip_htons(1090);

  /*
   *   Create the socket
   */
  client_socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if ( client_socket < 0 )
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Unable to create socket: errno %d", errno);))
    return false;
  }

  /*
   * Make the connection
   */
  if ( lwip_connect(client_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0 )
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Socket unable to connect: errno %d", errno);))
    close(client_socket);
    return false;
  }

  /*
   *  Got here, ready to go
   */
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Connected to %s:%d", json_wifi_target_ip, 1090);))
  run_state |= TARGET_CONNECTED; // Yay, we're connected
  return true;
}

/*****************************************************************************
 *
 * @function: WiFi_client_send
 *
 * @brief:    Send a payload to the target
 *
 * @return:   TRUE if the connection is succesful
 *
 ****************************************************************************
 *
 *  Take the message and send it out the TCPIP port
 *
 *
 ***************************************************************************/
void WiFi_client_send(void) //
{
  char s[MEDIUM_TEXT];
  int  length;              // Number f bytes to send

  length = tcpip_app_2_queue(s, sizeof(s));
  if ( length > 0 )
  {
    send(client_socket, s, length, 0);
  }

  return;
}

/*****************************************************************************
 *
 * @function: WiFi_client_send
 *
 * @brief:    Send a payload to the target
 *
 * @return:   TRUE if the connection is succesful
 *
 ****************************************************************************
 *
 *  Take the message and send it out the TCPIP port
 *
 *
 ***************************************************************************/
void WiFi_client_get(void)
{
  char s[MEDIUM_TEXT];
  int  rx_length; // Number of characters received

  /*
   *  Receive the data
   */
  rx_length = recv(client_socket, s, sizeof(s), 0);
  if ( rx_length < 0 )
  {
    run_state &= ~TARGET_CONNECTED;
    return;
  }

  tcpip_socket_2_queue(s, rx_length);

  return;
}