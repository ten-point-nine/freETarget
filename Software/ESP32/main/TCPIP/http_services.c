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
#include "esp_ota_ops.h"

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
int         http_shot  = -1;    // What shot number have we sent?
static bool event_done = false; // Set to true to end the event

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t service_get_FreeETarget(httpd_req_t *req);                      // Main target page
static esp_err_t service_get_control(httpd_req_t *req);                          // Control the target
static esp_err_t service_get_help(httpd_req_t *req);                             // User help page
static esp_err_t service_get_who(httpd_req_t *req);                              // Target information page
static esp_err_t service_get_issf_png(httpd_req_t *req);                         // Icon for the ISSF target
static esp_err_t service_get_json(httpd_req_t *req);                             // Webb ased JSON interface
static esp_err_t service_get_events(httpd_req_t *req);                           // Get shot events
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler
static esp_err_t service_post_post(httpd_req_t *req);
/*
 *  URL handlers
 */
const httpd_uri_t uri_list[] = {
    {.uri = "/",            .method = HTTP_GET,  .handler = service_get_help,        .user_ctx = NULL},
    {.uri = "/target",      .method = HTTP_GET,  .handler = service_get_FreeETarget, .user_ctx = NULL},
    {.uri = "/who",         .method = HTTP_GET,  .handler = service_get_who,         .user_ctx = NULL},
    {.uri = "/json",        .method = HTTP_GET,  .handler = service_get_json,        .user_ctx = NULL},
    {.uri = "/events",      .method = HTTP_GET,  .handler = service_get_events,      .user_ctx = NULL},
    {.uri = "/favicon.ico", .method = HTTP_GET,  .handler = service_get_issf_png,    .user_ctx = NULL},
    {.uri = "/post",        .method = HTTP_POST, .handler = service_post_post,       .user_ctx = NULL},
    {0,                     .0,                  0,                                  0               }  // End
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
  while ( uri_list[i].uri != 0 )
  {
    httpd_register_uri_handler(server, &uri_list[i]);
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
 * On the first call it outputs an empty shot record to prime
 * the client's target and the returns control back to the web
 * page server.
 *
 * On subsequent calls, the function will wait here until a new
 * shot is availabe and that is sent.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_events(httpd_req_t *req)
{
  char str[MEDIUM_TEXT]; // Temporary

  /*
   *  First time through, send an empty score
   */
  if ( http_shot < 0 )                              // First time through
  {
    build_json_score(&record[0], SCORE_HTTP_PRIME); // Use a fixed reply to set the target type
    http_shot  = 0;                                 // Next time reply with the first shot
    event_done = false;                             // Reset the event done flag
  }

  /*
   * Second time trought Send the next score
   */
  else
  {
    while ( (http_shot == shot_in)                      // Wait for a shot to arrive
            && (event_done == false) )                  // or the event has finished
    {
      vTaskDelay(ONE_SECOND);
    }

    if ( event_done == true )                           // If the event is done
    {
      sprintf(_xs, "{\"event\":\"done\"}");             // Send a done message
    }
    else
    {
      build_json_score(&record[http_shot], SCORE_HTTP); // Send the new shot
      http_shot = (http_shot + 1) % SHOT_SPACE;         // and bump up next one
    }
  }
  //
  /*
   * Prepare and send the payload
   */
  strcpy(str, "event:new_shotData\nid:\ndata: ");
  strcat(str, _xs);
  strcat(str, "\n\n");
  httpd_resp_set_hdr(req, "application/json", "new_shotData");
  httpd_resp_set_type(req, "text/event-stream");
  httpd_resp_send(req, str, strlen(str));

  /*
   *  All done, return
   */
  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_FreeETarget
 *
 * @brief:    Root entry for target HTML files
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 *
 *------------------------------------------------------------*/
static int       server_mode = 0;  // Server mode, 0 = Auto Refresh, 1 = Single
static esp_err_t service_get_FreeETarget(httpd_req_t *req)
{
  const char *resp_str;            // Reply to server
  char        my_name[SHORT_TEXT]; // Temporary string

  printf("service_get_FreeETarget: %s\n", req->uri);

  if ( (instr(req->uri, "MATCH") != 0) || (instr(req->uri, "match") != 0) )
  {
    start_new_session(SESSION_MATCH);
  }

  if ( (instr(req->uri, "SIGHT") != 0) || (instr(req->uri, "sight") != 0) )
  {
    start_new_session(SESSION_SIGHT);
  }

  if ( (instr(req->uri, "MATCH") != 0) || (instr(req->uri, "match") != 0) )
  {
    start_new_session(SESSION_MATCH);
  }

  if ( (instr(req->uri, "AUTO") != 0) || (instr(req->uri, "auto") != 0) )
  {
    server_mode = 0; // Set the server mode to auto refresh
  }
  else
  {
    server_mode = 1; // Set the server mode to single shot
  }

  if ( (instr(req->uri, "STOP") != 0) || (instr(req->uri, "stop") != 0) )
  {
    server_mode = 2; // Set the server mode to stop
  }

  /*
   * Do the things we need to do to start a session
   */
  http_shot = -1; // Reset the shot counter
  connection_list |= HTTP_CONNECTED;

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                       // Get the target name
  resp_str = (const char *)&FreeETarget_html; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_FreeETarget", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_help
 *
 * @brief:    Display the help page
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * Displays the help page to the user.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_help(httpd_req_t *req)
{
  const char *resp_str;            // Reply to server
  char        my_name[SHORT_TEXT]; // Temporary string

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                // Get the target name
  resp_str = (const char *)&help_html; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_help", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  return ESP_OK;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_stop
 *
 * @brief:    Stop the event
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * The function is called when the user clicks on the STOP button
 * or calls the URL /stop.
 *
 * This sets a semaphore to stop the event and send a message
 * back to the client that the event is done.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_control(httpd_req_t *req)
{
  char my_name[SHORT_TEXT]; // Temporary string

  if ( (instr(req->uri, "START") != 0) || (instr(req->uri, "start") != 0) )
  {
    start_new_session(SESSION_MATCH);
  }

  if ( (instr(req->uri, "STOP") != 0) || (instr(req->uri, "stop") != 0) )
  {
    http_shot = -1;    // Reset the shot counter
    connection_list &= ~HTTP_CONNECTED;
    event_done = true; // Set the event done flag
  }

  /*
   *  Send the reply to the client
   */
  target_name(my_name); // Get the target name
  httpd_resp_set_hdr(req, "get_control", my_name);

  if ( event_done == true )
  {
    sprintf(_xs, "event: done");
  }
  else
  {
    sprintf(_xs, "event: starting");
  }

  httpd_resp_send(req, _xs, HTTPD_RESP_USE_STRLEN);

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
  const char *resp_str;                   // Reply to server
  char        my_name[SHORT_TEXT];        // Target name

  squish(req->uri, _xs);                  // Go through the uri and keep the argument portion
  tcpip_socket_2_queue(_xs, strlen(_xs)); // Put the data into the TCPIP queue
  vTaskDelay(ONE_SECOND);                 // Give up time for the data to be processed

  resp_str = _xs;                         // Send back a reply if there is one
  target_name(&my_name);                  // Get the target name
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
  const char *resp_str;              // Reply to server
  char        my_name[SHORT_TEXT];   // Target name

  target_name(my_name);
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
  char                   my_name[SHORT_TEXT];
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  esp_app_desc_t         running_app_info;

  esp_ota_get_partition_description(running_partition, &running_app_info);

  target_name(my_name); // Get the target name
  httpd_resp_set_hdr(req, "get_who", my_name);

  sprintf(_xs,
          "Serial Number: %d"
          "<br>Target ID: %s"
          "<br>Version: %s"
          "<br>OTA Version: %s"
          "<br>Athelete: %s"
          "<br>Target: %s"
          "<br>Event: %s",
          json_serial_number, my_name, SOFTWARE_VERSION, running_app_info.version, json_athlete, json_target_name,
          json_event); // Fill in the target name

  httpd_resp_send(req, _xs, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}
