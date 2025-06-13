/*-------------------------------------------------------
 *
 * http_server.c
 *
 * General purpose file to perform HTTP web server
 * functions
 *
 *------------------------------------------------------
 *
 * This is a generic HTTP web server
 *
 * The softwareis based on
 * https://github.com/espressif/esp-idf/blob/8760e6d2a7e19913bc40675dd71f374bcd51b0ae/examples/protocols/http_server/simple/main/main.c
 *
 *
 * See http_server.h for the various compilation options
 *
 *-----------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include <esp_wifi.h>

#include "freETarget.h"
#include "http_server.h"
#include "http_services.h"
#include "diag_tools.h"
#include "json.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

/*
 *  Local functions
 */

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler

/*
 * Typedefs
 */

/*
 *  Variables
 */
static httpd_req_t *static_req = NULL; // Request pointer used inside of this file

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);

/*----------------------------------------------------------------
 *
 * @function: start_webserver(void)
 *
 * @brief:    Start the web server
 *
 * @return:   handle to the web server
 *
 *---------------------------------------------------------------
 *
 * This is called once to initialize the web server
 *
 * Initialization is done in three steps
 *  1 - Create a web socket (in this case port 80)
 *  2 - Register the URL handlers
 *  3 - Register a 404 (URL not found) handler
 *
 *------------------------------------------------------------*/
httpd_handle_t start_webserver(unsigned int port // Port to use for the web server
)
{
  static unsigned int server_count = 0;          // Count the number of servers started
  httpd_handle_t      server       = NULL;
  httpd_config_t      config       = HTTPD_DEFAULT_CONFIG();

  config.server_port      = port;
  config.lru_purge_enable = true;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "start_webserver(port: %d)", config.server_port);))

  /*
   * Start the web server
   */
  if ( httpd_start(&server, &config) == ESP_OK ) // Create the server
  {
#if ( 0 )
    if ( server_count == 0 )
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Registering event handlers");))
      ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
      ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    }
#endif
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Registering URI handlers using port %d", port);))
    register_services(server, port);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    server_count++; // Increment the number of servers started

    return server;
  }

  /*
   *  Got here because we could not start the server
   */
  DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Error starting server!");))
  return NULL;
}

/*----------------------------------------------------------------
 *
 * @function: disconnect_handler
 *
 * @brief:    Entry point to handle disconnections
 *
 * @return:   None
 *
 *---------------------------------------------------------------
 *
 * A disconnect event has arrived.  When that happens, stop
 * the web server
 *
 *------------------------------------------------------------*/
void disconnect_handler(void            *arg,        // Arguments that we got
                        esp_event_base_t event_base, //
                        int32_t          event_id,   //
                        void            *event_data  //
)
{
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if ( *server )
  {
    if ( stop_webserver(*server) == ESP_OK )
    {
      *server = NULL;
      DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Web server stopped");))
    }
    else
    {
      DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Web server failed to stop");))
    }
  }
}

/*----------------------------------------------------------------
 *
 * @function: connect_handler
 *
 * @brief:    A connection request from a client has arrived
 *
 * @return:   None
 *
 *---------------------------------------------------------------
 *
 * A disconnect event has arrived.  When that happens, stop
 * the web server
 *
 *------------------------------------------------------------*/
void connect_handler(void            *arg,        //
                     esp_event_base_t event_base, //
                     int32_t          event_id,   //
                     void            *event_data  //
)
{
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if ( *server == NULL )
  {
    DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Starting webserver");))
    *server = start_webserver(DEFAULT_HTTP_PORT);
  }
  else
  {
    DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Could not start webserver");))
  }

  /*
   *  All done, return
   */
  return;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
  // Stop the httpd server
  return httpd_stop(server);
}

