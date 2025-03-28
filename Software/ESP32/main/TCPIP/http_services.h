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
 * Global functions
 */
void                     register_services(httpd_handle_t server); // Pointer to active server
extern const httpd_uri_t uri_list[];                               // List of active URLs

/*
 * Global Variables
 */
extern int http_shot; // What shot are we procesing for the web based output
#endif
