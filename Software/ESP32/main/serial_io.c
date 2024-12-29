/*-------------------------------------------------------
 *
 * serial_io.c
 *
 * General purpose Serial port driver
 *
 *-------------------------------------------------------
 *
 * https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/peripherals/uart.html
 *
 *------------------------------------------------------*/

#include "stdbool.h"
#include "stdio.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "serial_io.h"
#include "timer.h"
#include "json.h"

/*
 *  Serial IO port configuration
 */
const int           uart_console        = UART_NUM_0;
const uart_config_t uart_console_config = {.baud_rate           = 115200,
                                           .data_bits           = UART_DATA_8_BITS,
                                           .parity              = UART_PARITY_DISABLE,
                                           .stop_bits           = UART_STOP_BITS_1,
                                           .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
                                           .rx_flow_ctrl_thresh = 122,
                                           .source_clk          = UART_SCLK_DEFAULT};
const int           uart_console_size   = (1024 * 2);
QueueHandle_t       uart_console_queue;

const int           uart_aux        = UART_NUM_1;
const uart_config_t uart_aux_config = {.baud_rate           = 115200,
                                       .data_bits           = UART_DATA_8_BITS,
                                       .parity              = UART_PARITY_DISABLE,
                                       .stop_bits           = UART_STOP_BITS_1,
                                       .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
                                       .rx_flow_ctrl_thresh = 122,
                                       .source_clk          = UART_SCLK_DEFAULT};

const int     uart_aux_size = (1024 * 2);
QueueHandle_t uart_aux_queue;

typedef struct queue_struct
{
  char queue[1024];               // Holding queue
  int  in;                        // Index of input characters
  int  out;                       // Index of output characters
} queue_struct_t;

static queue_struct_t in_buffer;  // TCPIP input buffer
static queue_struct_t out_buffer; // TCPIP input buffer

/******************************************************************************
 *
 * @function: serial_io_init
 *
 * @brief: Initalize the Console port
 *
 * @return: None
 *
 *******************************************************************************
 *
 * The serial port is initialized and the interrupt
 * driver assigned
 *
 ******************************************************************************/

void serial_io_init(void)
{

  /*
   *  Load the driver
   */
  uart_driver_install(UART_NUM_0, uart_console_size, uart_console_size, 10, &uart_console_queue, 0);

  /*
   *  Setup the communications parameters
   */
  uart_param_config(uart_console, &uart_console_config);
  setvbuf(stdout, NULL, _IONBF, 0); // Send something out as soon as you get it

  /*
   *  Prepare the TCPIP queues
   */
  in_buffer.in   = 0; // Queue pointers
  in_buffer.out  = 0;
  out_buffer.in  = 0; // Queue pointers
  out_buffer.out = 0;

  /*
   * All done, return
   */
  return;
}

void serial_aux_init(void)
{
  if ( json_aux_port_enable == false )
  {
    return;
  }

  /*
   *  Load the driver
   */
  uart_driver_install(UART_NUM_1, uart_aux_size, uart_aux_size, 10, &uart_aux_queue, 0);

  /*
   *  Setup the communications parameters
   */
  uart_param_config(uart_aux, &uart_aux_config);

  /*
   *  Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
   */
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  /*
   * All done, return
   */
  return;
}

/*******************************************************************************
 *
 * @function: serial_available
 *
 * @brief:    Determine if bytes are waiting in any port
 *
 * @return:   Number of characters in all of the ports
 *
 *******************************************************************************
 *
 *
 ******************************************************************************/
int serial_available(int ports // Bit mask of active ports
)
{
  int n_available;
  int length;

  n_available = 0;

  if ( ports & CONSOLE )
  {
    uart_get_buffered_data_len(uart_console, (size_t *)&length);
    n_available += length;
  }

  if ( (json_aux_port_enable == true) // Is there hardware on the Aux port?
       && (ports & AUX) )             // Are we reading the AUX port
  {
    uart_get_buffered_data_len(uart_aux, (size_t *)&length);
    n_available += length;
  }

  if ( ports & TCPIP )
  {
    if ( in_buffer.in != in_buffer.out )
    {
      length = in_buffer.in - in_buffer.out;
      if ( length < 0 )
      {
        length += sizeof(in_buffer.queue);
      }
      n_available += length;
    }
  }

  /*
   * Return the number of characters waiting
   */
  return n_available;
}

/*******************************************************************************
 *
 * @function: serial_who
 *
 * @brief:    Determine WHICH serial channel is active
 *
 * @return:   Active serial channel
 *
 *******************************************************************************
 *
 * Look at each of the queues and return the queue number which has data ready
 * to be read.
 *
 ******************************************************************************/
int serial_who(void)
{
  int length;

  uart_get_buffered_data_len(uart_console, (size_t *)&length);
  if ( length != 0 )
  {
    return CONSOLE;
  }

  uart_get_buffered_data_len(uart_aux, (size_t *)&length);
  if ( length != 0 )
  {
    return AUX;
  }

  if ( in_buffer.in != in_buffer.out )
  {
    return TCPIP;
  }

  return 0;
}

