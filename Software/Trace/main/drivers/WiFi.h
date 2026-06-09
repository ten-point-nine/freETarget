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
void WiFi_station_init(void);          // Initialize the WiFI as a station
void WiFi_loopback_test(void);         // Loopback the TCPIP channel
bool WiFi_my_IP_address(char *s);      // Return the current IP address
void WiFi_remote_IP_address(char *s);  // Return the current gateway address
void WiFi_MAC_address(char *mac);      // Read the MAC address
void WiFi_setup(void);                 // Configure the WiFi operation
void WiFi_AP_scan_test(void);          // Scan for access points (APs)
bool WiFi_client_init(void);           // Setup the client
void WiFi_server_test(void);           // Server test for diag.c
void WiFi_station_loopback_test(void); // Station test for diag.c
void WiFi_AP_loopback_test(void);      // Access point test for diag.c

void WiFi_client_send(void);           // Send a message out the TCPIP port
void WiFi_client_get(void);     // Receive a message from the TCPIP port

/*
 * #defines
 */

#endif
