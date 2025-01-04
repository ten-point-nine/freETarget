/*----------------------------------------------------------------
 *
 * http_server.h
 *
 * Header file for http server function
 *
 *---------------------------------------------------------------*/
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVEr_H_

/*
 * Global functions
 */
httpd_handle_t start_webserver(void);                                                             // Turn it on and let it go
void           disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                  void *event_data);                                              // What to do when the server disconnects

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data); // What to do when a connection arrives

/*
 * #defines
 */
#define DEFAULT_SERVER_PORT 80 // Use the stanard port 80 for HTTP requests
#endif
