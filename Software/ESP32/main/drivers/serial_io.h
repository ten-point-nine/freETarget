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
#define CONSOLE        0x0001                // 0x1
#define AUX            (CONSOLE << 1)        // 0x2
#define BLUETOOTH      (AUX + 2)             // 0x4
#define RS485          (BLUETOOTH + 2)       // 0x6
#define TCPIP_0        (CONSOLE << 4)        // 0x10
#define TCPIP_1        (TCPIP_0 << 1)
#define TCPIP_2        (TCPIP_1 << 1)
#define TCPIP_3        (TCPIP_2 << 1)
#define HTTP_CONNECTED (TCPIP_3 << 1)
#define EVEN_ODD_BEGIN (HTTP_CONNECTED << 1) // Remember to output in even_odd mode
#define EVEN_ODD_END   (EVEN_ODD_BEGIN << 1) // Exit even odd mode

#define ALL      (CONSOLE | AUX_PORT | TCPIP | HTTP_CONNECTED)
#define TCPIP    (TCPIP_0 | TCPIP_1 | TCPIP_2 | TCPIP_3)
#define SOME     (CONSOLE | TCPIP)
#define AUX_PORT (AUX | BLUETOOTH | RS485)
/*
 *  Global Variables
 */
extern unsigned int connection_list;

#endif
