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
static int                          socket_list[MAX_SOCKETS]; // Space to remember four sockets
static esp_netif_ip_info_t          ipInfo;                   // IP Address of the access point
static int                          dns_valid;                // We have a valid IP address for the URL
static ip_addr_t                    url_ip_address;           // Address of the server
static esp_netif_t                 *sta_netif;                // Station configuration
static bool                         WiFi_initialized = false;

/*
 * Private Functions
 */
void        WiFi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void tcpip_server_io(void);                  // Manage TCPIP traffic
static void dns_found_cb(const char *name, const ip_addr_t *ip_addr, void *callback_arg);
esp_err_t   esp_base_mac_addr_get(uint8_t *mac);
static void WiFi_start_new_connection(int sock);    // Socket token to use
static void wifi_set_static_ip(esp_netif_t *netif); // Override the IP address

/*
 * Definitions
 */
#define TO_IP(x) ((int)x) & 0xff, ((int)x >> 8) & 0xff, ((int)x >> 16) & 0xff, ((int)x >> 24) & 0xff

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
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Connected to AP SSID:  \"%s\"", json_wifi_ssid);))
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Using WiFi_IP_ADDRESS: \"%s\"", str_c);))
  }
  else if ( bits & WIFI_FAIL_BIT )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to connect to SSID:%s, password:%s", json_wifi_ssid, json_wifi_pwd);))
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Unexpectged WiFi event");))
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

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "WiFi_station_init()");))

  s_wifi_event_group = xEventGroupCreate();
  esp_netif_init();

  esp_event_loop_create_default();
  sta_netif = esp_netif_create_default_wifi_sta();

  esp_wifi_init(&WiFi_init_config); // Initialize the configuration
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, sta_netif, &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler, NULL, &instance_got_ip);

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "WiFi SSID: %s", json_wifi_ssid);))
  strcpy((char *)&WiFi_config.sta.ssid, json_wifi_ssid);
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "WiFi password: %s", json_wifi_pwd);))
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
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Connected to AP SSID:  \"%s\"", json_wifi_ssid);))
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Using WiFi_IP_ADDRESS: \"%s\"", str_c);))
  }
  else if ( bits & WIFI_FAIL_BIT )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to connect to SSID:%s, password:%s", json_wifi_ssid, json_wifi_pwd);))
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Unexpectged WiFi event");))
  }

  /*
   *  All done
   */
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_get_remote_IP()
 *
 * @brief:    Find the address of the remote IP
 *
 * @return:   TRUE if the dns has been found
 *
 ****************************************************************************
 *
 * This function calls the DNS server to obtain the IP address of a URL.
 *
 * Example "google.com" is 142.250.190.14
 *
 * See https://gist.github.com/MakerAsia/37d2659310484bdbba9d38558e2c3cdb
 * for programming example
 *
 * See https://www.nongnu.org/lwip/2_0_x/group__infrastructure__errors.html
 * for LWIP errors
 *
 ****************************************************************************/
bool WiFi_get_remote_IP(char *remote_url) // Text string of the remote URL
{
  int i;
  /*
   * Prepare the callback for the result
   */
  if ( dns_gethostbyname(remote_url, &url_ip_address, dns_found_cb, NULL) == 0 )
  {
    dns_valid = 1; // IP was cached and available
  }
  else
  {
    dns_valid = 0; // IP is not currently valid
  }

  /*
   * Wait here for the DNS to come back
   */
  i = DNS_TRIES;
  while ( (dns_valid == 0) && (i != 0) )
  {
    vTaskDelay(0.1 * ONE_SECOND);
    i--;
  }

  /*
   *  Return the DNS state
   */
  if ( dns_valid == 0 )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "URL %s not found", remote_url);))
  }
  return dns_valid;
}

static void dns_found_cb(const char      *name,        // Name of dns search
                         const ip_addr_t *ip_addr,     // IP address of the URL
                         void            *callback_arg // Not used
)
{
  url_ip_address = *ip_addr;
  dns_valid      = true;

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
    if ( event_id == WIFI_EVENT_STA_START ) // Begin a connection to the SSID
    {
      esp_wifi_connect();
    }
    if ( event_id == WIFI_EVENT_STA_CONNECTED )
    {
      wifi_set_static_ip(arg);
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
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Received IP: " IPSTR, IP2STR(&event->ip_info.ip));))
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }
  /*
   * I am an access point
   */
  if ( event_id == WIFI_EVENT_AP_STACONNECTED )
  {
    DLT(DLT_COMMUNICATION, SEND(ALL, sprintf(_xs, "AP connected");))
  }

  if ( event_id == WIFI_EVENT_AP_STADISCONNECTED )
  {
    DLT(DLT_COMMUNICATION, SEND(ALL, sprintf(_xs, "AP disconnected");))
  }

  /*
   * All done, return
   */
  return;
}

/*****************************************************************************
 *
 * @function:wifi_set_static_ip
 *
 * @brief:   Replace the DNS IP address with a static IP
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * This function is provided to users who override the dynamic IP address
 * assigned by the router.
 *
 *******************************************************************************/
