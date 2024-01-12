/*----------------------------------------------------------------
 *
 * gpio.h
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _SERIAL_IO_H_
#define _SERIAL_IO_H_

/*
 * Global functions
 */
void serial_io_init(void);                                        // Initialize the Serial ports
void serial_to_all(char* s, bool console, bool aux, bool tcpip);  // Multipurpose driver
void serial_putch(char ch, bool console, bool aux, bool tcpip);   // Output a single character
char serial_gets(bool console, bool aux, bool tcpip);             // Read from all of the ports
char serial_getch(bool console, bool aux, bool tcpip);            // Read the selected port
unsigned int serial_available(bool console, bool aux, bool tcpip);// Find out how much is waiting for us
void serial_flush(bool console, bool aux, bool tcpip);            // Get rid of everything
int tcpip_app_2_queue(char* buffer, int length);                  // Save for later output to the socket  
int tcpip_queue_2_socket(char* buffer, int length);               // Take from queue and put to socket
int tcpip_socket_2_queue(char* buffer, int length);               // Take from socket and queue
int tcpip_queue_2_app(char* buffer, int length);                  // Take from queue and return to application
void serial_port_test(void);                                      // Loopback the AUX port

/*
 *  Definitions
 */
#define CONSOLE true,  false, false
#define AUX     false, true,  false
#define TCPIP   false, false, true
#define ALL     true,  false,  false

#endif