/*******************************************************************************
 *
 * @function: serial_flush
 *
 * @brief:    purge everything in the queue
 *
 * @return:   Number of characters in all of the ports
 *
 *******************************************************************************
 *
 *
 ********************************************************************************/
void serial_flush(int ports // active port list
)
{
  if ( ports & CONSOLE )
  {
    uart_flush(uart_console);
  }

  if ( (json_aux_port_enable == true) // Is there hardware on the Aux port?
       && (ports & AUX) )             // Are we reading the AUX port
  {
    uart_flush(uart_aux);
  }

  if ( ports & TCPIP )
  {
    in_buffer.in  = 0;
    in_buffer.out = 0;
  }
  return;
}

/*******************************************************************************
 *
 * @function: serial_getch
 *
 * @brief:    Read one or more of the serial ports
 *
 * @return:   Next character in the  selected serial port
 *
 *******************************************************************************
 *
 *
 *******************************************************************************-*/
char serial_getch(int ports // Bit mask of active ports
)
{
  char ch;

  /*
   * Bring in the console bytes
   */
  if ( ports & CONSOLE )
  {
    if ( uart_read_bytes(uart_console, &ch, 1, 0) > 0 )
    {
      return ch;
    }
  }

  /*
   *  Bring in the AUX bytes
   */
  if ( (json_aux_port_enable == true) // Is there hardware on the Aux port?
       && (ports & AUX) )             // Are we reading the AUX port
  {
    if ( uart_read_bytes(uart_aux, &ch, 1, 0) > 0 )
    {
      return ch;
    }
  }

  /*
   *  Bring in the TCPIP bytes
   */
  if ( ports & TCPIP )
  {
    if ( tcpip_queue_2_app(&ch, 1) > 0 )
    {
      return ch;
    }
  }

  /*
   * Got nothing
   */
  return 0;
}

/*******************************************************************************
 *
 * @function: serial_putch
 *
 * @brief:    Send a string to the available serial ports
 *
 * @return:   None
 *
 *******************************************************************************
 *
 * Send a string to all of the serial devices that are
 * in use.
 *
 ******************************************************************************/
void serial_putch(char ch,
                  int  ports // Bitmask of active ports
)
{

  /*
   * Output to the devices
   */
  if ( ports & CONSOLE )
  {
    printf("%c", ch);                 // Must be printf
  }

  if ( (json_aux_port_enable == true) // Is there hardware on the Aux port?
       && (ports & AUX) )             // Are we reading the AUX port
  {
    uart_write_bytes(uart_aux, (const char *)&ch, 1);
  }

  if ( ports & TCPIP )
  {
    tcpip_app_2_queue(&ch, 1);
  }

  /*
   * All done
   */
  return;
}

void serial_to_all(char *str,  // String to output
                   int   ports // List of active ports
)
{
  unsigned int length;

  /*
   *  Figure out the string length
   */
  length = 0;
  while ( str[length] )
  {
    length++;
  }

  /*
   * Output to the devices
   */
  if ( ports & CONSOLE )
  {
    printf("%s", str);                // Must be printf
  }

  if ( (json_aux_port_enable == true) // Is there hardware on the Aux port?
       && (ports & AUX) )             // Are we reading the AUX port
  {
    uart_write_bytes(uart_aux, (const char *)str, length);
  }

  if ( ports & TCPIP )
  {
    tcpip_app_2_queue(str, length);
  }

  /*
   * All done
   */
  return;
}

/*******************************************************************************
 *
 * @function: tcpip_app_2_queue
 *
 * @brief:    Put something into the output queue for later transmission
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * This function is called by the application to save data into the
 * TCPIP queue for later output onto the TCPIP channel
 *
 ******************************************************************************/
