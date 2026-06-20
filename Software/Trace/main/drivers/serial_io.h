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
void serial_io_init(void);                          // Initialize the Console Port
void serial_to_all(char *s, int ports);             // Multipurpose driver
void serial_putch(char ch, int ports);              // Output a single character
char serial_getch(int ports);                       // Read the selected port
int  serial_available(int ports);                   // Find out how much is waiting for us
int  serial_who(void);                              // Determine WHO is trying to talk to us
void serial_flush(int ports);                       // Get rid of everything



int  get_string(char destination[], int size);      // Collect a string from the input ports
void check_new_connection(void);                    // Check to see if a new connection has been made

#if ( 0 )
int tcpip_app_2_queue(char *buffer, int length);    // From the application to a buffer  (application level)
int tcpip_queue_2_socket(char *buffer, int length); // From the buffer to the application (driver level)

int tcpip_socket_2_queue(char *buffer, int length); // From socket to the queue (driver level)
int tcpip_queue_2_app(char *buffer, int length);    // Take from queue and return to application (application level)
#endif

/*
 *  Definitions
 *              CONSOLE   AUX    TCPIP
 */
#define CONSOLE        0x0001                // 0x1
#define TARGET         (CONSOLE << 1)        // 0x20
#define EVEN_ODD_BEGIN (TARGET << 1)         // Remember to output in even_odd mode
#define EVEN_ODD_END   (EVEN_ODD_BEGIN << 1) // Exit even odd mode
#define ALL            (CONSOLE | TARGET)

#define DEFAULT_BAUD_RATE 115200             // Standard development baud rate for the console port
#define MAX_BAUD_RATE     921600             // Maximum baud rate for the console port

/*
 *  Global Variables
 */

#endif
