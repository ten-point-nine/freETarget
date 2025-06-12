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
#include "http_services.h"
#include "diag_tools.h"
#include "json.h"
#include "serial_io.h"

#include "html.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

/*
 * Typedefs
 */
typedef enum // Server modes
{
  IDLE = 0,  // 0 Idle mode, no event
  AUTO,      // 1 Auto refresh mode
  SINGLE,    // 2 Single shot mode
  CLOSE      // 3 Close the event
} event_mode_t;

/*
 *  Variables
 */
int                 http_shot  = -1;   // What shot number have we sent?
static event_mode_t event_mode = IDLE; // Set the server mode to auto refresh

typedef struct
{
  unsigned int port;
} my_user_ctx_t;                       // Internal user context structure

static const my_user_ctx_t http_events_ctx = {DEFAULT_HTTP_PORT};
static const my_user_ctx_t event_menu_ctx  = {EVENT_HTTP_PORT};

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t service_get_FreeETarget(httpd_req_t *req);                      // Main target page
static esp_err_t service_get_help(httpd_req_t *req);                             // User help page
static esp_err_t service_get_menu(httpd_req_t *req);                             // Control back channel
static esp_err_t service_get_who(httpd_req_t *req);                              // Target information page
static esp_err_t service_get_issf_png(httpd_req_t *req);                         // Icon for the ISSF target
static esp_err_t service_get_json(httpd_req_t *req);                             // Webb ased JSON interface
static esp_err_t service_get_events(httpd_req_t *req);                           // Get shot events
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err); // Create a URL not found handler
static esp_err_t service_post_post(httpd_req_t *req);

/*
 *  URI handlers
 */
