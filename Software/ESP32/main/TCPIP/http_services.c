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
#include "compute_hit.h"
#include "helpers.h"
#include "http_server.h"
#include "diag_tools.h"
#include "json.h"
#include "serial_io.h"

#include "html.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

/*
 * Typedefs
 */

/*
 *  Variables
 */
static int http_shot = -1; // What shot number have we sent?

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t service_get_index(httpd_req_t *req);
static esp_err_t service_get_who(httpd_req_t *req);
static esp_err_t service_get_shotData(httpd_req_t *req);
static esp_err_t service_get_issf_png(httpd_req_t *req);
static esp_err_t service_get_json(httpd_req_t *req);
static esp_err_t service_get_events(httpd_req_t *req);
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler

/*
 *  URL handlers
 */
const httpd_uri_t url_list[] = {
    {.uri = "/index", .method = HTTP_GET, .handler = service_get_index, .user_ctx = "Index not found"},
    {.uri = "/who", .method = HTTP_GET, .handler = service_get_who, .user_ctx = "Timelord"},
    {.uri = "/shotData", .method = HTTP_GET, .handler = service_get_shotData, .user_ctx = "Shot Data"},
    {.uri = "/json", .method = HTTP_GET, .handler = service_get_json, .user_ctx = "json"},
    {.uri = "/events", .method = HTTP_GET, .handler = service_get_events, .user_ctx = "events"},
    {.uri = "/favicon.ico", .method = HTTP_GET, .handler = service_get_issf_png, .user_ctx = NULL},
    {}
};

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
  int i;

  i = 0;
  while ( url_list[i].uri != 0 )
  {
    httpd_register_uri_handler(server, &url_list[i]);
    i++;
  }
  return;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_events
 *
 * @brief:    Return events to the client
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * This function is called once when the /events URL arrives.
 *
 * On the first call it outputs an empty shot record to prime
 * the client's target and the returns control back to the web
 * page server.
 *
 * Periodically, as scores arrive, the new score is posted to
 * the client in a separate function
 *
 *------------------------------------------------------------*/

