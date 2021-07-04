/*----------------------------------------------------------------
 *
 * esp-01.h
 *
 * WiFi Driver for esp-01
 *
 *---------------------------------------------------------------*/

#ifndef _ESP_01_H_
#define _ESP_01_H_

/*
 * Function Prototypes
 */
void esp01_init(void);                            // Initialize the device
bool esp01_restart(void);                         // Take control and reset the device

char         esp01_read(void);                    // Read a character from the queuue
unsigned int esp01_available(void);               // Return the number of available characters
void         esp01_send(bool start, int index);   // Start or end a send sequence on which connection
void         esp01_receive(void);                 // Take care of receiving characters from the IP channel
bool         esp01_connected(int channel);        // TRUE if the channel is connected.
bool         esp01_is_present(void);              // TRUE if an ESP-01 was found

/*
 * Definitions
 */
#define MAX_CONNECTIONS 3                         // Allow up to 3 connections

#endif