int tcpip_app_2_queue(char *buffer, // Where to return the bytes
                      int   length  // Maximum transfer size
)
{
  int bytes_moved;                  // Number of bytes written

  bytes_moved = 0;
  while ( length != 0 )
  {
    out_buffer.queue[out_buffer.in] = *buffer;
    buffer++;
    length--;
    bytes_moved++;
    out_buffer.in = (out_buffer.in + 1) % sizeof(out_buffer.queue);
  }

  /*
   *  All done, return the number of bytes written to the queue
   */
  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_queue_2_socket
 *
 * @brief:    Take waiting bytes out of the queue and into the socket
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * This function is the companion to tcpip_app_t_queue that finished sending
 * the data out to the socket
 *
 ******************************************************************************/
int tcpip_queue_2_socket(char *buffer, // Place to put data
                         int   length  // Number of bytes to read
)
{
  int bytes_moved;                     // Number of bytes read from queue

  if ( out_buffer.out == out_buffer.in )
  {
    return 0;                          // Nothing to say
  }

  bytes_moved = 0;

  while ( length != 0 )
  {
    *buffer = out_buffer.queue[out_buffer.out];
    buffer++;
    length--;
    bytes_moved++;
    out_buffer.out = (out_buffer.out + 1) % sizeof(out_buffer.queue);
    if ( out_buffer.out == out_buffer.in )
    {
      break; // RUn out of things to read
    }
  }

  /*
   *  All done, return the number of bytes written to the queue
   */
  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_queue_2_app
 *
 * @brief:    Read data out of the queue and return it to the application
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * Characters from the TCPIP input queue are returned to the application
 *
 ******************************************************************************/
int tcpip_queue_2_app(char *buffer, // Where to return the bytes
                      int   length  // Maximum transfer size
)
{
  int bytes_moved;

  bytes_moved = 0;
  if ( in_buffer.out == in_buffer.in )
  {
    return 0; // Nothing waiting for us
  }

  while ( length )
  {
    *buffer = in_buffer.queue[in_buffer.out];
    buffer++;
    length--;
    bytes_moved++;
    in_buffer.out = (in_buffer.out + 1) % sizeof(in_buffer.queue);
    if ( in_buffer.out == in_buffer.in )
    {
      break; // Reached the end
    }
  }

  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: tcpip_socket_2_queue
 *
 * @brief:    Put fresh TCPIP data into the queue for later
 *
 * @return:   Buffer updated
 *
 *******************************************************************************
 *
 * Characters from the TCPIP input queue are returned to the application
 *
 ******************************************************************************/
int tcpip_socket_2_queue(char *buffer, // Where to return the bytes
                         int   length  // Maximum transfer size
)
{
  int bytes_moved;

  bytes_moved = 0;
  while ( length )
  {
    in_buffer.queue[in_buffer.in] = *buffer;
    buffer++;
    length--;
    bytes_moved++;
    in_buffer.in = (in_buffer.in + 1) % sizeof(in_buffer.queue);
    if ( in_buffer.out == in_buffer.in )
    {
      DLT(DLT_CRITICAL, SEND(sprintf(_xs, "TCPIP input queue overrun\r\n");)) // Reached the end
      break;
    }
  }

  return bytes_moved;
}

/*******************************************************************************
 *
 * @function: get_string
 *
 * @brief:    Read a text string from the available ports
 *
 * @return:   TRUE if a CR or LF string terminator is entered
 *
 *******************************************************************************
 *
 * Stay in a loop waiting for characters to arrive on any of the serial input
 * streams.  Collect each character as it arrives and return when a CR or LF
 * has been received.
 *
 ******************************************************************************/
bool get_string(char destination[], int size)
{
  int ch; // Input character
  int i;  // Input index

  i              = 0;
  destination[0] = 0;
  while ( 1 )
  {
    if ( serial_available(ALL) != 0 )
    {
      ch = serial_getch(ALL);
      SEND(sprintf(_xs, "%c", ch);)

      switch ( ch )
      {
        case 8: // Backspace
          i--;
          if ( i < 0 )
          {
            i = 0;
          }
          destination[i] = 0;
          break;

        case '\r':       // Enter
        case '\n':       // newline
          return 1;

        case 'C' & 0x1F: // Control C, exit
        case 0x1B:       // Escape
          return 0;

        default:
          destination[i] = ch;
          if ( i < size )
          {
            i++;
          }
          destination[i] = 0;
          break;
      }
    }
    vTaskDelay(10);
  }
}

/*******************************************************************************
 *
 * @function: serial_port_test
 *
 * @brief:    Loopback between AUX and console
 *
 * @return:   None
 *
 *******************************************************************************
 *
 * Output to the AUX port, read back the results and send them to the console
 *
 ******************************************************************************/
void serial_port_test(void)
{
  unsigned char          test[] = "PASS - This is the loopback test";
  unsigned int           i;
  unsigned char          ch;
  volatile unsigned long test_time;

  timer_new(&test_time, ONE_SECOND * 10);

  /*
   * Abort the test if the AUX port is not available
   */
  if ( json_aux_port_enable == false )
  {
    SEND(sprintf(_xs, "\r\nAUX port not enabled.  Use {\"AUX_PORTS_ENABLE\": 1} to enable");)
    SEND(sprintf(_xs, _DONE_);)
    return;
  }

  /*
   * Send out the AUX port, back in, and then to the console
   */
  SEND(sprintf(_xs, "\r\nAUX Serial Port Loopback.  Make sure AUX port is looped back");)
  for ( i = 0; i != sizeof(test); i++ )
  {
    serial_putch(test[i], AUX); // Output to the AUX Port

    while ( serial_available(AUX) == 0 )
    {
      timer_delay(1);           // Wait for it to come back
      if ( test_time == 0 )
      {
        SEND(sprintf(_xs, "\r\nTest failed, no input from AUX\r\n");)
        return;
      }
    }

    ch = serial_getch(AUX);
    serial_putch(CONSOLE, ch);
  }

  /*
   *  The test is over
   */
  timer_delete(&test_time);
  SEND(sprintf(_xs, _DONE_);)
  return;
}