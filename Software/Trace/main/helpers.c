/*******************************************************************************
 *
 * file: helpers.c
 *
 * trace helper files
 *
 *******************************************************************************
 *
 * Helper files that have no pre-defined home
 *
 ******************************************************************************/
#include <stdio.h>
#include "string.h"
#include "ctype.h"
#include "driver\gpio.h"
#include "math.h"
#include "esp_timer.h"
#include "esp_random.h"

#include "trace.h"
#include "helpers.h"
#include "json.h"
#include "timer.h"
#include "gpio.h"
#include "gpio_define.h"
#include "diag_tools.h"
#include "wifi.h"
#include "serial_io.h"

#define SHOT_TIME_TO_SECONDS(x) ((real_t)(x)) / 1000000.0
real_t SQ(real_t a)
{
  return a * a;
}

/*-----------------------------------------------------
 *
 * @function: to_int
 *
 * @brief: Convert an ASCII hex number to an integer
 *
 * @return: Integer value
 *
 *-----------------------------------------------------
 *
 * Depending on the settings, determine the target name
 * and return it to the caller
 *
 *-----------------------------------------------------*/
int to_int(char h)
{
  h = toupper(h);

  if ( h > '9' )
  {
    return 10 + (h - 'A');
  }
  else
  {
    return h - '0';
  }
}

/*-----------------------------------------------------
 *
 * @function: instr
 *
 * @brief: Compare two strings
 *
 * @return: Number of matching characters
 *
 *-----------------------------------------------------
 *
 * Compare two strings.
 * Return -1 if not equal,
 * length of string if equal
 * S1 Long String, S2 Short String .
 * instr("CAT Sam", "CAT") = 3
 * instr("CAT Sam", "CUT") == -1)
 *-----------------------------------------------------*/

int instr(char *s1, // Source string
          char *s2  // Comparison string
)
{
  int i;

  i = 0;
  while ( (*s1 != 0) && (*s2 != 0) )
  {
    if ( *s1 != *s2 )
    {
      return -1;
    }
    s1++;
    s2++;
    i++;
  }

  /*
   * Reached the end of the comparison string. Check that we arrived at a NULL
   */
  if ( *s2 == 0 ) // Both strings are the same
  {
    return i;
  }

  return -1;      // The strings are different
}

/*-----------------------------------------------------
 *
 * @function: contains
 *
 * @brief: Determine if a string contains another string
 *
 * @return: TRUE if string 2 is inside of string 1`
 *
 *-----------------------------------------------------
 *
 *-----------------------------------------------------*/
bool contains(char *source,                    // Source string
              char *match                      // Comparison string
)
{
  char *start;

  start = match;                               // Save the start of the comparison string

  while ( (*source != 0) && (*match != 0) )
  {
    if ( toupper(*match) == toupper(*source) ) // Found a match
    {
      match++;                                 // Move to the next character in the comparison string
    }
    else
    {
      match = start;                           // Reset the comparison string to the start
    }
    source++;                                  // Move to the next character in the source string
  }

  /*
   * Reached the end of the comparison string. Check that we arrived at a NULL
   */
  if ( *match == 0 ) // Reached the end of the comparison string
  {
    return true;
  }

  return false;      // The strings are different
}

/*----------------------------------------------------------------
 *
 * @function: prompt_for_confirm
 *
 * @brief:    Display a propt and wait for a return
 *
 * @return:   TRUE if the confirmation is Yes
 *
 *--------------------------------------------------------------*/
bool prompt_for_confirm(void)
{
  unsigned char ch;

  SEND(ALL, sprintf(_xs, "\r\nConfirm Y/N?");)

  /*
   * Loop and wait for a confirmation
   */
  while ( 1 )
  {
    if ( serial_available(ALL) != 0 )
    {
      ch = serial_getch(ALL);
      switch ( ch )
      {
        case 'y':
        case 'Y':
          return true;

        case 'n':
        case 'N':
          return false;

        default:
          break;
      }
      vTaskDelay(ONE_SECOND / 10);
    }
  }
}

/*----------------------------------------------------------------
 *
 * @function: hello
 *
 * @brief:    Come out of power saver
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function Puts the system back in service
 *
 *--------------------------------------------------------------*/
