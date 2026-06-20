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
void WiFi_station_init(void);                       // Initialize the WiFI as a station
void WiFi_loopback_test(void);                      // Loopback the TCPIP channel
bool WiFi_my_IP_address(char *s);                   // Return the current IP address
void WiFi_remote_IP_address(char *s);               // Return the current gateway address
void WiFi_MAC_address(char *mac);                   // Read the MAC address
void WiFi_setup(void);                              // Configure the WiFi operation
void WiFi_AP_scan_test(void);                       // Scan for access points (APs)
bool WiFi_client_init(void);                        // Setup the client
void WiFi_server_test(void);                        // Server test for diag.c
void WiFi_station_loopback_test(void);              // Station test for diag.c
void WiFi_AP_loopback_test(void);                   // Access point test for diag.c

void WiFi_client_task(void *params);                // Receive TCIP traffic
int  WiFi_available(void);                          // Number of characters waiting
int  WiFi_putch(char ch);                           // Output a charcter
char WiFi_getch(void);                              // Read a character
int  WiFi_puts(char *str, int length);              // Output a string to the WiFi
void WiFi_client_test(void);                        // Send and receive stuff from the target

int tcpip_app_2_queue(char *buffer, int length);    // Save for later output to the socket
int tcpip_queue_2_socket(char *buffer, int length); // Take from queue and put to socket
int tcpip_socket_2_queue(char *buffer, int length); // Take from socket and queue
int tcpip_queue_2_app(char *buffer, int length);    // Take from queue and return to application

/*
 * #defines
 */

#endif
