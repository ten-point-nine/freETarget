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
void serial_bt_config(unsigned int baud_rate);       // Initialize the Bluetooth port for operational configuration
void check_new_connection(void);                     // Check to see if a new connection has been made

/*
 *  Definitions
 *              CONSOLE   AUX    TCPIP
 */
#define CONSOLE        0x0001
#define AUX            0x0002
#define BLUETOOTH      0x0004
#define TCPIP_0        0x0008
#define TCPIP_1        0x0010
#define TCPIP_2        0x0020
#define TCPIP_3        0x0040
#define HTTP_CONNECTED 0x0080
#define TCPIP          (TCPIP_0 | TCPIP_1 | TCPIP_2 | TCPIP_3)
#define EVEN_ODD_BEGIN 0x0100 // Remember to output in even_odd mode
#define EVEN_ODD_END   0x0200 // Exit even odd mode

#define ALL  (CONSOLE | AUX | BLUETOOTH | TCPIP | HTTP_CONNECTED)
#define SOME (CONSOLE | TCPIP)

/*
 *  Global Variables
 */
extern unsigned int connection_list;

#endif
