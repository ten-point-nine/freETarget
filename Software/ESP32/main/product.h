/*----------------------------------------------------------------
 *
 * product.h
 *
 * Definition of how the product is constructed
 *
 *--------------------------------------------------------------*/

#ifndef _PRODUCT_H
#define _PRODUCT_H

#define SOFTWARE_VERSION "\"6.5.0 June 14, 2026\""

#define BUILD_TRACE          (0 == 1) // Build the trace module
#define BUILD_TARGET         (1 == 1) // Build the target module
#define INCLUDE_WIFI_STATION (1 == 1) // Include the code to make a station
#define INCLUDE_WIFI_AP      (1 == 1) // Trace is always a station
#define INCLUDE_CLIENT       (0 == 1) // Include Client network software
#define INCLUDE_SERVER       (1 == 1) // Include Server network software
#define INCLUDE_MDNS         (1 == 1) // Build the mDNS module

#endif
