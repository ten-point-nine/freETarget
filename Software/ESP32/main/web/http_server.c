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

#if ( !BRIAN )
#include "freETarget.h"
#include "http_server.h"
#include "diag_tools.h"
#include "json.h"
#endif

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

#if BRIAN
static char _xs[512];
#define DLT()
#endif

static esp_err_t target_get_handler(httpd_req_t *req);                           // Respond to a request for a target display
esp_err_t        http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler

/*
 * Typedefs
 */

/*
 *  Variables
 */
extern char ts[]; // Pointer to target HTML

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);

/*
 *  URL handlers
 */
static const httpd_uri_t url_target = {.uri = "/target", .method = HTTP_GET, .handler = target_get_handler, .user_ctx = "Target not found"};

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
    httpd_register_uri_handler(server, &url_target);
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
 * @function: target_get_handler
 *
 * @brief:    Display a target on the page
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Read the nonvol into RAM.
 *
 * If the results is uninitalized then force the factory default.
 * Then check for out of bounds and reset those values
 *
 *------------------------------------------------------------*/
static esp_err_t target_get_handler(httpd_req_t *req)
{
  const char *resp_str;         // Reply to server

  resp_str = (const char *)&ts; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", names[json_name_id]);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

#if ( 0 )
/*----------------------------------------------------------------
 *
 * @function: hello_post_handler
 *
 * @brief:    Entry point to handle a GET request
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Read the nonvol into RAM.
 *
 * If the results is uninitalized then force the factory default.
 * Then check for out of bounds and reset those values
 *
 *------------------------------------------------------------*/
static esp_err_t echo_post_handler(httpd_req_t *req)
{
  char buf[100];
  int  ret, remaining = req->content_len;

  while ( remaining > 0 )
  {
    /* Read the data for the request */
    if ( (ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0 )
    {
      if ( ret == HTTPD_SOCK_ERR_TIMEOUT )
      {
        /* Retry receiving if timeout occurred */
        continue;
      }
      return ESP_FAIL;
    }

    /* Send back the same data */
    httpd_resp_send_chunk(req, buf, ret);
    remaining -= ret;

    /* Log data received */
    ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG, "%.*s", ret, buf);
    ESP_LOGI(TAG, "====================================");
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}
#endif

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

  strncpy(temp, req->uri, sizeof(temp));                 // Copy part of the URL for display
  temp[63] = 0;
  sprintf(_xs, "Error 404. Service %s not found", temp); // Error reported to the user
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, _xs);
  return ESP_FAIL;
}