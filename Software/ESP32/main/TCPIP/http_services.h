/*----------------------------------------------------------------
 *
 * http_services.h
 *
 * HTTP services supported by the target
 *
 *---------------------------------------------------------------*/
#ifndef _HTTP_SERVICES_H_
#define _HTTP_SERVICES_H_

/*
 * Typedefs
 */
typedef struct my_uri      // Internal URI structure
{
  unsigned int port;       // Port number uri is listening on
  char        *help;
  httpd_uri_t  uri_struct; // URI structure
} my_uri_t;

/*
 * Global functions
 */
void register_services(httpd_handle_t server, unsigned int port); // Pointer to active server
#ifndef HTTP_SERVICES_C
extern const my_uri_t uri_list[];                                 // List of active URLs
#endif

/*
 * Global Variables
 */
extern int http_shot; // What shot are we procesing for the web based output
#endif
