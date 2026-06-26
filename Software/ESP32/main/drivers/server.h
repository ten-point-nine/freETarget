/*----------------------------------------------------------------
 *
 * server.h
 *
 * Header file for server functions
 *
 *---------------------------------------------------------------*/
#ifndef _SERVER_H_
#define _SERVER_H_

/*
 * Global functions
 */
void WiFi_server_init(void);                 // Start the WiFi server
void WiFi_station_loopback_test(void);       // Station test for diag.c

void server_send(void *pvParameters);        // Send data to connected clients
void server_accept(void *parameters);        // Wait for a socket connection
void server_receive(void *parameters);       // Listen to TCPIP recv calls
                                             // Flush the server input buffers
#if ( 0 )
void server_socket_poll_1(void *parameters); // Listen to TCPIP recv calls
void server_socket_poll_2(void *parameters); // Listen to TCPIP recv calls
void server_socket_poll_3(void *parameters); // Listen to TCPIP recv calls
#endif
/*
 * #defines
 */

#endif