/*----------------------------------------------------------------
 *
 * @function: http_404_error_handler
 *
 * @brief:    Report a 404 error to the target
 *
 * @return:   None
 *
 *---------------------------------------------------------------
 *
 * The user has asked for a URL that doesn't exist.
 *
 * This function just reports the url back to the user for
 * correction
 *
 *------------------------------------------------------------*/
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "http_404_error_handler: %s", req->uri);))

  sprintf(_xs, "Error 404. Service not found: %s", req->uri); // Error reported to the user

  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, _xs);
  return ESP_FAIL;
}

/*----------------------------------------------------------------
 *
 * @function: get_url_arg
 *
 * @brief:    Extract the arguements from the URL
 *
 * @return:   Number of characters extracted
 *
 *---------------------------------------------------------------
 *
 * The URL arrives at the target like
 *
 * /target?sch,i.html?_from=R40&_trksid=p4432023.m570.l1313&_nkw=bob&_sacat=0
 *
 * This function takes the URL apart starting at the ? and
 * returns the rest of the string
 *
 *------------------------------------------------------------*/
int get_url_arg(char *req_url,     // URL and arguements as from web page
                char *reply,       // Whare to put the reply
                int   sizeof_reply // How much space do we have );
)
{
  int  i;                          // Iteration counter
  char ch;                         // Working character
  char temp[5];                    // Place to put working number

  /*
   *  Find out where the arguements start (just after the ?)
   */
  i = 0;
  while ( *req_url != 0 )
  {
    if ( *req_url == '?' ) // Found the ?
    {
      break;               // Yes, bail
    }
    req_url++;             // No, keep looking
    i++;
    if ( i >= sizeof_reply )
    {
      return -1;           // Ran out of space
    }
  }
  req_url++;               // Move past the?

                           /*
                            * Copy over the results
                            */
  while ( *req_url != 0 )
  {
    ch = *req_url;   // Pick up the next character
    if ( ch == '%' ) // Is it an escape?
    {
      temp[0] = '0';
      temp[1] = 'X';
      req_url++;     // Go to the next character
      temp[2] = *req_url;
      req_url++;
      temp[3] = *req_url;
      req_url++;
      temp[4] = 0;
      ch      = strtol(temp, NULL, 16);
    }
    req_url++;
    *reply = ch;
    reply++;
    i++;
    if ( i >= sizeof_reply )
    {
      return -1;
    }
  }

  /*
   *   We have moved the string
   */
  *reply = 0;
  return i;
}

/*----------------------------------------------------------------
 *
 * @function: http_send_string
 *
 * @brief:    Send a chunk of text to the HTTP client
 *
 * @return:   none
 *
 *---------------------------------------------------------------
 *
 * This function is called in response to a user starting a
 * shooting session.
 *
 *------------------------------------------------------------*/
void http_send_string_start(httpd_req_t *req)       // Start to send a string to the client
{

  static_req = req;                                 // Remember who we are talking to

  return;
}

void http_send_string(const char *str)              // String to send to the client
{
  char *sout;                                       // Pointer to the output string
  int   i;

  if ( static_req == NULL )                         // If the request pointer is not set
  {
    return;
  }

  i    = 0;
  sout = (char *)str;                               // Point to the string to send
  while ( *str != 0 )
  {
    if ( *str == '\n' )                             // If we have a new line
    {
      httpd_resp_send_chunk(static_req, sout, i);   // Send out what we have so far
      httpd_resp_send_chunk(static_req, "<br>", 4); // Send a CRLF to the client
      i = 0;                                        // Reset the counter
      str++;                                        // Move to the next character
      sout = (char *)str;                           // Point to the string to send
    }
    else
    {
      i++;                                          // Move to the next character
      str++;
    }
  }

  if ( i == 0 )                                     // If we have nothing to send
  {
    return;                                         // Nothing to do
  }
  httpd_resp_send_chunk(static_req, sout, i);
}

void http_send_string_end(httpd_req_t *req)         // Stop to send a string to the client
{

  char str[1];                                      // Temporary string

  str[0] = 0;
  if ( static_req != NULL )                         // If the request pointer is not set
  {
    httpd_resp_send_chunk(static_req, str, 0);      // Send the string to the client
  }
  static_req = NULL;                                // Erase it

  return;
}