void hello(void)
{
  char str[SHORT_TEXT];

  /*
   * Woken up again.  Turn things back on
   */
  SEND(ALL, sprintf(_xs, "{\"%s\"0, \"NAME\":\"%s\"}", _HELLO_, str);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: send_keep_alive
 *
 * @brief:    Send a keep alive over the TCPIP
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This is a callback when the keep alive timer expires.
 *
 * Send out the keep alive message and reset the timer
 *
 *--------------------------------------------------------------*/
void send_keep_alive(void)
{
  static int keep_alive_count = 0;
  char       str[SHORT_TEXT];

  SEND(TCPIP, sprintf(_xs, "{\"KEEP_ALIVE\":%d, \"NAME\":\"%s\"}", keep_alive_count++, str);)
  keep_alive = (time_count_t)json_keep_alive * ONE_SECOND;

  return;
}

/*----------------------------------------------------------------
 *
 * @function: echo_serial
 *
 * @brief:    Echo what comes back from the serial port for TBD duration
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function allows the user to remotly shut down the unit
 * when not in use.
 *
 * This is called every second from the synchronous scheduler
 *
 *--------------------------------------------------------------*/
void echo_serial(int duration, // Duration in clock ticks
                 int in_ports, // Where to read from
                 int out_ports // Where to ouput to
)
{
  unsigned char ch;
  time_count_t  test_time;

  ft_timer_new(&test_time, (time_count_t)duration, NULL, "echo serial");

  /*
   * Loop and echo the characters
   */
  while ( test_time != 0 )
  {
    if ( serial_available(in_ports) != 0 )
    {
      ch = serial_getch(in_ports);
      serial_putch(ch, out_ports);
    }
  }

  /*
   * Finished, clean up
   */
  ft_timer_delete(&test_time);
  return;
}

/*----------------------------------------------------------------
 *
 * @function: squish
 *
 * @brief:    Reduce the uri to an argument
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * A uri with arguments looks like
 *
 * /json?{%22ECHO%22:0}
 *
 * The function parses along until it finds the ? and then
 * reduces the rest of the sthring to an argument
 *
 *--------------------------------------------------------------*/
void squish(char *uri,      // URI to squish
            char *argument) // Argument to return
{
  unsigned char ch;

                            /*
                             * Find the ?
                             */
  while ( *uri != 0 )
  {
    if ( *uri == '?' )
    {
      uri++;
      break;
    }
    uri++;
  }

  /*
   * Copy the rest of the string and remove the %s
   */
  while ( *uri != 0 )
  {
    if ( *uri == '%' )
    {
      uri++;
      ch = to_int(*uri) * 16;
      uri++;
      ch          = ch + to_int(*uri);
      *argument++ = ch;
      uri++;
    }
    else
    {
      *argument++ = *uri++;
    }
  }

  /*
   * Terminate the string
   */
  *argument = 0;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: hamming_weight
 *
 * @brief:    Count the number of bits in a word
 *
 * @return:   Number of bits in the word
 *
 *----------------------------------------------------------------
 *
 * Count up the bits in the word
 *
 *--------------------------------------------------------------*/
unsigned int hamming_weight(unsigned int word)
{
  unsigned int weight;

  weight = 0;
  while ( word != 0 )
  {
    weight += word & 1;
    word >>= 1;
  }

  return weight;
}

/*----------------------------------------------------------------
 *
 * @function: to_binary
 *
 * @brief:    Convert a number to a binary string
 *
 * @return:   string of the number in binary
 *
 *----------------------------------------------------------------
 *

 *
 *--------------------------------------------------------------*/
void to_binary(unsigned int x, // Number to convert
               unsigned int bits,
               char        *s  // String to return the binary string
)
{
  int i, j;

  j = 0;
  for ( i = 0; i != bits; i++ )
  {
    s[j++] = '0' + ((x & (1 << ((bits - i) - 1))) != 0);
    x <<= 1; // Shift the number to the left
  }
  s[j] = 0;  // Terminate the string

  return;
}

/*----------------------------------------------------------------
 *
 * @function: watchdog
 *
 * @brief:    Monitor the target health
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * Monitor the health of the target and take action if something
 * is wrong
 *
 * WiFI connection not made.
 *   The board IP address is empty
 *   Try to reconnect
 *
 *--------------------------------------------------------------*/
void watchdog(void)
{
  char        str_c[SHORT_TEXT];
  static bool wifi_is_connected = false;

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "watchdog()");))

  /*
   *  Check to see if we have a connection to the WiFi
   */
  if ( json_wifi_ssid[0] != 0 )                 // We are a station?
  {
    if ( wifi_is_connected == false )           // Was not connected
    {
      if ( WiFi_my_IP_address(str_c) == false ) // Find our IP address
      {
        DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "Trying to connect to access point");))
        WiFi_reconnect();
      }
      else
      {
        wifi_is_connected = true; // We are connected
      }
    }
  }

  /*
   *  All done
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: get_OTA_serial
 *
 * @brief:    Read a bloack of raw data
 *
 * @return:   Number of bytes read in
 *            0 - End of record
 *           -1 - Error
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/
#define OTA_SERIAL_TIMEOUT (time_count_t)10 * ONE_SECOND                   // 10 second timeout
static time_count_t time_out;                                              // Time out timer

int get_OTA_serial(int   length,                                           // Maximum number of bytes to read
                   char *s)                                                // Place to save the input data
{
  unsigned char ch;                                                        // Inputing character
  unsigned int  byte_count;                                                // Number of bytes read in
  int           bytes_available;                                           // Number of bytes available from PC Client

  byte_count = 0;                                                          // Nothing has arrived yet
  ft_timer_new(&time_out, OTA_SERIAL_TIMEOUT, NULL, "OTA serial timeout"); // Time out timer

  /*
   * Loop and read the data
   */
  while ( 1 )
  {
    bytes_available = serial_available(ALL);
    if ( bytes_available > 0 )        // Got something?
    {
      time_out = OTA_SERIAL_TIMEOUT;  // Reset the timout
      while ( bytes_available-- > 0 ) // Read all available characters
      {
        ch = serial_getch(ALL);       // Read the character
        *s = ch;                      // and save it away
        s++;                          // Move to the next character
        length--;                     // One less to read
        byte_count++;                 // Count the bytes read in

        if ( length == 0 )            // Got everything for now
        {
          return byte_count;
        }
      }
    }

    if ( time_out <= 0 ) // Timeout
    {
      return -1;         // Report an error
    }
  }

  return byte_count;
}

