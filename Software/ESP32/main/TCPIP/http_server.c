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
#define BRIAN (0 == 1)
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// #include <esp_log.h>
//  #include <nvs_flash.h>
#include <sys/param.h>
// #include "esp_netif.h"
// #include "protocol_examples_common.h"
// #include "protocol_examples_utils.h"
// #include "esp_tls_crypto.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
// #include "esp_check.h"

#include <esp_wifi.h>
// #include <esp_system.h>
// #include "nvs_flash.h"
//  #include "esp_eth.h"

#include "freETarget.h"
#include "http_server.h"
#include "http_services.h"
#include "diag_tools.h"
#include "json.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

#if BRIAN
static char _xs[512];
#define DLT()
#endif

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler

/*
 * Typedefs
 */

/*
 *  Variables
 */

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
httpd_handle_t start_webserver(void)
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.server_port      = DEFAULT_SERVER_PORT;
  config.lru_purge_enable = true;

  DLT(DLT_INFO, SEND(sprintf(_xs, "start_webserver(port: %d)", config.server_port);))

  /*
   *  Create event loops
   */
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

  /*
   * Start the web server
   */
  if ( httpd_start(&server, &config) == ESP_OK ) // Create the server
  {
    DLT(DLT_HTTP, SEND(sprintf(_xs, "Registering URI handlers");))
    register_services(server);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    return server;
  }

  /*
   *  Got here because we could not start the server
   */
  DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Error starting server!");))
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
      DLT(DLT_HTTP, SEND(sprintf(_xs, "Web server stopped");))
    }
    else
    {
      DLT(DLT_HTTP, SEND(sprintf(_xs, "Web server failed to stop");))
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
    DLT(DLT_HTTP, SEND(sprintf(_xs, "Starting webserver");))
    *server = start_webserver();
  }
  else
  {
    DLT(DLT_HTTP, SEND(sprintf(_xs, "Could not start webserver");))
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
  char temp[64];
  int  i;

  strncpy(temp, req->uri, sizeof(temp));                          // Copy part of the URL for display
  sprintf(_xs, "Error 404. Service %s not found", temp);          // Error reported to the user
  strncat(_xs, "<br>Valid URLs<br/>", sizeof(_xs) - strlen(_xs)); // Error reported to the user
  i = 0;
  while ( url_list[i] != 0 )
  {
    strncat(_xs, "<br>", sizeof(_xs) - strlen(_xs));
    strncat(_xs, url_list[i]->uri, sizeof(_xs) - strlen(_xs));
    i++;
  }
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