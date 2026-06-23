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
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/lwip.html
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

#include "freETarget.h"
#include "helpers.h"
#include "http_client.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"
#include "timer.h"
#include "tcpip_helpers.h"

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
 * Typdef
 */

/*
 * Variables
 */
static wifi_config_t                WiFi_config;
static EventGroupHandle_t           s_wifi_event_group;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static int                          s_retry_num = 0;
static esp_netif_ip_info_t          ipInfo;                           // IP Address of the access point
static esp_netif_t                 *sta_netif;                        // Station configuration
static socket_description_t         socket_list[MAX_SOCKETS]; // Space to remember four sockets

/*
 * Private Functions
 */
void      WiFi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t esp_base_mac_addr_get(uint8_t *mac);

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
  esp_wifi_set_ps(WIFI_PS_NONE);

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
 * @function: WiFi_tcp_server_task()
 *
 * @brief: Synchorous task to manage the TCPIP Stack
 *
 * @return: Never
 *
 ******************************************************************************
 *
 * Synchronous task called from freeRTOS to interrogate the TCPIP stack and
 * accept calls from clients.
 *
 * Once a socket has been connected, the input and outut queues are managed
 * to send and receive data
 *
 *******************************************************************************/
#define TCP_SERVER_PERIOD TICK_10ms
void WiFi_tcp_server_task(void *pvParameters)
{
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "WiFi_tcp_server_task(%d)", TCP_SERVER_PERIOD);))

  /*
   *  Move data in and out of the TCP queues
   */
  while ( 1 )
  {
    tcpip_server_io();
    /*
     *  Time out till the next time
     */
    vTaskDelay(TCP_SERVER_PERIOD);
  }
}

/*****************************************************************************
 *
 * @function: tcpip_server_io()
 *
 * @brief: Transmit data in and out of the target
 *
 * @return: None
 *
 ******************************************************************************
 *
 * If there is any data in the tcpip_queue_2_socket queue, this function
 * extracts the data and then loops through the active sockets to put the data
 * out to the client.
 *
 * When trying to send to a previously active socket which is now closed, the
 * SEND(ALL,) function will return a -1 to indicate that no information was sent.
 * This is the signal that the connection has been dropped.  At the end of the
 * loop, if all of the sockets have been closed the connection indication is
 * updated.
 *
 *******************************************************************************/
static void tcpip_server_io(void)
{
  int  length;
  char rx_buffer[128];
  int  to_send;
  int  i;
  int  buffer_offset;
  bool new_socket_closed;

  new_socket_closed = false; // Was a socket closed this cycle?
                             /*
                              * Out to TCPIP
                              */
  to_send = tcpip_queue_2_socket(rx_buffer, sizeof(rx_buffer));
  if ( to_send > 0 )
  {
    for ( i = 0; i != MAX_SOCKETS; i++ )
    {
      if ( socket_list[i].handle > 0 )
      {
        buffer_offset = 0;
        while ( buffer_offset < to_send )
        {
          length = send(socket_list[i].handle, rx_buffer + buffer_offset, to_send - buffer_offset, 0);
          if ( length < 0 )
          {
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "TCPIP send socket closed to %s ", socket_list[i].ip);))
            close(socket_list[i].handle);
            socket_list[i].handle = AVAILABLE_SOCKET;
            socket_list[i].ip[0]  = 0;
            new_socket_closed     = true;
            break;
          }
          buffer_offset += length;
          send(socket_list[i].handle, NULL, 0, 0); // Flush the transmit queue
        }
      }
    }
  }
  /*
   *  See if all of the sockets are closed
   */
  if ( new_socket_closed )
  {
    for ( i = 0; i != MAX_SOCKETS; i++ )
    {
      if ( socket_list[i].handle > 0 )
      {
        break;
      }
    }

    if ( i == MAX_SOCKETS )         // All of them are closed?
    {
      if ( json_wifi_ssid[0] != 0 ) //  I'm a station
      {
        set_status_LED(LED_WIFI_STATION);
      }
      else                          // I'm an access point
      {
        set_status_LED(LED_WIFI_ACCESS);
      }
    }
  }
  /*
   *  All done
   */
  return;
}

/*****************************************************************************
 *
 * @function: tcpip_socket_poll()  0-3
 *
 * @brief:    Tasks to poll the sockets
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * There are four identical functions that wait for a TCPIP message to arrive.
 * On receipt of the message, the bytes are copied from TCPIP into a circular
 * queue that holds everything until needed.
 *
 * IMPORTANT
 *
 * The recv function is a blocking call that waits for something to
 * appear in the TCPIP channel. Meaning that these functions will be suspended
 * indefinitly until something arrives and then be woken up until the next
 * recv() call is made.  For this reason, the functions are separated out, one
 * for each possible socket.
 *
 *******************************************************************************/
