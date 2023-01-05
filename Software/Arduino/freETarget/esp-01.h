/*----------------------------------------------------------------
 *
 * esp-01.h
 *
 * WiFi Driver for esp-01
 *. 
 *---------------------------------------------------------------*/

#ifndef _ESP_01_H_
#define _ESP_01_H_

/*
 * Function Prototypes
 */
void         esp01_init(void);                    // Initialize the device
bool         esp01_restart(void);                 // Take control and reset the device

char         esp01_read(void);                    // Read a character from the queuue
unsigned int esp01_available(void);               // Return the number of available characters
bool         esp01_send(char* str, int channel);  // Send out a string to the channel
void         esp01_receive(void);                 // Take care of receiving characters from the IP channel
bool         esp01_connected(void);               // TRUE if any channel is connected.
bool         esp01_is_present(void);              // TRUE if an ESP-01 was found
void         esp01_test(void);                    // Diagnostic self test
void         esp01_status(void);                  // Read the device status
void         esp01_close(unsigned int channel);   // Close this connection
void         esp01_status(void);                  // Get the status of the esp01
void         esp01_broadcast(void);               // Just blab out a count
void         esp01_myIP(char* s);                 // Obtain the working IP address

/*
 * Definitions
 */
#define esp01_N_CONNECT      3                    // Allow up to 3 connections
#define esp01_MAX_POWER     80                    // Set the max power to 80dBM
#define esp01_MAX_WAITOK  2000                    // Wait for 2 seconds for the OK to come back
#define esp01_BUFFER_SIZE  2048                   // The ESP buffer size

#define esp01_SSID_SIZE     (16+1)                // Give 16+1 characters for SSID *** DO NOT UDE **
#define esp01_SSID_SIZE_32  (32+1)                // Give 32+1 characters for SSID
#define esp01_PWD_SIZE      17                    // Give 16+1 characters for PWD
#define esp01_IP_SIZE       17                    // Use 192.168.100.100 for IP
#endif
