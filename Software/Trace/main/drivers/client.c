/******************************************************************************
 *
 * client.c
 *
 * FreeETarget client driver
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
static int client_socket = AVAILABLE_SOCKET; // Socket used to talk to the target

/*
 * Private Functions
 */
esp_err_t esp_base_mac_addr_get(uint8_t *mac);

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
 * Create a connection to a remote server
 *
 ***************************************************************************/
bool WiFi_client_init(void)
{
  struct sockaddr_in dest_addr;

  /*
   *  Check to see if we are already connected
   */
  IF_IN(SERVER_CONNECTED)
  {
    return true;
  }

  /*
   *  Not connected, then connect
   */
  if ( client_socket <= 0 )
  {
    memset((void *)&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_len         = sizeof(dest_addr);
    dest_addr.sin_addr.s_addr = inet_addr(json_remote_ip);
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
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(CONSOLE, sprintf(_xs, "Keeping socket: %d", client_socket);))
    close(client_socket);
    vTaskDelay(2);
  }

  /*
   * Make the connection
   */
  if ( lwip_connect(client_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0 )
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Socket unable to connect to %s:%d: errno %d", json_remote_ip, 1090, errno);))
    return false;
  }

  /*
   *  Got here, ready to go
   */
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Connected to remote at: %s:%d", json_remote_ip, 1090);))
  run_state |= SERVER_CONNECTED; // Yay, we're connected
  return true;
}

/*****************************************************************************
 *
 * @function: WiFi_client_recv
 *
 * @brief:    Manage received packets from the target
 *
 * @return:   None
 *
 ****************************************************************************
 *
 *
 *
 ***************************************************************************/
char rx_buffer[256];
char propeller[] = {'-', '/', '|', '\\'};

void client_recv(void *params)
{
  int length;

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "WiFi_client_recv()");))

  while ( 1 )
  {
    IF_IN(SERVER_CONNECTED)
    {
      length = recv(client_socket, rx_buffer, sizeof(rx_buffer), MSG_DONTWAIT | MSG_OOB);
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
 * @function: WiFi_client_send
 *
 * @brief:    Send packets to the target
 *
 * @return:   None
 *
 ****************************************************************************
 *
 * This is called periodically from FreeRTOS to bring in the latest
 *
 *
 ***************************************************************************/
void client_send(char *buffer, // Data to send
                      int   length)   // Length of the data
{

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "WiFi_client_send()");))

  lwip_send(client_socket, buffer, length, MSG_DONTWAIT);

  /*
   * Done
   */
  return;
}

/*****************************************************************************
 *
 * @function: WiFi_client_test
 *
 * @brief:    Send and receive packets from the target for test
 *
 * @return:   None
 *
 ****************************************************************************
 *
 *
 *
 ***************************************************************************/
static char *test_s[] = {"{\"ECHO\"}", "{\"VERSION\"}", "{\"NTP_ASK\"}", "{\"NTP_MASTER\"}", "{\"NTP_SLAVE\"}", NULL};

void WiFi_client_test(void) //
{
  char ch;
  int  i;

  while ( 1 )
  {
    /*
     *  Prompt for a test number.  Display incoming messages
     */
    SEND(CONSOLE, sprintf(_xs, "\r\nChoose test");)
    i = 0;

    while ( test_s[i] != NULL ) // Display the tests
    {
      SEND(CONSOLE, sprintf(_xs, "\r\n%d: %s", i, test_s[i]);)
      i++;
    }
    SEND(CONSOLE, sprintf(_xs, "\r\n");)

    /*
     * Echo the target back to the console
     */
    while ( serial_available(CONSOLE) == 0 )
    {
      vTaskDelay(TICK_10ms);
      while ( serial_available(CLIENT) != 0 )
      {
        ch = serial_getch(CLIENT);
        if ( isprint(ch) )
        {
          SEND(CONSOLE, sprintf(_xs, "%c", ch);)
        }
        else
        {
          SEND(CONSOLE, sprintf(_xs, " 0x%02X ", ch);)
        }
      }
    }

    /*
     *  Get the test number
     */
    ch = serial_getch(CONSOLE); // Get the test number
    if ( ch == '!' )            // ! => Exit
    {
      break;
    }

    /*
     *  Send out the test
     */
    ch = (ch - '0') % (sizeof(test_s) / sizeof(char *) - 1);

    IF_NOT(SERVER_CONNECTED)                                             // Not connected
    {
      SEND(CONSOLE, sprintf(_xs, "\r\nTarget not connected");)           // Send this test
      break;                                                             // Return nothing
    }

    SEND(CONSOLE, sprintf(_xs, "\r\nSending: %s\r\n", test_s[(int)ch]);) // Send this test
    SEND(CLIENT, sprintf(_xs, "%s", test_s[(int)ch]);)                   // Send this test
  }

  /*
   *  Test done
   */
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}
