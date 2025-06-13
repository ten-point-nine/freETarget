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
#include "string.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include "freETarget.h"
#include "helpers.h"
#include "diag_tools.h"
#include "serial_io.h"
#include "timer.h"
#include "json.h"
#include "nonvol.h"
#include "http_server.h"

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

const uart_config_t uart_BT_config = {.baud_rate           = 115200,
                                      .data_bits           = UART_DATA_8_BITS,
                                      .parity              = UART_PARITY_DISABLE,
                                      .stop_bits           = UART_STOP_BITS_1,
                                      .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
                                      .rx_flow_ctrl_thresh = 122,
                                      .source_clk          = UART_SCLK_DEFAULT};

const uart_config_t uart_BT_INIT_38400_config = {.baud_rate           = 38400,
                                                 .data_bits           = UART_DATA_8_BITS,
                                                 .parity              = UART_PARITY_DISABLE,
                                                 .stop_bits           = UART_STOP_BITS_1,
                                                 .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
                                                 .rx_flow_ctrl_thresh = 122,
                                                 .source_clk          = UART_SCLK_DEFAULT};

const uart_config_t uart_BT_INIT_9600_config = {.baud_rate           = 9600,
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
  char queue[1024];                    // Holding queue
  int  in;                             // Index of input characters
  int  out;                            // Index of output characters
} queue_struct_t;

static queue_struct_t in_buffer;       // TCPIP input buffer
static queue_struct_t out_buffer;      // TCPIP input buffer
unsigned int          connection_list; // Bitmask of existing connections

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
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Console port initialized");))
  return;
}

void serial_aux_init(void)
{
  if ( (json_aux_mode != AUX) && (json_aux_mode != BLUETOOTH) )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "AUX Port not enabled");))
    return;
  }

  /*
   *  Load the driver
   */
  uart_driver_install(UART_NUM_1, uart_aux_size, uart_aux_size, 10, &uart_aux_queue, 0);

  /*
   *  Setup the communications parameters
   */
  if ( json_aux_mode == AUX )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "AUX port enabled\r\n");))
    uart_param_config(uart_aux, &uart_aux_config); // 115200 baud rate
  }
  if ( json_aux_mode == BLUETOOTH )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "BLUETOOTH port enabled\r\n");))
    uart_param_config(uart_aux, &uart_BT_config);  // 115200 baud rate
  }

  /*
   *  Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
   */
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  /*
   * All done, return
   */
  return;
}

