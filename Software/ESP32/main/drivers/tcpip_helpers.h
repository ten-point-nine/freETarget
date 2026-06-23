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
int tcpip_app_2_queue(char *buffer, int length);     // Save for later output to the socket
int tcpip_queue_2_socket(char *buffer, int length);  // Take from queue and put to socket
int server_socket_2_queue(char *buffer, int length); // Take from socket and queue
int tcpip_queue_2_app(char *buffer, int length);     // Take from queue and return to application

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
