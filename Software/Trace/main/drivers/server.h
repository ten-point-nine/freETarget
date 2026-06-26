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
void server_accept_poll(void *parameters);   // Wait for a socket connection
//void server_send(void *pvParameters);        // Send data to connected clients
//void server_send(void *pvParameters);        // Send data to connected clients
void server_send(char * buffer, int length);  // Send data to connected clients
void server_receive_poll(void *parameters);  // Listen to TCPIP recv calls

void server_accept_poll(void *parameters);   // Wait for a socket connection

void WiFi_station_loopback_test(void);       // Station test for diag.c

#if ( 0 )
void server_socket_poll_1(void *parameters); // Listen to TCPIP recv calls
void server_socket_poll_2(void *parameters); // Listen to TCPIP recv calls
void server_socket_poll_3(void *parameters); // Listen to TCPIP recv calls
#endif
/*
 * #defines
 */

#endif