void serial_bt_config(unsigned int baud_rate) // Program port for Bluetooth initialization
{
  if ( json_aux_mode == 0 )
  {
    return;
  }

  /*
   *  Setup the communications parameters
   */
  switch ( baud_rate )
  {
    case 9600:
      uart_param_config(uart_aux, &uart_BT_INIT_9600_config);
      break;
    case 38400:
      uart_param_config(uart_aux, &uart_BT_INIT_38400_config);
      break;
    default:
    case 115200:
      uart_param_config(uart_aux, &uart_BT_config);
      break;
  }

  /*
   * All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "serial_bt_config(%d)\r\n", baud_rate);))
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

  if ( ((ports & json_aux_mode) & (AUX | BLUETOOTH)) != 0 ) // Is there hardware on the Aux port?
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

  if ( ((ports & json_aux_mode) & (AUX | BLUETOOTH)) != 0 ) // Is there hardware on the Aux port?
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
 * Poll the incoming queues and return the next character.
 *
 * If there is a character present, update the connection mask
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
      connection_list |= CONSOLE;
      return ch;
    }
  }

  /*
   *  Bring in the AUX bytes
   */
  if ( ((ports & json_aux_mode) & (AUX | BLUETOOTH)) != 0 ) // Is there hardware on the Aux port?
  {
    if ( uart_read_bytes(uart_aux, &ch, 1, 0) > 0 )
    {
      connection_list |= AUX;
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
      connection_list |= TCPIP; // Set the connection list
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
  if ( ports & (EVEN_ODD_BEGIN | EVEN_ODD_END) )
  {
    return;                  // Return if it's a control message
  }

  /*
   * Output to the devices
   */
  if ( ports & CONSOLE )
  {
    printf("%c", ch);                                       // Must be printf
  }

  if ( ((ports & json_aux_mode) & (AUX | BLUETOOTH)) != 0 ) // Is there hardware on the Aux port?
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

void serial_to_all(char *str,        // String to output
                   int   ports       // List of ports to output to
)
{
  static bool even_odd_mode = 0;     // Are we concatinating output?
  static bool even_odd      = false; // Set the even odd flag to odd (fiest half)
  static int  line_count    = 0;     // How many lines have we output
  static char e_o_line[SHORT_TEXT];  // Place to store even odd srring

  /*
   * Check to see if we have a control message coming
   */
  if ( (ports & EVEN_ODD_BEGIN) != 0 ) // Begin concatination
  {
    even_odd_mode = true;
    even_odd      = false;
    e_o_line[0]   = 0;
    line_count    = 0;
    return;
  }

  if ( (ports & EVEN_ODD_END) != 0 ) // End concatination
  {
    even_odd_mode = false;
    return;
  }

  /*
   *See if we need to concatinate alternate calls
   */
  if ( even_odd_mode == true ) // Concatination mode
  {
    if ( even_odd == false )
    {
      strcpy(e_o_line, str);
      strncat(e_o_line, "                                                                            ", SHORT_TEXT - strlen(e_o_line));
      e_o_line[50] = 0;
      even_odd     = true;
      return; // Remember the first half of the line and return
    }

    strcat(e_o_line, str);
    strcat(e_o_line, "\r\n");
    strcpy(str, e_o_line);
    line_count++;
    if ( (line_count % 4) == 0 ) // Split up every 4 lines of output
    {
      strcat(str, "\r\n");
    }
    even_odd = false;
  }
  else                           // Normal mode
  {
    if ( even_odd == true )      // Is half a message waiting to come from last time?
    {
      strcat(e_o_line, "\r\n");  // Yes, put in a new line
      strcat(e_o_line, str);     // Add the new to the old
      strcpy(str, e_o_line);     // Put it back into the calling line
      even_odd = false;
    }
  }

  /*
   * Output to the devices
   */
  if ( ports & CONSOLE )
  {
    printf("%s", str);                                      // Must be printf
  }

  if ( ((ports & json_aux_mode) & (AUX | BLUETOOTH)) != 0 ) // Is there hardware on the Aux port?
  {
    uart_write_bytes(uart_aux, (const char *)str, strlen(str));
  }

  if ( ports & TCPIP )
  {
    tcpip_app_2_queue(str, strlen(str));
  }

  if ( ports & HTTP_CONNECTED ) // Is there a web server connected?
  {
    http_send_string(str);      // Yes, send it to the web server
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
 * @return:   Input queue updated
 *
 *******************************************************************************
 *
 * Fresh characters from the TCPIP socket are placed into the input queue
 *
 * Used also by HTTP to put client data into the queue
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
      DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "TCPIP input queue overrun\r\n");)) // Reached the end
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
      SEND(ALL, sprintf(_xs, "%c", ch);)

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
  unsigned char test[] = "PASS - This is the loopback test";
  unsigned int  i;
  unsigned char ch;
  time_count_t  test_time;

  /*
   * Abort the test if the AUX port is not available
   */
  if ( (json_aux_mode & (AUX | BLUETOOTH)) == 0 )
  {
    SEND(ALL, sprintf(_xs, "\r\nAUX port not enabled.  Use {\"AUX_MODE\": 2} to enable");)
    SEND(ALL, sprintf(_xs, _DONE_);)
    return;
  }

  SEND(ALL, sprintf(_xs, "\r\nAUX Serial Port Loopback.  Make sure AUX port is looped back");)
  if ( prompt_for_confirm() == false )
  {
    SEND(ALL, sprintf(_xs, "\r\nAborting configuration");)
    return;
  }

  /*
   * Send out the AUX port, back in, and then to the console
   */
  ft_timer_new(&test_time, ONE_SECOND * 10);
  for ( i = 0; i != sizeof(test); i++ )
  {
    serial_putch(test[i], AUX); // Output to the AUX Port

    while ( serial_available(AUX) == 0 )
    {
      timer_delay(1);           // Wait for it to come back
      if ( test_time == 0 )
      {
        SEND(ALL, sprintf(_xs, "\r\nTest failed, no input from AUX\r\n");)
        return;
      }
    }

    ch = serial_getch(AUX);
    serial_putch(CONSOLE, ch);
  }

  /*
   *  The test is over
   */
  ft_timer_delete(&test_time);
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*******************************************************************************
 *
 * @function: check_new_connection
 *
 * @brief:    Look to see if anybody new has connected
 *
 * @return:   None
 *
 *******************************************************************************
 *
 * Check to see if there is a new connection, and if so, check to see if the
 * variables need to be cleared
 *
 ******************************************************************************/
static unsigned int old_connection_list = 0;    // Previous connection mask

void check_new_connection(void)
{

  if ( old_connection_list == connection_list ) // Has anything changed?
  {
    return;                                     // No, do nothing
  }
  old_connection_list = connection_list;

  /*
   *  Count up the number of connections
   */
  if ( hamming_weight(connection_list) > 1 ) // Do we have more than one connection?
  {
    return;                                  // Yes, then return
  }

  start_new_session(json_session_type);

  /*
   *  All done, return
   */
  return;
}