void tcpip_socket_poll_0(void *parameters)
{
  int  length;
  char rx_buffer[256];

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_socket_poll_0()");))

  while ( 1 )
  {
    if ( socket_list[0].handle > 0 )
    {
      length = recv(socket_list[0].handle, rx_buffer, sizeof(rx_buffer), 0);
      if ( length > 0 )
      {
        tcpip_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

void tcpip_socket_poll_1(void *parameters)
{
  int  length;
  char rx_buffer[256];

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_socket_poll_1()");))

  while ( 1 )
  {
    if ( socket_list[1].handle > 0 )
    {
      length = recv(socket_list[1].handle, rx_buffer, sizeof(rx_buffer), 0);
      if ( length > 0 )
      {
        tcpip_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

void tcpip_socket_poll_2(void *parameters)
{
  int  length;
  char rx_buffer[256];

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_socket_poll_2()");))

  while ( 1 )
  {
    if ( socket_list[2].handle > 0 )
    {
      length = recv(socket_list[2].handle, rx_buffer, sizeof(rx_buffer), 0);
      if ( length > 0 )
      {
        tcpip_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

void tcpip_socket_poll_3(void *parameters)
{
  int  length;
  char rx_buffer[256];

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_socket_poll_3()");))

  while ( 1 )
  {
    if ( socket_list[3].handle > 0 )
    {
      length = recv(socket_list[3].handle, rx_buffer, sizeof(rx_buffer), 0);
      if ( length > 0 )
      {
        tcpip_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

/*****************************************************************************
 *
 * @function: tcpip_accept_poll()
 *
 * @brief:    Tasks to poll waiting for an incoming connection
 *
 * @return:   None
 *
 ******************************************************************************
 *
 * Once the WiFi has been set up, the target waits here for an incoming
 * connection.
 *
 * Once the connection has been made, the function determines the socket address
 * and then looks for an empty entry in the socket list. The new socket is now
 * added to the socket list and polled via the functions above.
 *
 * How it works.
 *
 * This function sets up a socket. When a connection is made, a link is made
 * between the incoming request and one of four available sockets.
 *
 * From this point on, the incoming or outgoing data is linked to the sockt
 *
 *******************************************************************************/
void tcpip_accept_poll(void *parameters)
{
  char                    addr_str[128];
  int                     ip_protocol  = 0;
  int                     keepAlive    = 1;
  int                     keepIdle     = KEEPALIVE_IDLE;
  int                     keepInterval = KEEPALIVE_INTERVAL;
  int                     keepCount    = KEEPALIVE_COUNT;
  struct sockaddr_storage dest_addr;
  int                     listen_sock;
  int                     option = 1;
  struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
  socklen_t               addr_len = sizeof(source_addr);
  int                     sock;
  int                     i, j;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_accept_poll()");))

  /*
   * Start the server
   */
  for ( i = 0; i != MAX_SOCKETS; i++ )
  {
    socket_list[i].handle = AVAILABLE_SOCKET; // Make it available
    socket_list[i].ip[0]  = 0;                // No IP yet
  }

  struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
  dest_addr_ip4->sin_addr.s_addr    = htonl(INADDR_ANY);
  dest_addr_ip4->sin_family         = AF_INET;
  dest_addr_ip4->sin_port           = htons(PORT);
  ip_protocol                       = IPPROTO_IP;

  listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol); // WE need to have a socket
  if ( listen_sock < 0 )
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Unable to create socket: errno %d\r\n", errno);))
    vTaskDelete(NULL);                                     // No sockets?  Cannot run
    return;
  }

  option = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  listen(listen_sock, 1);

  /*
   *  Wait here for a socket to be requested
   */
  while ( 1 )
  {
    sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len); // This is a blocking call
    if ( sock > 0 )                                                         // Unblocked
    {
      for ( i = 0; i != MAX_SOCKETS; i++ )
      {
        if ( socket_list[i].handle == AVAILABLE_SOCKET )                    // FInd an available socket
        {
          socket_list[i].handle = sock;                                     //  and connect to it.
          WiFi_start_new_connection(sock);
          break;
        }
      }

      /*
       * Got a socket, and we have enough room to add it
       */
      if ( i != MAX_SOCKETS )
      {
        /*
         *  Is this a duplicate?  ie, the socket dropped and we don't know about it
         */
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        for ( j = 0; j != MAX_SOCKETS; j++ )
        {
          if ( strcmp(&socket_list[j].ip[0], addr_str) == 0 ) // Do we have duplicate?
          {
            lwip_close(socket_list[j].handle);                // Close this connection
            socket_list[j].handle = AVAILABLE_SOCKET;         // Free up the connection
            socket_list[j].ip[0]  = 0;                        // Forget the IP
            DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Duplicate socket connection %d from %s", j, addr_str);))
          }
        }
        strcpy(&socket_list[i].ip[0], addr_str);              // Remember this IP address

        /*
         * Set tcp keepalive option
         */
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        DLT(DLT_INFO, { SEND(ALL, sprintf(_xs, "Socket accepted ip address: %s\r\n", addr_str);) })
        set_status_LED(LED_WIFI_STATION_CN);
      }
    }
  }

  /*
   *  Never get here
   */
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_start_new_connection
 *
 * @brief:    Prepare a new connection
 *
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * A new socket connection has been made.
 *
 * Once that has been done, then update the PC client with all of  the pending
 * scores.
 *
 *******************************************************************************/
static void WiFi_start_new_connection(int sock) // Socket token to use
{
  int  i;
  char str[SHORT_TEXT];

  /*
   *  Build up a mask of existing WiFi connections
   */
  connection_list &= ~(TCPIP);
  for ( i = 0; i != MAX_SOCKETS; i++ ) // How many connections do we have?
  {
    if ( socket_list[i].handle != AVAILABLE_SOCKET )
    {
      connection_list = (TCPIP_0) << i;
    }
  }

  /*
   *  Inform the PC what is going on
   */
  target_name(str);
  SEND(ALL, sprintf(_xs, "{\"%s\":%ld, \"NAME\":\"%s\"}", _GREETING_, run_time_seconds(), str);)

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    if ( (record[i].session_type & SESSION_VALID) != 0 )
    {
      send_replay(&record[i], i);
      send(sock, _xs, strlen(_xs), 0);
    }
  }

  /*
   *  All done, return
   */
  return;
}
