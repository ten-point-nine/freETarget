/*----------------------------------------------------------------
 *
 * client.h
 *
 * Header file for client functions
 *
 *---------------------------------------------------------------*/
#ifndef _CLIENT_H_
#define _CLIENT_H_

/*
 * Global functions
 */
bool WiFi_client_init(void);                // Initialize the WiFi client
void client_recv(void *params);             // Receive TCIP traffic
void client_send(char *buffer, int length); // Send TCIP traffic
void WiFi_client_test(void);                // Send and receive stuff from the target

/*
 * #defines
 */

#endif