static void wifi_set_static_ip(esp_netif_t *netif)
{
  esp_netif_ip_info_t ip; // New IP address

  /*
   * Check to see if a static IP has been assigned
   */
  if ( json_wifi_static_ip[0] == 0 )
  {
    return;
  }

  /*
   *  Yes, stop the current IP address from the DCHP server
   */
  if ( esp_netif_dhcpc_stop(netif) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to stop dhcp client");))
    return;
  }

  /*
   *  Write out a new one from the JSON configuration
   */
  memset(&ip, 0, sizeof(esp_netif_ip_info_t));
  ip.ip.addr      = ipaddr_addr(json_wifi_static_ip);
  ip.netmask.addr = ipaddr_addr("255.255.255.0");
  ip.gw.addr      = ipaddr_addr(json_wifi_gateway);

  if ( esp_netif_set_ip_info(netif, &ip) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to change IP address");))
    return;
  }

  /*
   *   Finished, return
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

  SEND(ALL, sprintf(_xs, "\r\nCloser to 0 is stronger");)

  if ( esp_wifi_scan_start(NULL, true) != ESP_OK )
  {
    SEND(ALL, sprintf(_xs, "\r\nFailed to scan for APs");)
    return;
  }

  if ( esp_wifi_scan_get_ap_records(&ap_count, ap_info) != ESP_OK )
  {
    SEND(ALL, sprintf(_xs, "\r\nFailed to get AP records");)
    return;
  }

  for ( i = 0; i < ap_count; i++ )
  {
    SEND(ALL, sprintf(_xs, "\r\nAP[%d] SSID: %s,  RSSI:%d,  ", i, ap_info[i].ssid, ap_info[i].rssi);)

    switch ( ap_info[i].authmode )
    {
      case WIFI_AUTH_OPEN:
        SEND(ALL, sprintf(_xs, " Auth: Open");) break;
      case WIFI_AUTH_WEP:
        SEND(ALL, sprintf(_xs, " Auth: WEP");) break;
      case WIFI_AUTH_WPA_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WPA_PSK");) break;
      case WIFI_AUTH_WPA2_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WPA2_PSK");) break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WPA_WPA2_PSK");) break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        SEND(ALL, sprintf(_xs, " Auth: WPA2_ENTERPRISE");) break;
      case WIFI_AUTH_WPA3_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WPA3_PSK");) break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WPA2_WPA3_PSK");) break;
      case WIFI_AUTH_WAPI_PSK:
        SEND(ALL, sprintf(_xs, " Auth: WAPI_PSK");) break;
      case WIFI_AUTH_MAX:
        SEND(ALL, sprintf(_xs, " Auth: MAX");) break;

      default:
        SEND(ALL, sprintf(_xs, " Auth: Unknown");) break;
    }

    SEND(ALL, sprintf(_xs, "     {\"WIFI_SSID\":\"%s\", \"WIFI_PWD\":\"---\"}", ap_info[i].ssid);)
  }

  /*
   * All done, return
   */

  WiFi_my_IP_address(str_c);
  SEND(ALL, sprintf(_xs, "\r\n\r\nConnected to AP SSID:  \"%s\"", json_wifi_ssid);)
  SEND(ALL, sprintf(_xs, "   IP: \"%s\"", str_c);)
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*
 *  New
 */

#include <string.h>
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASS"
#define SERVER_IP                                                                                                                          \
  "192.168.0.167" // Set to your server's IP [InlineCitation-3-esp-idf/examples/protocols/sockets/tcp_client/README.md at master ·
                  // espressif/esp-idf ·
                  // GitHub](https://github.com/espressif/esp-idf/blob/master/examples/protocols/sockets/tcp_client/README.md)
#define SERVER_PORT                                                                                                                        \
  3333            // Set to your TCP server port [InlineCitation-3-esp-idf/examples/protocols/sockets/tcp_client/README.md at master ·
       // espressif/esp-idf · GitHub](https://github.com/espressif/esp-idf/blob/master/examples/protocols/sockets/tcp_client/README.md)

static const char *TAG = "tcp_client";

/* Wi-Fi event handler */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START )
  {
    esp_wifi_connect();
  }
  else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED )
  {
    ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
    esp_wifi_connect();
  }
  else if ( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP )
  {
    ESP_LOGI(TAG, "Got IP address");
  }
}

/* Initialize Wi-Fi in station mode */
#if(0)
static void wifi_client_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

  wifi_config_t wifi_config = {
      .sta =
          {
                .ssid     = WIFI_SSID,
                .password = WIFI_PASS,
                },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}
#endif 

/* TCP client task */
static void tcp_client_task(void *pvParameters)
{
  char               rx_buffer[128];
  struct sockaddr_in dest_addr;

  dest_addr.sin_addr.s_addr = inet_addr(json_wifi_target_ip);
  dest_addr.sin_family      = AF_INET;
  dest_addr.sin_port        = 1090;

  while ( 1 )
  {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if ( sock < 0 )
    {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    ESP_LOGI(TAG, "Connecting to %s:%d", SERVER_IP, SERVER_PORT);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if ( err != 0 )
    {
      ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
      close(sock);
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    ESP_LOGI(TAG, "Successfully connected");

    int counter = 0;
    while ( 1 )
    {
      char message[64];
      snprintf(message, sizeof(message), "Hello from ESP32, msg #%d", ++counter);

      int err = send(sock, message, strlen(message), 0);
      if ( err < 0 )
      {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        break;
      }

      ESP_LOGI(TAG, "Sent: %s", message);

      int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
      if ( len < 0 )
      {
        ESP_LOGE(TAG, "Recv failed: errno %d", errno);
        break;
      }
      else if ( len == 0 )
      {
        ESP_LOGW(TAG, "Connection closed by server");
        break;
      }
      else
      {
        rx_buffer[len] = 0;
        ESP_LOGI(TAG, "Received: %s", rx_buffer);
      }

      vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "Shutting down and restarting");
    shutdown(sock, 0);
    close(sock);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

