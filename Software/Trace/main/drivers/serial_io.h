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

/*
 *  Definitions
 *              CONSOLE   AUX    TCPIP
 */
#define CONSOLE        0x0001                // 0x1
#define TCPIP          (CONSOLE << 1)        // 0x2
#define CLIENT         (TCPIP << 1)          // 0x0x02
#define EVEN_ODD_BEGIN (CLIENT << 1)         // Remember to output in even_odd mode
#define EVEN_ODD_END   (EVEN_ODD_BEGIN << 1) // Exit even odd mode
#define ALL            (CONSOLE | TCPIP)

#define DEFAULT_BAUD_RATE 115200             // Standard development baud rate for the console port
#define MAX_BAUD_RATE     921600             // Maximum baud rate for the console port

/*
 *  Global Variables
 */

#endif
