/*----------------------------------------------------------------
 *
 * tcpip_helpers.h
 *
 * Header file for TCP/IP helper functions
 *
 *---------------------------------------------------------------*/
#ifndef _TCPIP_HELPERS_H_
#define _TCPIP_HELPERS_H_

/*
 * Global functions
 */
int  server_app_2_queue(char *buffer, int length);    // Save for later output to the socket
int  server_queue_2_socket(char *buffer, int length); // Take from queue and put to socket
int  server_socket_2_queue(char *buffer, int length); // Take from socket and queue
int  server_queue_2_app(char *buffer, int length);    // Take from queue and return to application
void server_flush(void);                              // Flush the server input queue
int  server_available(int port);                      // Find out if there is data waiting

/*
 * #defines
 */
typedef struct queue_struct
{
  char queue[1024]; // Holding queue
  int  in;          // Index of input characters
  int  out;         // Index of output characters
} queue_struct_t;

#endif
