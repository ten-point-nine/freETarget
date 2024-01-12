/*----------------------------------------------------------------
 *
 * WiFi.
 *
 * Header file for WiFi functions
 *
 *---------------------------------------------------------------*/
#ifndef _WIFI_H_
#define _WIFI_H_

/*
 * Global functions
 */
void WiFi_init(void);                         // Initialize the WiFi
void WiFi_AP_init(void);                      // Initialize the WiFi as an Access Point
void WiFi_station_init(void);                 // Initialize the WiFI as a station
void WiFi_tcp_server_task(void *pvParameters);// TCP Server task
void WiFi_loopback_test(void);                // Loopback the TCPIP channel
void WiFi_my_ip_address(char* s);             // Return the current IP address 

void tcpip_socket_poll_0(void* parameters);   // Listen to TCPIP recv calls 
void tcpip_socket_poll_1(void* parameters);   // Listen to TCPIP recv calls 
void tcpip_socket_poll_2(void* parameters);   // Listen to TCPIP recv calls 
void tcpip_socket_poll_3(void* parameters);   // Listen to TCPIP recv calls 

void tcpip_accept_poll(void* parameters);     // Wait for a socket connection


/*
 * #defines
 */

#endif
