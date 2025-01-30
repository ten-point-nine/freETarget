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
void serial_io_init(void);                           // Initialize the Console Port
void serial_aux_init(void);                          // Initialize the AUX port
void serial_to_all(char *s, int ports);              // Multipurpose driver
void serial_putch(char ch, int ports);               // Output a single character
char serial_gets(int ports);                         // Read from all of the ports
char serial_getch(int ports);                        // Read the selected port
int  serial_available(int ports);                    // Find out how much is waiting for us
int  serial_who(void);                               // Determine WHO is trying to talk to us
void serial_flush(int ports);                        // Get rid of everything
int  tcpip_app_2_queue(char *buffer, int length);    // Save for later output to the socket
int  tcpip_queue_2_socket(char *buffer, int length); // Take from queue and put to socket
int  tcpip_socket_2_queue(char *buffer, int length); // Take from socket and queue
int  tcpip_queue_2_app(char *buffer, int length);    // Take from queue and return to application
void serial_port_test(void);                         // Loopback the AUX port
bool get_string(char destination[], int size);       // Collect a string from the input ports

/*
 *  Definitions
 *              CONSOLE   AUX    TCPIP
 */
#define TCPIP_0        0x01
#define TCPIP_1        0x02
#define TCPIP_2        0x04
#define TCPIP_3        0x08
#define TCPIP          (TCPIP_0 + TCPIP_1 + TCPIP_2 + TCPIP_3)
#define CONSOLE        0x10
#define AUX            0x20
#define BLUETOOTH      0x40
#define EVEN_ODD_BEGIN 0x81 // Remember to output in even_odd mode
#define EVEN_ODD_END   0x80 // Exit even odd mode

#define ALL (CONSOLE + AUX + TCPIP + BLUETOOTH)

#endif
