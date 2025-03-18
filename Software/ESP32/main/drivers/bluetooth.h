/*----------------------------------------------------------------
 *
 * bluetooth.h
 *
 * Header file for Bluetooth.c
 *
 *---------------------------------------------------------------*/
#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

/*
 * Global functions
 */

void BlueTooth_configuration(void);        // Configure the BlueTooth module
void Bluetooth_start_new_connection(void); // New Bluetooth connection

/*
 * Definitions
 */
#define BUILD_HC_05 false // Build for the HC-05 module
#define BUILD_HC_06 true  // Build for the HC-06 module
#endif
