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

static esp_err_t service_get_index(httpd_req_t *req);                            // Standard HTML Index
static esp_err_t service_get_target(httpd_req_t *req);                           // Respond to a request for a target display
static esp_err_t service_get_issf_png(httpd_req_t *req);                         // Respond to a request for a target display
static esp_err_t service_get_post(httpd_req_t *req);
static esp_err_t service_post_post(httpd_req_t *req);                            // Sample POST handler
static esp_err_t service_get_post2(httpd_req_t *req);
static esp_err_t service_post_post2(httpd_req_t *req);                           // Sample POST handler
esp_err_t        http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler

/*
 * Typedefs
 */

/*
 *  Variables
 */
extern const char index_html[];     // Pointer to target HTML
extern const char pistol_10_html[]; // Pointer to target HTML
extern const char post_test_html[];
extern const char post_test_2_html[];
extern const char issf_png[];
extern const int  sizeof_issf_png;
extern const int  sizeof_index_html;

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);

/*
 *  URL handlers
 */
static const httpd_uri_t url_get_index = {.uri = "/index", .method = HTTP_GET, .handler = service_get_index, .user_ctx = "Index not found"};

static const httpd_uri_t url_get_target = {
    .uri = "/target", .method = HTTP_GET, .handler = service_get_target, .user_ctx = "Target not found"};
static const httpd_uri_t url_get_favicon  = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = service_get_issf_png, .user_ctx = NULL};
static const httpd_uri_t url_get_issf_png = {
    .uri = "/issf.png", .method = HTTP_GET, .handler = service_get_issf_png, .user_ctx = "Target not found"};

static const httpd_uri_t url_get_post  = {.uri = "/post", .method = HTTP_GET, .handler = service_get_post, .user_ctx = NULL};
static const httpd_uri_t url_post_post = {.uri = "/post", .method = HTTP_POST, .handler = service_post_post, .user_ctx = NULL};

static const httpd_uri_t url_get_post2  = {.uri = "/post2", .method = HTTP_GET, .handler = service_get_post2, .user_ctx = NULL};
static const httpd_uri_t url_post_post2 = {.uri = "/post2", .method = HTTP_POST, .handler = service_post_post2, .user_ctx = NULL};

/*----------------------------------------------------------------
 *
 * @function: register_services
 *
 * @brief:    Register the services in this file
 *
 * @return:   None
 *
 *---------------------------------------------------------------
 *
 * Read the nonvol into RAM.
 *
 * If the results is uninitalized then force the factory default.
 * Then check for out of bounds and reset those values
 *
 *------------------------------------------------------------*/
void register_services(httpd_handle_t server // Pointer to active server
)
{
  httpd_register_uri_handler(server, &url_get_index);

  httpd_register_uri_handler(server, &url_get_target);
  httpd_register_uri_handler(server, &url_get_favicon);
  httpd_register_uri_handler(server, &url_get_issf_png);

  httpd_register_uri_handler(server, &url_get_post);
  httpd_register_uri_handler(server, &url_post_post);

  httpd_register_uri_handler(server, &url_get_post2);
  httpd_register_uri_handler(server, &url_post_post2);
  return;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_index
 *
 * @brief:    Root entry for target HTML files
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
static esp_err_t service_get_index(httpd_req_t *req)
{
  const char *resp_str;                 // Reply to server

  resp_str = (const char *)&index_html; // point to the target HTML file

  httpd_resp_set_hdr(req, "index", names[json_name_id]);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_target
 *
 * @brief:    Display a target on the page
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Return a pointer to the target HTML
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_target(httpd_req_t *req)
{
  const char *resp_str;                     // Reply to server

  resp_str = (const char *)&pistol_10_html; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", names[json_name_id]);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_issf_png
 *
 * @brief:    Return a .PNG file to the client
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Return a pointer to the target HTML
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_issf_png(httpd_req_t *req)
{
  const char *resp_str;               // Reply to server

  resp_str = (const char *)&issf_png; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", names[json_name_id]);
  httpd_resp_send(req, resp_str, sizeof_issf_png);

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: sample_post_get_handler
 *
 * @brief:    Use a GET to post a POST page
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Test POST hanbdler.  This uploads the GET page
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_post(httpd_req_t *req)
{
  const char *resp_str;                     // Reply to server

  resp_str = (const char *)&post_test_html; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", names[json_name_id]);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: sample_post_post_handler
 *
 * @brief:    Entry point to handle a POST request
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Test POST hanbdler.  This uploads the POST page
 *
 *------------------------------------------------------------*/
static esp_err_t service_post_post(httpd_req_t *req)
{
  int ret;                      // Number of bytes remaining to be proceesed
  int remaining;                // Bytes remaining to be processed

  remaining = req->content_len; // Find out how long the transfer is

                                /*
                                 *  Loop and find out how long the transfer is
                                 */
  while ( remaining > 0 )
  {
    if ( (ret = httpd_req_recv(req, _xs, MIN(remaining, sizeof(_xs)))) <= 0 )
    {
      if ( ret == HTTPD_SOCK_ERR_TIMEOUT )
      {
        /* Retry receiving if timeout occurred */
        continue;
      }
      return ESP_FAIL; // Ran out of time waiting
    }

                       /*
                        *  The buffer _xs contains (at most) sizeof(_xs) bytes
                        *  Do womething with it
                        */

    printf("\r\n=========== RECEIVED DATA ==========\r\n");
    printf("%.*s", ret, _xs);
    printf("\r\n====================================\r\n");
  }

  /*
   *  All done, end the respone and return
   */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: sample_post_get_handler
 *
 * @brief:    Use a GET to post a POST page
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
static esp_err_t service_get_post2(httpd_req_t *req)
{
  const char *resp_str;                       // Reply to server

  resp_str = (const char *)&post_test_2_html; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", names[json_name_id]);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: sample_post_post_handler
 *
 * @brief:    Entry point to handle a POST request
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * The REST client has issued a POST request to the server (me)
 *
 * This function copies the contents of the transfer to a buffer
 * for later use by the application
 *
 *------------------------------------------------------------*/
static esp_err_t service_post_post2(httpd_req_t *req)
{
  int ret;                      // Number of bytes remaining to be proceesed
  int remaining;                // Bytes remaining to be processed

  remaining = req->content_len; // Find out how long the transfer is

                                /*
                                 *  Loop and find out how long the transfer is
                                 */
  while ( remaining > 0 )
  {
    if ( (ret = httpd_req_recv(req, _xs, MIN(remaining, sizeof(_xs)))) <= 0 )
    {
      if ( ret == HTTPD_SOCK_ERR_TIMEOUT )
      {
        /* Retry receiving if timeout occurred */
        continue;
      }
      return ESP_FAIL; // Ran out of time waiting
    }

                       /*
                        *  The buffer _xs contains (at most) sizeof(_xs) bytes
                        *  Do womething with it
                        */

    printf("\r\n=========== RECEIVED DATA ==========\r\n");
    printf("%.*s", ret, _xs);
    printf("\r\n====================================\r\n");
  }

  /*
   *  All done, end the respone and return
   */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}