static esp_err_t service_get_events(httpd_req_t *req)
{
  const char *resp_str;                           // Reply to server
  char        str[MEDIUM_TEXT];                   // Temporary

  printf("\r\nservice_get_events(%s)", req->uri); // Send out the result for debugging

  if ( http_shot < 0 )                            // First time through
  {
    strcpy(str, "event:new_shotData\nid:\ndata: ");
    build_json_score(&record[0], SCORE_HTTP_PRIME);
    strcat(str, _xs);
    strcat(str, "\n\n");
    resp_str = (const char *)str;
    httpd_resp_set_hdr(req, "application/json", "new_shotData");
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_send(req, resp_str, strlen(resp_str));

    http_shot = 0;
  }
  else
  {
    /*
     * Output a shot when there is one ready
     */
    while ( http_shot == shot_in )
    {
      vTaskDelay(ONE_SECOND);                                                                      // Wait here for a shot to arrive
    }
    if ( (record[http_shot].session_type & (SESSION_VALID | SESSION_SIGHT | SESSION_SCORE)) != 0 ) // Do we have a shot record?
    {
      strcpy(str, "event:new_shotData\nid:\ndata: ");                                              // Yes, prepare it and send it on
      build_json_score(&record[http_shot], SCORE_HTTP);
      strcat(str, _xs);
      strcat(str, "\n\n");
      resp_str = (const char *)str;
      httpd_resp_set_hdr(req, "application/json", "new_shotData");
      httpd_resp_set_type(req, "text/event-stream");
      httpd_resp_send(req, resp_str, strlen(resp_str));
    }
    http_shot = (http_shot + 1) % SHOT_SPACE; // Point to the next shot
  }

  /*
   * Go and wait for the next shot
   */
  return ESP_OK;
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
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_index(httpd_req_t *req)
{
  const char *resp_str;                       // Reply to server
  char        my_name[SHORT_TEXT];            // Temporary string

  printf("service_get_index:(%s)", req->uri); // Send out the result for debugging

  /*
   * Do the things we need to do to start a session
   */
  http_shot = -1; // Reset the shot counter

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                 // Get the target name
  resp_str = (const char *)&index_html; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_index", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_shotData
 *
 * @brief:    Send the shot data to the client
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * The function goes through each shot record and if it is valid
 * sends it to the server as a JSON string.
 *
 * If all of the records are sent, then the function returns an
 * empty string to the client.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_shotData(httpd_req_t *req)
{
  const char *resp_str;                          // Reply to server
  char        my_name[SHORT_TEXT];               // Target name

  printf("service_get_shotData:(%s)", req->uri); // Send out the result for debugging

  /*
   *  First time through, prime the client
   */
  if ( http_shot < 0 )
  {
    build_json_score(&record[0], SCORE_HTTP_PRIME);
    http_shot = 0;
  }

  /*
   *  If there is a new shot, build the json, otherwise empty
   */
  else
  {
    if ( (http_shot != shot_in)                                                                         // Shot ready to send
         && ((record[http_shot].session_type & (SESSION_VALID | SESSION_SIGHT | SESSION_SCORE)) != 0) ) // Do we have a shot record?
    {
      build_json_score(&record[http_shot], SCORE_HTTP);
      http_shot = (http_shot + 1) % SHOT_SPACE;
    }
    else                                                                                                // No shot record
    {
      _xs[0] = 0;
    }
  }

  /*
   *  Populate the reply
   */
  resp_str = (const char *)_xs; // point to the target json file
  target_name(&my_name);        // Get the target name
  httpd_resp_set_hdr(req, "get_ShotData", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  /*
   * All done
   */
  strcpy(my_name, _xs);
  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_shotData: %s", my_name);)) // Send out the result for debugging
  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_json
 *
 * @brief:    Emulate the json commands over a USB or TCPIP connection
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * The input from the client is of the form:
 *
 * json?{"ECHO":1}
 *
 * The function extracts the {"ECHO":1} and puts it into the
 * TCPIP queue.  Executing the vTaskDelay function transfers
 * control to json.c where the data is processed.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_json(httpd_req_t *req)
{
  const char *resp_str;                     // Reply to server
  char        my_name[SHORT_TEXT];          // Target name

  printf("service_get_json(%s)", req->uri); // Send out the result for debugging

  squish(req->uri, _xs);                    // Go through the uri and keep the argument portion
  tcpip_socket_2_queue(_xs, strlen(_xs));   // Put the data into the TCPIP queue
  vTaskDelay(ONE_SECOND);                   // Give up time for the data to be processed

  resp_str = _xs;                           // Send back a reply if there is one
  target_name(&my_name);                    // Get the target name
  httpd_resp_set_hdr(req, "get_json", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  /*
   * All done, return
   */
  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_issf_png
 *
 * @brief:    Send the ISSF PNG file to the client
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_issf_png(httpd_req_t *req)
{
  const char *resp_str;                         // Reply to server
  char        my_name[SHORT_TEXT];              // Target name

  printf("service_get_issf_jpg(%s)", req->uri); // Send out the result for debugging

  target_name(my_name);

  // DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_json(%s)", req);))

  resp_str = (const char *)issf_png; // point to the target json file
  httpd_resp_set_hdr(req, "get_issf_png", my_name);
  httpd_resp_send(req, resp_str, SIZEOF_ISSF_PNG);

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

#if ( 0 )
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
  const char *resp_str;                     // Reply to server
  char        name[SHORT_TEXT];

  target_name(name);                        // Get the target name

  resp_str = (const char *)&post_test_html; // point to the target HTML file

  httpd_resp_set_hdr(req, "FreeETarget", name);
  httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}
#endif

/*----------------------------------------------------------------
 *
 * @function: service_get_who
 *
 * @brief:    Display the target information
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Return the target name and other information
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_who(httpd_req_t *req)
{
  char name[SHORT_TEXT];

  target_name(name); // Get the target name
  httpd_resp_set_hdr(req, "get_who", name);

  sprintf(_xs,
          "Serial Number: %d"
          "<br>Target ID: %s"
          "<br>Verson: %s"
          "<br>Athelete: %s"
          "<br>Target: %s"
          "<br>Event: %s",
          json_serial_number, name, SOFTWARE_VERSION, json_athlete, json_target_name, json_event); // Fill in the target name
  httpd_resp_send(req, _xs, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}
