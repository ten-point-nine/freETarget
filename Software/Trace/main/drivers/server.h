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
void WiFi_AP_scan_test(void);          // Scan for access points (APs)
void WiFi_server_test(void);           // Server test for diag.c
void WiFi_station_loopback_test(void); // Station test for diag.c
void WiFi_AP_loopback_test(void);      // Access point test for diag.c

/*
 * #defines
 */

#endif
