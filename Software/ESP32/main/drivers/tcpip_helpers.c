/******************************************************************************
 *
 * tcpip_helpers.c
 *
 * General purpose TCP/IP helper functions
 *
 ******************************************************************************
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

/*
 * Macros
 */

/*
 * Typdef
 */

/*
 * Variables
 */

queue_struct_t in_buffer;  // TCPIP input buffer
queue_struct_t out_buffer; // TCPIP input buffer
/*
 * Private Functions
 */

/*******************************************************************************
 *
 * @function: tcpip_app_2_queue
 *
 * @brief:    Put something into the output queue for later transmission
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * This function is called by the application to save data into the
 * TCPIP queue for later output onto the TCPIP channel
 *
 ******************************************************************************/
int tcpip_app_2_queue(char *buffer, // Where to return the bytes
                      int   length  // Maximum transfer size
)
{
  int bytes_moved;                  // Number of bytes written

  bytes_moved = 0;
  while ( length != 0 )
  {
    out_buffer.queue[out_buffer.in] = *buffer;
    buffer++;
    length--;
    bytes_moved++;
    out_buffer.in = (out_buffer.in + 1) % sizeof(out_buffer.queue);
  }

  /*
   *  All done, return the number of bytes written to the queue
   */
  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_queue_2_socket
 *
 * @brief:    Take waiting bytes out of the queue and into the socket
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * This function is the companion to tcpip_app_t_queue that finished sending
 * the data out to the socket
 *
 ******************************************************************************/
int tcpip_queue_2_socket(char *buffer, // Place to put data
                         int   length)   // Number of bytes to read
{
  int bytes_moved;                     // Number of bytes read from queue

  if ( out_buffer.out == out_buffer.in )
  {
    return 0;                          // Nothing to say
  }

  bytes_moved = 0;

  while ( length != 0 )
  {
    *buffer = out_buffer.queue[out_buffer.out];
    buffer++;
    length--;
    bytes_moved++;
    out_buffer.out = (out_buffer.out + 1) % sizeof(out_buffer.queue);
    if ( out_buffer.out == out_buffer.in )
    {
      break; // Run out of things to read
    }
  }

  /*
   *  All done, return the number of bytes written to the queue
   */
  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_queue_2_app
 *
 * @brief:    Read data out of the queue and return it to the application
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * Characters from the TCPIP input queue are returned to the application
 *
 ******************************************************************************/
int tcpip_queue_2_app(char *buffer, // Where to return the bytes
                      int   length  // Maximum transfer size
)
{
  int bytes_moved;

  bytes_moved = 0;
  if ( in_buffer.out == in_buffer.in )
  {
    return 0; // Nothing waiting for us
  }

  while ( length )
  {
    *buffer = in_buffer.queue[in_buffer.out];
    buffer++;
    length--;
    bytes_moved++;
    in_buffer.out = (in_buffer.out + 1) % sizeof(in_buffer.queue);
    if ( in_buffer.out == in_buffer.in )
    {
      break; // Reached the end
    }
  }

  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_socket_2_queue
 *
 * @brief:    Put fresh TCPIP data into the queue for later
 *
 * @return:   Input queue updated
 *
 *******************************************************************************
 *
 * Fresh characters from the TCPIP socket are placed into the input queue
 *
 * Used also by HTTP to put client data into the queue
 *
 ******************************************************************************/
int tcpip_socket_2_queue(char *buffer, // Where to return the bytes
                         int   length)   // Maximum transfer size
{
  int bytes_moved;

  bytes_moved = 0;
  while ( length )
  {
    in_buffer.queue[in_buffer.in] = *buffer;
    buffer++;
    length--;
    bytes_moved++;
    in_buffer.in = (in_buffer.in + 1) % sizeof(in_buffer.queue);
    if ( in_buffer.out == in_buffer.in )
    {
      DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "TCPIP input queue overrun");)) // Reached the end
      break;
    }
  }

  return bytes_moved;
}
