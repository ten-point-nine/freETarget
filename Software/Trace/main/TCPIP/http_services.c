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

#include "trace.h"

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
  CLOSE,     // 3 Close the event
  START      // 4 Start the event
} event_mode_t;

/*
 *  Variables
 */
int                 http_shot  = -1;   // What shot number have we sent?
static event_mode_t event_mode = IDLE; // Set the server mode to auto refresh
static int          check_mask = 0;    // Mask of the checkboxes

typedef struct
{
  unsigned int port;
} my_user_ctx_t;                       // Internal user context structure

static const my_user_ctx_t http_events_ctx = {DEFAULT_HTTP_PORT};

/*
 * Local functions
 */
static esp_err_t stop_webserver(httpd_handle_t server);
static esp_err_t service_get_who(httpd_req_t *req);                                    // Target information page
static esp_err_t service_get_FreeETarget_png(httpd_req_t *req);                        // Icon for the FreeETarget page
static esp_err_t service_get_trace(httpd_req_t *req);                                  // Get the last trace
static esp_err_t service_get_events(httpd_req_t *req);                                 // Get shot events
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);       // Create a URL not found handler
static esp_err_t service_post_post(httpd_req_t *req);
static void      http_printf(const char *format, httpd_req_t *req, unsigned int size); // Formatted print to the HTTP request

/*
 *  URI handlers
 */
const my_uri_t uri_list[] = {
    {DEFAULT_HTTP_PORT, "",            {"/trace", HTTP_GET, service_get_trace, (void *)&http_events_ctx}},
    {DEFAULT_HTTP_PORT, "Target info", {"/who", HTTP_GET, service_get_who, NULL}                        },
    {DEFAULT_HTTP_PORT, "",            {"/favicon.ico", HTTP_GET, service_get_FreeETarget_png, NULL}    },

    {0,                 "",            {"", 0, NULL, NULL}                                              }
};

/*
 *  Definitions
 */

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
static esp_err_t service_get_trace(httpd_req_t *req)
{
  const char *resp_str;            // Reply to server
  char        my_name[SHORT_TEXT]; // Temporary string

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_trace(%s)", req->uri);))

  /*
   * Do the things we need to do to start a session
   */
  http_shot  = -1;   // Reset the shot counter
  event_mode = AUTO; // Set the server mode to auto refresh

  /*
   *  Send the reply to the client
   */
  resp_str = (const char *)&FreeETarget_html_start; // point to the target HTML file
  httpd_resp_set_hdr(req, "get_FreeETarget", my_name);
  httpd_resp_send(req, resp_str, SIZEOF_FreeETarget_HTML);

  return ESP_OK;
}


/*----------------------------------------------------------------
 *
 * @function: service_get_FreeETarget_png
 *
 * @brief:    Send the FreeETarget icon file to the client
 *
 * @return:   esp_err_t, error type
 *
 *---------------------------------------------------------------
 *
 *
 *------------------------------------------------------------*/
static esp_err_t service_get_FreeETarget_png(httpd_req_t *req)
{
  const char *resp_str;                           // Reply to server
  char        my_name[SHORT_TEXT];                // Target name

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_FreeETargetIcon_png(%s)", req->uri);))

  resp_str = (const char *)FreeETarget_png_start; // point to the target json file
  httpd_resp_set_hdr(req, "get_FreeETargetIcon_png", my_name);
  httpd_resp_send(req, resp_str, SIZEOF_FREEETARGET_PNG);

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
  #if(0)
  char                   my_name[SHORT_TEXT];
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  esp_app_desc_t         running_app_info;
  static int             cycle_count = 0;

  DLT(DLT_HTTP, SEND(ALL, sprintf(_xs, "service_get_who(%s)", req->uri);))

  esp_ota_get_partition_description(running_partition, &running_app_info);

  target_name(my_name); // Get the target name
  httpd_resp_set_hdr(req, "get_who", my_name);

  sprintf(_xs,
          "Serial Number: %d"
          "<br>Target ID: %s"
          "<br>Version: %s"
          "<br>Cycle count: %d",
          json_serial_number, my_name, SOFTWARE_VERSION, running_app_info.version, json_athlete, json_target_name, json_event,
          cycle_count); // Fill in the target name

  httpd_resp_send(req, _xs, HTTPD_RESP_USE_STRLEN);
  cycle_count++;
  #endif 
  return ESP_OK;
}
/*----------------------------------------------------------------
 *
 * @function: http_printf
 *
 * @brief:    Local version of printf to send data to the client
 *
 * @return:   None
 *
 *---------------------------------------------------------------
 *
 * Kluged up 'printf' to send data to an HTTP client. This allows
 * the programmer to incorporate variable data in the reply to
 * the client.
 *
 * The format string is a text string that contains formatting
 * fields that are replaced with the data.
 *
 * For example: %A will be replaced with the string "SELECTED"
 * if the radio button has been SELECTED.
 *
 *------------------------------------------------------------*/
#define SELECTED                                                                                                                           \
  strcat(_xs, " selected ");                                                                                                               \
  i = strlen(_xs);

static void http_printf(const char  *format, // Format string
                        httpd_req_t *req,    // HTTP request to return to
                        unsigned int size    // Size of the format string
)
{
  int          i;
  unsigned int remainder;                    // Number of bytes remaining to be processed

  _xs[0]    = 0;
  i         = 0;
  remainder = size;
  /*
   *  Loop and output the format string
   */

  while ( remainder != 0 )
  {
    if ( *format == '^' )             // Look for a format specifier
    {
      format++;                       // Skip the %
      remainder--;                    // Decrement the number of bytes remaining

      switch ( *format )
      {
        case '^':
          strcat(_xs, "^");           // If we have a double ^ then just output a single ^
          break;


        default:                      // Unknown format
          break;
      }
      format++;
      remainder--;                    // Decrement the number of bytes remaining
    }
    else
    {
      _xs[i++] = *format++;           // Copy the character to the buffer
      remainder--;                    // Decrement the number of bytes remaining
    }
    _xs[i] = 0;                       // Null terminate the string

    /*
     *  See if it is time to send out the buffer.
     */
    if ( i >= sizeof(_xs) - 512 ) // If we have almost filled the buffer?
    {
      printf("%s", _xs);          // Print the buffer to the console
      httpd_resp_send_chunk(req, _xs, i);
      i      = 0;                 // Reset the index
      _xs[0] = 0;                 // Null terminate the string
    }
  }

  /*
   *  Finished Send out the last
   */
  if ( i != 0 )                         // If we have something to send
  {
    _xs[i] = 0;                         // Null terminate the string
    httpd_resp_send_chunk(req, _xs, i); // Send the string to the client
  }
  httpd_resp_send_chunk(req, NULL, 0);  // End the response
  return;
}