const my_uri_t uri_list[] = {
    {DEFAULT_HTTP_PORT, {"/", HTTP_GET, service_get_FreeETarget, NULL}                     }, // Main target page
    {DEFAULT_HTTP_PORT, {"/events", HTTP_GET, service_get_events, (void *)&http_events_ctx}},

    {DEFAULT_HTTP_PORT, {"/help", HTTP_GET, service_get_help, NULL}                        },
    {DEFAULT_HTTP_PORT, {"/who", HTTP_GET, service_get_who, NULL}                          },
    {DEFAULT_HTTP_PORT, {"/json", HTTP_GET, service_get_json, NULL}                        },
    {DEFAULT_HTTP_PORT, {"/favicon.ico", HTTP_GET, service_get_issf_png, NULL}             },

    {EVENT_HTTP_PORT,   {"/", HTTP_GET, service_get_menu, (void *)&event_menu_ctx}         }, // Control back channel
    {EVENT_HTTP_PORT,   {"/menu", HTTP_GET, service_get_menu, (void *)&event_menu_ctx}     },
    {EVENT_HTTP_PORT,   {"/help", HTTP_GET, service_get_help, NULL}                        },
    {EVENT_HTTP_PORT,   {"/favicon.ico", HTTP_GET, service_get_issf_png, NULL}             },
    {0,                 {"", 0, NULL, NULL}                                                }
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
void register_services(httpd_handle_t server, // Pointer to active server
                       unsigned int   port)
{
  int i;

  i = 0;
  while ( uri_list[i].port != 0 )
  {
    if ( uri_list[i].port == port ) // Only register the services for this port
    {
      httpd_register_uri_handler(server, &uri_list[i].uri_struct);
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Registering %s on port %d", uri_list[i].uri_struct.uri, uri_list[i].port);))
    }

    i++;
  }
  return;
}

/*----------------------------------------------------------------
 *
 * @function: service_get_FreeETarget
 *
 * @brief:    Basic service to return the FreeETarget page
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * This function is called in response to a user starting a
 * shooting session.
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_FreeETarget(httpd_req_t *req)
{
  const char *resp_str;            // Reply to server
  char        my_name[SHORT_TEXT]; // Temporary string

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_FreeETarget(%s)", req->uri);))

  /*
   * Do the things we need to do to start a session
   */
  http_shot  = -1;   // Reset the shot counter
  event_mode = AUTO; // Set the server mode to auto refresh

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                             // Get the target name
  resp_str = (const char *)&FreeETarget_html_start; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_FreeETarget", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

  return ESP_OK;
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

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_events(%s) event_mode: %d", req->uri, event_mode);))

  /*
   *  First time through, send an empty score
   */
  if ( http_shot < 0 )                        // First time through
  {
    strcpy(str, "event:new_shotData\nid:\ndata: ");
    build_json_score(NULL, SCORE_HTTP_PRIME); // Send the new shot
    http_shot = 0;                            // Next time reply with the first shot
  }

  /*
   * Second time trought Send the next score
   */
  else
  {
    while ( (http_shot == shot_in)                        // Wait for a shot to arrive
            && (event_mode != CLOSE) )                    // or the event has finished
    {
      vTaskDelay(ONE_SECOND);
    }

    switch ( event_mode )                                 // Check the event mode
    {
      default:
      case IDLE:                                          // If the event is idle, then do nothing
        DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Shooting event idle");))
        strcpy(str, "event:idle\n");                      // Send an idle message
        break;                                            // Return and wait for the next shot

      case CLOSE:                                         // If the event is done
        DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Shooting event closed");))
        strcpy(str, "event:close\nid:\ndata: ");          // Send a close message
        break;

      case SINGLE:                                        // If the event is in single shot mode
      case AUTO:                                          // If the event is in auto mode
        DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "Shooting event auto mode");))
        strcpy(str, "event:new_shotData\nid:\ndata: ");
        build_json_score(&record[http_shot], SCORE_HTTP); // Send the new shot
        http_shot = (http_shot + 1) % SHOT_SPACE;         // and bump up next one
        break;
    }
  }

  /*
   * Prepare and send the payload
   */
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
 * @function: service_get_menu
 *
 * @brief:    Control the target over port 81
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 * This function is called in respoinse to a user starting a
 * shooting session.a
 *
 * Arguements
 *
 * MATCH - Start a match session
 * SIGHT - Start a sighting session
 * AUTO  - Set the server to auto refresh
 * SINGLE - Set the server to single shot mode
 * STOP  - Stop the server
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_menu(httpd_req_t *req)
{
  const char *resp_str;            // Reply to server
  char        my_name[SHORT_TEXT]; // Temporary string

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_menu(%s)", req->uri);))

  /*
   *  Decode the command line arguements if there are any
   */
#if ( 0 )
  if ( ((my_user_ctx_t *)(req->user_ctx))->port == DEFAULT_HTTP_PORT ) // Target can obly be started on port 80
  {
    if ( contains(req->uri, "start") || contains(req->uri, "START") )
    {
      start_new_session(SESSION_MATCH);
      /*
       * Do the things we need to do to start a session
       */
      http_shot  = -1;   // Reset the shot counter
      event_mode = AUTO; // Set the server mode to auto refresh

                         /*
                          *  Send the reply to the client
                          */
      target_name(my_name);                             // Get the target name
      resp_str = (const char *)&FreeETarget_html_start; // point to the target HTML file
      httpd_resp_set_hdr(req, "menu", my_name);
      httpd_resp_send(req, resp_str, strlen(resp_str));
      return ESP_OK;                                    // All done, return
    }
  }
#endif

  if ( contains(req->uri, "stop") || contains(req->uri, "STOP") )
  {
    event_mode = CLOSE; // Set the server mode to close
  }

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                      // Get the target name
  resp_str = (const char *)&menu_html_start; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_menu", my_name);
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

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_help(%s)", req->uri);))

  /*
   *  Send the reply to the client
   */
  target_name(my_name);                      // Get the target name
  resp_str = (const char *)&help_html_start; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_help", my_name);
  httpd_resp_send(req, resp_str, strlen(resp_str));

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

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_json(%s)", req->uri);))
  squish(req->uri, _xs);                  // Go through the uri and keep the argument portion
  tcpip_socket_2_queue(_xs, strlen(_xs)); // Put the data into the TCPIP queue

  /*
   * Set the header to indicate that this is a json request
   */
  target_name(&my_name);       // Get the target name
  httpd_resp_set_hdr(req, "get_json", my_name);
  http_send_string_start(req); // Start sending a string to the client
  vTaskDelay(ONE_SECOND);      // Give up time for the data to be processed
  http_send_string_end();      // Stop sending a string to the client

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
  const char *resp_str;                    // Reply to server
  char        my_name[SHORT_TEXT];         // Target name

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_issf_png(%s)", req->uri);))

  target_name(my_name);
  resp_str = (const char *)issf_png_start; // point to the target json file
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

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_who(%s)", req->uri);))

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
