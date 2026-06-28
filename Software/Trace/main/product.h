/*----------------------------------------------------------------
 *
 * product.h
 *
 * Definition of how the product is constructed
 *
 *--------------------------------------------------------------*/

#ifndef _PRODUCT_H
#define _PRODUCT_H

#define USE_ICM45686         (1 == 1) // Use the ICM45686 sensor
#define USE_BMI270           (0 == 1) // Use the BMI270 sensor
#define BUILD_TRACE          (1 == 1) // Build the trace module
#define BUILD_TARGET         (0 == 1) // Build the target module
#define INCLUDE_WIFI_STATION (1 == 1) // Include the code to make a station
#define INCLUDE_WIFI_AP      (0 == 1) // Trace is always a station
#define INCLUDE_CLIENT       (1 == 1) // Include Client network software
#define INCLUDE_SERVER       (1 == 1) // Include Server network software
#define INCLUDE_MDNS         (0 == 1) // Build the mDNS module

#if USE_BMI270
#define IMU_INIT(x) BMI270_init(X)
#endif

#if USE_ICM45686
#define IMU_INIT(x) ICM45686_init(x)
#endif

#define SOFTWARE_VERSION "\"1.0.0 June 13, 2026\""

#endif
