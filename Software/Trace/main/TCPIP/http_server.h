/*----------------------------------------------------------------
 *
 * http_server.h
 *
 * Header file for http server function
 *
 *---------------------------------------------------------------*/
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

/*
 * Global functions
 */
httpd_handle_t start_webserver(unsigned int port);                                                // Turn it on and let it go
void           disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                  void *event_data);                                              // What to do when the server disconnects

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data); // What to do when a connection arrives
int  get_url_arg(char *req_url, char *reply, int sizeof_reply);
void http_send_string_start(httpd_req_t *req);                                                    // Start to send a string to the client
void http_send_string(const char *str);                                                           // String to send to the client
void http_send_string_end(); // Stop sending a string to the client and pump it out

/*
 * #defines
 */
#define DEFAULT_HTTP_PORT 80   // Use the stanard port 80 for HTTP requests
#define EVENT_HTTP_PORT   8080 // Use port 8080 for the event control
#endif