/*----------------------------------------------------------------
 *
 * @function: get_number
 *
 * @brief:    Get a number from the command line
 *
 * @return:   Number entered
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/
void get_number(char *prompt, real_t *value)
{
  char   str[SHORT_TEXT];
  char  *end_ptr;
  real_t val;

  SEND(ALL, sprintf(_xs, "%s", prompt);)

  /*
   * Loop and get a number
   */
  while ( 1 )
  {
    /*
     * Get the input string
     */
    get_string(str, sizeof(str));

    /*
     * Convert to a number
     */
    val = strtod(str, &end_ptr);
    if ( end_ptr != str ) // Got something
    {
      *value = val;
      return;
    }

    /*
     * Error, try again
     */
    SEND(ALL, sprintf(_xs, "Invalid number, try again:");)
  }
}

/*----------------------------------------------------------------
 *
 * @function: atan2_2PI
 *
 * @brief:    atan2 function that returns 0 to 2PI
 *
 * @return:   angle in radians
 *
 *----------------------------------------------------------------
 *
 * The natice atan2 function returns -PI to +PI.
 * This function returns 0 to 2PI
 *
 *--------------------------------------------------------------*/
real_t atan2_2PI(real_t y, real_t x)
{
  real_t angle = atan2f(y, x);
  if ( angle < 0 )
  {
    angle += TWO_PI; // Keep in range 0 to 2PI
  }

  return angle;
}

real_t atan2_degrees(real_t y, real_t x)
{
  return atan2_2PI(x, y) / PI * 180.0f;
}

/*----------------------------------------------------------------
 *
 * @function: no_singularity
 *
 * @brief:    Ensure no sigularities between sets of numbers
 *
 * @return:   Numbers adjusted to prevent singularities
 *
 *----------------------------------------------------------------
 *
 * If any of the three arguements are equal, there is a risk of
 * a singularity in the calculations.
 *
 * To avoid this problem, the numbers are tweaked a bit so that
 * they are not the same.
 *
 * To do this, a mask of what equals what is generated.  If none
 * are equal, then the function returns.
 *
 * If any or all are equal, the values are tweaked so they are
 * different and the caomparison takes place again util they
 * are all different.
 *
 * A is adjusted downwards
 * B is always left alone
 * C is adjusted upwards
 *--------------------------------------------------------------*/
#define AB  0b011 // A == B
#define BC  0b110 // B == C
#define AC  0b101 // A == C
#define ABC 0b111 // A == B== C

void no_singularity(real_t *a, real_t *b, real_t *c)
{
  int equal;      // Mask of equalities

  while ( 1 )
  {
    equal = 0;
    if ( *a == *b )
    {
      equal |= AB;
    }
    if ( *b == *c )
    {
      equal |= BC;
    }
    if ( *a == *c )
    {
      equal |= AC;
    }

    if ( equal == 0 ) // None are equal
    {
      return;         // Nothing more to do
    }

    switch ( equal )  // Jump to the ones that are equal
    {
      case ABC:
        *a -= 1E-6;   // and adjust A smaller
        *c += 1E-6;   // C bigger
        break;

      case AB:        // Change A
        *a -= 1E-6;
        break;

      case BC:        // Change C
      case AC:
        *c += 1E-6;
        break;
    }
  }

  return;
}

/*----------------------------------------------------------------
 *
 * @function: radians_to_degrees
 *            degrees_to_radians
 *
 * @brief:    Angular conversions
 *
 * @return:   Converted values
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
real_t radians_to_degrees(real_t radians)
{
  return (radians / PI * 180.0);
}

real_t degrees_to_radians(real_t degrees)
{
  return (degrees / 180.0 * PI);
}