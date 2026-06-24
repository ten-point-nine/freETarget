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
#include "server.h"
#include "compute_hit.h"

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
static esp_netif_ip_info_t          ipInfo;                   // IP Address of the access point
static esp_netif_t                 *sta_netif;                // Station configuration
static socket_description_t         socket_list[MAX_SOCKETS]; // Space to remember four sockets

/*
 * Private Functions
 */
esp_err_t   esp_base_mac_addr_get(uint8_t *mac);
static void WiFi_start_new_connection(int sock); // Socket token to use

#if ( 0 )
#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
void WiFi_remote_IP_address(char *s              // Where to return the string
)
{
  url_ip_address = *ip_addr;
  sprintf(s, "%d.%d.%d.%d", TO_IP(url_ip_address.u_addr.ip4.addr));
  return;
}
#endif
#endif

/*****************************************************************************
 *
 * @function: server_send()
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
void server_send(void *pvParameters)
{
  int  length;
  char rx_buffer[128];
  int  to_send;
  int  i;
  int  buffer_offset;
  bool new_socket_closed;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "WiFi_tcp_server_task(%d)", TCP_SERVER_PERIOD);))

  /*
   *  Move data in and out of the TCP queues
   */
  while ( 1 )
  {
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
    vTaskDelay(TCP_SERVER_PERIOD);
  }
  /*
   *  All done
   */
  return;
}

/*****************************************************************************
 *
 * @function: server_socket_poll()  0-3
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
void server_receive_poll(void *parameters)
{
  int  length;
  char rx_buffer[256];
  int  i;
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "tcp_socket_poll_0()");))

  while ( 1 )
  {
    for ( i = 0; i != MAX_SOCKETS; i++ )
    {
      if ( socket_list[i].handle > 0 )
      {
        length = recv(socket_list[i].handle, rx_buffer, sizeof(rx_buffer), MSG_DONTWAIT);
        if ( length > 0 )
        {
          server_socket_2_queue(rx_buffer, length);
        }
      }
    }
    vTaskDelay(10);
  }
}

#if ( 0 )
void server_socket_poll_0(void *parameters)
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
        server_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}
#endif

void server_socket_poll_1(void *parameters)
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
        server_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

void server_socket_poll_2(void *parameters)
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
        server_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

void server_socket_poll_3(void *parameters)
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
        server_socket_2_queue(rx_buffer, length);
      }
    }
    vTaskDelay(10);
  }
}

/*****************************************************************************
 *
 * @function: server_accept_poll()
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
void server_accept_poll(void *parameters)
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

/*****************************************************************************
 *
 * @function: WiFi_tests
 *
 * @brief:    A varity of WiFi tests
 *
 * @return:   None
 *
 ****************************************************************************/

void WiFi_show_connections(void)
{
  int i;

  for ( i = 0; i != MAX_SOCKETS; i++ )
  {
    if ( socket_list[i].handle > 0 )
    {
      SEND(ALL, sprintf(_xs, "\r\n\"Socket\": %d, \"Handle\": %d, \"IP\": \"%s\"", i, socket_list[i].handle, &socket_list[i].ip[0]);)
    }
  }

  return;
}
