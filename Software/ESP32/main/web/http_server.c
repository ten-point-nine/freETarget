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
 * https://github.com/espressif/esp-idf/blob/8760e6d2a7e19913bc40675dd71f374bcd51b0ae/examples/protocols/http_server/simple/main.c
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

// #include <esp_wifi.h>
// #include <esp_system.h>
// #include "nvs_flash.h"
//  #include "esp_eth.h"

#if ( !BRIAN )
#include "freETarget.h"
#include "http_server.h"
#include "diag_tools.h"
#endif

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

#if BRIAN
static char _xs[512];
#define DLT()
#endif

static esp_err_t target_get_handler(httpd_req_t *req);

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

/*
 *  URL handlers
 */
static const httpd_uri_t url_target = {.uri = "/target", .method = HTTP_GET, .handler = target_get_handler, .user_ctx = "Hello World!"};

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
 *
 *------------------------------------------------------------*/
httpd_handle_t start_webserver(void)
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.server_port      = DEFAULT_SERVER_PORT;
  config.lru_purge_enable = true;

  /*
   * Start the web server
   */
  DLT(DLT_HTTP, SEND(sprintf(_xs, "Starting server on port: '%d'", config.server_port);))

  if ( httpd_start(&server, &config) == ESP_OK ) // Create the server
  {
    DLT(DLT_HTTP, SEND(sprintf(_xs, "Registering URI handlers");))
    httpd_register_uri_handler(server, &url_target);

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
 * @brief:    Entry point to handle a GET request
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

static esp_err_t target_get_handler(httpd_req_t *req)
{
  char  *buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if ( buf_len <= sizeof(_xs) )
  {
    buf = &_xs;
    if ( httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK )
    {
      //      DLT(DLT_HTTP, SEND(sprintf(_xs, "Found header => Host: %s", buf);))
    }
  }

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
  httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

  extern char ts[];

  /* Send response with custom headers and body set as the
   * string passed in user context*/
  const char *resp_str = (const char *)&ts;
  printf(ts);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  /* After sending the HTTP response the old HTTP request
   * headers are lost. Check if HTTP request headers can be read now. */
  if ( httpd_req_get_hdr_value_len(req, "Host") == 0 )
  {
    DLT(DLT_HTTP, SEND(sprintf(_xs, "Request headers lost");))
  }
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

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
  /* For any other URI send 404 and close socket */
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
  return ESP_FAIL;
}
