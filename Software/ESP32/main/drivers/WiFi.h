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
void WiFi_init(void);                  // Initialize the WiFi
#if ( INCLUDE_WIFI_AP )
void WiFi_AP_init(void);               // Initialize the WiFi as an Access Point
#endif
#if ( INCLUDE_WIFI_STATION )
void WiFi_station_init(void);          // Initialize the WiFI as a station
#endif
void WiFi_loopback_test(void);         // Loopback the TCPIP channel
bool WiFi_my_IP_address(char *s);      // Return the current IP address
void WiFi_remote_IP_address(char *s);  // Return the current gateway address
void WiFi_MAC_address(char *mac);      // Read the MAC address
void WiFi_setup(void);                 // Configure the WiFi operation
void WiFi_AP_scan_test(void);          // Scan for access points (APs)
void WiFi_pingpong_test(void);         // Ping pong the signals
void WiFi_reconnect(void);             // Try and start a new connection.

#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
bool WiFi_get_remote_IP(char *url);            // Get the IP address of the remote URL
void http_DNS_test(void);                      // Exercise the DNS lookup
#endif

void WiFi_station_loopback_test(void);         // Station test for diag.c
void WiFi_AP_loopback_test(void);              // Access point test for diag.c

void WiFi_trace_test(void);                    // Send messages to the trace device
void WiFi_show_connections(void);              // Show who is attached to us

                                               /*
                                                * #defines
                                                */

typedef struct socket_description
{
  int  handle;                            // Socket handle
  char ip[sizeof("192.168.123.123") + 2]; // Text of remote IP
  bool is_trace;                          // This is a trace device
} socket_description_t;

#endif
