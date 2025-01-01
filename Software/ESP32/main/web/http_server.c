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

static esp_err_t hello_get_handler(httpd_req_t *req);

/*
 * Typedefs
 */
#if ( BUILD_BASIC_AUTHORIZATION )
typedef struct
{
  char *username;
  char *password;
} basic_auth_info_t;
#endif

/*
 *  Variables
 */
#if ( BUILD_BASIC_AUTHORIZATION )
#define HTTPD_401 "401 UNAUTHORIZED" /*!< HTTP Response 401 */
static httpd_uri_t basic_auth = {
    .uri     = "/basic_auth",
    .method  = HTTP_GET,
    .handler = basic_auth_get_handler,
};
#endif

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);

/*
 *  URL handlers
 */
static const httpd_uri_t url_hello = {.uri = "/hello", .method = HTTP_GET, .handler = hello_get_handler, .user_ctx = "Hello World!"};

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
    httpd_register_uri_handler(server, &url_hello);
#if BUILD_BASIC_AUTHORIZATION
    httpd_register_basic_auth(server);
#endif
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
 * @function: hello_get_handler
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

static esp_err_t hello_get_handler(httpd_req_t *req)
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

  /* Send response with custom headers and body set as the
   * string passed in user context*/
  const char *resp_str = (const char *)req->user_ctx;
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

#if ( BUILD_BASIC_AUTHORIZATION )
static char *http_auth_basic(const char *username, const char *password)
{
  size_t out;
  char  *user_info = NULL;
  char  *digest    = NULL;
  size_t n         = 0;
  int    rc        = asprintf(&user_info, "%s:%s", username, password);
  if ( rc < 0 )
  {
    ESP_LOGE(TAG, "asprintf() returned: %d", rc);
    return NULL;
  }

  if ( !user_info )
  {
    ESP_LOGE(TAG, "No enough memory for user information");
    return NULL;
  }
  esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

  /* 6: The length of the "Basic " string
   * n: Number of bytes for a base64 encode format
   * 1: Number of bytes for a reserved which be used to fill zero
   */
  digest = calloc(1, 6 + n + 1);
  if ( digest )
  {
    strcpy(digest, "Basic ");
    esp_crypto_base64_encode((unsigned char *)digest + 6, n, &out, (const unsigned char *)user_info, strlen(user_info));
  }
  free(user_info);
  return digest;
}
#endif

#if ( BUILD_BASIC_AUTHORIZATION )
/*----------------------------------------------------------------
 *
 * @function: basic_auth_get_handler
 *
 * @brief:    Manages an HTTP GET request
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
static esp_err_t basic_auth_get_handler(httpd_req_t *req // Pointer to received request
)
{
  char              *buf             = NULL;
  size_t             buf_len         = 0;
  basic_auth_info_t *basic_auth_info = req->user_ctx;

  /*
   *  Check to see if the request is authorized
   */
  buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1; // Find out how long the request is
  if ( buf_len > 1 )                                               // Got something legitimate?
  {
    buf = calloc(1, buf_len);                                      // Allocate memory
    if ( !buf )                                                    // No memory
    {
      ESP_LOGE(TAG, "No enough memory for basic authorization");
      return ESP_ERR_NO_MEM;
    }

                                                                   /*
                                                                    *   Pull in the header and look for "AUthorization" as a field
                                                                    */
    if ( httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK )
    {
      ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
    }
    else
    {
      ESP_LOGE(TAG, "No auth value received");
    }

    /*
     * See if out authorization matches
     */
    char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
    if ( !auth_credentials )
    {
      ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
      free(buf);
      return ESP_ERR_NO_MEM;
    }

    if ( strncmp(auth_credentials, buf, buf_len) )
    {
      ESP_LOGE(TAG, "Not authenticated");
      httpd_resp_set_status(req, HTTPD_401);
      httpd_resp_set_type(req, "application/json");
      httpd_resp_set_hdr(req, "Connection", "keep-alive");
      httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
      httpd_resp_send(req, NULL, 0);
    }
    else
    {
      ESP_LOGI(TAG, "Authenticated!");
      char *basic_auth_resp = NULL;
      httpd_resp_set_status(req, HTTPD_200);
      httpd_resp_set_type(req, "application/json");
      httpd_resp_set_hdr(req, "Connection", "keep-alive");
      int rc = asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
      if ( rc < 0 )
      {
        ESP_LOGE(TAG, "asprintf() returned: %d", rc);
        free(auth_credentials);
        return ESP_FAIL;
      }
      if ( !basic_auth_resp )
      {
        ESP_LOGE(TAG, "No enough memory for basic authorization response");
        free(auth_credentials);
        free(buf);
        return ESP_ERR_NO_MEM;
      }
      httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
      free(basic_auth_resp);
    }
    free(auth_credentials);
    free(buf);
  }
  else
  {
    ESP_LOGE(TAG, "No auth header received");
    httpd_resp_set_status(req, HTTPD_401);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
    httpd_resp_send(req, NULL, 0);
  }

  return ESP_OK;
}
#endif
#if ( BUILD_BASIC_AUTHORIZATION )
/*----------------------------------------------------------------
 *
 * @function: httpd_register_basic_auth
 *
 * @brief:    Save the authorization parameters for later if used
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
static void httpd_register_basic_auth(httpd_handle_t server)
{
  basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
  if ( basic_auth_info )
  {
    basic_auth_info->username = CONFIG_EXAMPLE_BASIC_AUTH_USERNAME;
    basic_auth_info->password = CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD;

    basic_auth.user_ctx = basic_auth_info;
    httpd_register_uri_handler(server, &basic_auth);
  }
}
#endif
