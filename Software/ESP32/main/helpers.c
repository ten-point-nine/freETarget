/*******************************************************************************
 *
 * file: helpers.c
 *
 * FreeETarget helper files
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

#include "freETarget.h"
#include "helpers.h"
#include "json.h"
#include "token.h"
#include "timer.h"
#include "gpio.h"
#include "gpio_define.h"
#include "diag_tools.h"
#include "analog_io.h"
#include "wifi.h"
#include "serial_io.h"

#define SHOT_TIME_TO_SECONDS(x) ((real_t)(x)) / 1000000.0

real_t sq(real_t x)
{
  return x * x;
}

/*-----------------------------------------------------
 *
 * @function: target_name
 *
 * @brief: Determine the target name and return
 *
 * @return: Target name returned via pointer
 *
 *-----------------------------------------------------
 *
 * Depending on the settings, determine the target name
 * and return it to the caller
 *
 * To set the target name, the following rules apply:
 *
 * {"NAME_ID":0-20} Pre defined names
 * {"NAME_ID":99, "NAME_TEXT":"myTargetName"}
 *
 *
 *-----------------------------------------------------*/
const char *names[] = {"TARGET",                                                                                         //  0
                       "1",      "2",      "3",       "4",      "5",       "6",       "7",      "8",     "9",      "10", //  1
                       "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                          // 11
                       "RUDOLF", "DONNER", "BLITZEN", "DASHER", "PRANCER", "VIXEN",   "COMET",  "CUPID", "DUNDER",       // 18
                       "ODIN",   "WODEN",  "THOR",    "BALDAR", "TEST",                                                  // 26
                       0};

void target_name(char *name_space)
{

  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF) )
  {
    switch ( json_name_id )
    {
      case JSON_NAME_TEXT:
        sprintf(name_space, "FET-%s", json_name_text);      // Name - FET-MyTargetName
        break;

      case JSON_NAME_CLIENT:
        sprintf(name_space, "%s", json_name_text);          // Name - MyTargetName
        break;

      case JSON_NAME_SN:
        sprintf(name_space, "FET-%d", json_serial_number);  // Name - serial Number
        break;

      default:
        if ( (json_name_id < 0) || (json_name_id > 26) )    // Check for limits
        {
          json_name_id = 0;
        }
        sprintf(name_space, "FET-%s", names[json_name_id]); // Name - FET-TARGET, FET-2, etc.
        break;
    }
  }
  else
  {
    sprintf(name_space, "FET-%d", my_ring);
  }

  /*
   * All done, return
   */
  return;
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
  target_name(str);
  SEND(ALL, sprintf(_xs, "{\"%s\"0, \"NAME\":\"%s\"}", _HELLO_, str);)

  set_status_LED(LED_READY);
  set_LED_PWM_now(json_LED_PWM);
  power_save = json_power_save * (time_count_t)ONE_SECOND * 60L;
  run_state &= ~IN_SLEEP; // Out of sleep and back in operation
  run_state |= IN_OPERATION;
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
 * This is called every second to send out the keep alive to the
 * TCPIP server
 *
 *--------------------------------------------------------------*/
void send_keep_alive(void)
{
  static int keep_alive_count = 0;
  static int keep_alive       = 0;
  char       str[SHORT_TEXT];

  if ( (json_keep_alive != 0) && (keep_alive <= 0) ) // Time in seconds
  {
    target_name(str);
    SEND(TCPIP, sprintf(_xs, "{\"KEEP_ALIVE\":%d, \"NAME\":\"%s\"}", keep_alive_count++, str);)
    keep_alive = (time_count_t)json_keep_alive * ONE_SECOND;
  }
  return;
}
/*----------------------------------------------------------------
 *
 * @function: bye
 *
 * @brief:    Go into power saver
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function allows the user to remotly shut down the unit
 * when not in use.
 *
 * There are two modes of operation:
 * 1. Forced Bye - When the power save timer runs out
 * 2. Normal Bye - When commanded by the user via the MFS switch
 *
 *--------------------------------------------------------------*/
static enum {
  BYE_BYE = 0,                    // Wait for the timer to run out
  BYE_HOLD,                       // Wait for the MFS to be pressed
  BYE_START                       // Go back into service
} bye_state;

void bye_tick(void)               // Call back from the power_save timer
{
  bye_state = BYE_BYE;
  if ( json_token != TOKEN_NONE ) // Skip if token ring enabled
  {
    return;
  }
  bye(0);
  return;
}

void bye(unsigned int force_bye) // Set to true to force a shutdown
{

  char str[SHORT_TEXT];

  switch ( bye_state )
  {
    case BYE_BYE:                          // Say Good Night Gracie!
      target_name(str);
      SEND(ALL, sprintf(_xs, "{\"%s\":0, \"NAME\":\"%s\"}", _BYE_, str);)
      set_LED_PWM(0);                      // Going to sleep
      set_status_LED(LED_BYE);
      serial_flush(ALL);                   // Purge the com port
      run_state &= ~IN_OPERATION;          // Take the system out of operating mode
      run_state |= IN_SLEEP;               // Put it to sleep
      bye_state = BYE_HOLD;
      break;

    case BYE_HOLD:                         // Loop waiting for something to happen
      if ( (DIP_SW_A)                      // Wait for the switch to be pressed
           || (DIP_SW_B)                   // Or the switch to be pressed
           || (serial_available(ALL) != 0) // Or a character to arrive
           || (is_running() != 0) )        // Or a shot arrives
      {
        bye_state = BYE_START;             // wait for the swich to be released
      } // turns up
      break;

    case BYE_START:
      if ( !(DIP_SW_A) // Wait here for both switches to be released
           && !(DIP_SW_B) )
      {
        hello();
        bye_state = BYE_BYE;
      }
      break;
  }

  /*
   * Loop for the next time
   */
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
 * @function: build_json_score
 *
 * @brief:    Assemble the score as a JSON string based on the format
 *
 * @return:   _xs contains the JSON string
 *
 *----------------------------------------------------------------
 *
 * Build up a JSON string for the score.
 *
 * The function is called with the shot record to be reported and
 * a pointer to a string of fields used to build up the JSON
 * output.
 *
 * IMPORTANT
 *
 * Each field is preceeded with a comma (,) except for "shot"
 * which must be the first field printed.
 *
 *--------------------------------------------------------------*/

void build_json_score(shot_record_t *shot, // Pointer to shot record
                      const char    *fields)
{
  char       str[MEDIUM_TEXT];             // String holding buffers
  static int ts, tx, ty;                   // Test Values

  _xs[0] = 0;

  /*
   *  Loop and build up the payload
   */
  while ( *fields != 0 )
  {
    switch ( *fields )
    {
      case SCORE_LEFT_BRACE:
        sprintf(str, "{"); // Start the opening bracket
        break;

      case SCORE_RIGHT_BRACE:
        if ( is_trace & DLT_SCORE )
        {
          sprintf(str, ", \"n\":%d, \"e\":%d, \"s\":%d, \"w\":%d }", shot->timer_count[N], shot->timer_count[E], shot->timer_count[S],
                  shot->timer_count[W]);                     // Hardware values
        }
        else
        {
          sprintf(str, "}");                                 // End the closing bracket
        }
        break;

      case SCORE_NEW_LINE:                                   // Add a newline
        sprintf(str, "\n");
        break;

      case SCORE_TEST:                                       // Prime for HTTP
        sprintf(str, "\"%s\":%d, \"x\":%d, \"y\":%d, \"r\":0, \"a\":0,\"target\":%d", _SHOT_, ts, tx, ty, http_target_type());
        ts++;
        tx = (tx + 5) % 103;
        ty = (ty + 7) % 103;                                 // Test values to show that the function works
        break;

      case SCORE_PRIME:                                      // Prime for HTTP
        sprintf(str, "\"%s\":0, \"x\":0, \"y\":0, \"r\":0, \"a\":0,\"target\":%d", _SHOT_, http_target_type());
        break;

      case SCORE_SHOT:                                       // Shot number
        sprintf(str, "\"%s\":%d", _SHOT_, (shot->shot) + 1); // The client wants shots to start at 1
        break;

      case SCORE_MISS:                                       // Miss
        sprintf(str, ", \"miss\":%d", shot->miss);
        break;

      case SCORE_SESSION:                                    // Session type
        sprintf(str, ", \"session_type\":%d", shot->session_type);
        break;

      case SCORE_TIME:                                       // Time
        sprintf(str, ", \"time\":%ld, \"time_to_go\":%ld", shot->shot_time, time_to_go);
        break;

      case SCORE_ELAPSED:                                    // Time since shooting began
        sprintf(str, ", \"elapsed_time\":%lds", run_time_seconds());
        break;

      case SCORE_XY:                                         // X
        sprintf(str, ", \"x\":%4.2f, \"y\":%4.2f", shot->x_mm, shot->y_mm);
        break;

      case SCORE_POLAR:                                      // Polar
        sprintf(str, ", \"r\":%6.2f, \"a\":%6.2f", shot->radius, shot->angle);
        break;

      case SCORE_HARDWARE:                                   // Hardware
        sprintf(str, ", \"n\":%d, \"e\":%d, \"s\":%d, \"w\":%d", (int)shot->timer_count[N + 0], (int)shot->timer_count[E + 0],
                (int)shot->timer_count[S + 0], (int)shot->timer_count[W + 0]);
        break;

      case SCORE_TARGET:                                     // Target type
        sprintf(str, ", \"target\":%d ", http_target_type());
        break;

      case SCORE_EVENT:                                      // Event data
        sprintf(str, ", \"athelete\":\"%s\", \"event\":\"%s\", \"target_name\":\"%s\"", json_athlete, json_event, json_target_name);
        break;

      default:
        break;
    }
    fields++;
    strcat(_xs, str);
  }

  /*
   * Put in the closing } and return
   */
  return;
}

/*
 *  Generate a known score message
 */
void test_build_json_score(void)
{
  char str[MEDIUM_TEXT];

  SEND(ALL, sprintf(_xs, "\r\ntest_build_json_score()");)

  build_json_score(&record[0], SCORE_ALL);
  strncpy(str, _xs, sizeof(str));
  SEND(ALL, sprintf(_xs, "\r\nALL:       %s", str);)

  build_json_score(&record[0], SCORE_USB);
  strncpy(str, _xs, sizeof(str));
  SEND(ALL, sprintf(_xs, "\r\nUSB:       %s", str);)

  build_json_score(&record[0], SCORE_TCPIP);
  strncpy(str, _xs, sizeof(str));
  SEND(ALL, sprintf(_xs, "\r\nTCPIP:     %s", str);)

  build_json_score(&record[0], SCORE_BLUETOOTH);
  strncpy(str, _xs, sizeof(str));
  SEND(ALL, sprintf(_xs, "\r\nBLUETOOTH: %s", str);)

  build_json_score(&record[0], SCORE_HTTP);
  strncpy(str, _xs, sizeof(str));
  SEND(ALL, sprintf(_xs, "\r\nHTTP:      %s", str);)

  SEND(ALL, sprintf(_xs, "%s", _DONE_);)

  return;
}

/*----------------------------------------------------------------
 *
 * @function: http_target_type
 *
 * @brief:    Figure out the target type for
 *
 * @return:   target type number
 *
 *----------------------------------------------------------------
 *
 * The HTTP application uses a number to identify the target.
 *
 * This function matches up the target name from the PC client to
 * the number used by HTTP.
 *
 *  110   ISSF 10 Metre Air Rifle
 *  111   ISSF 10 Metre Air Rifle Practice
 *  100   ISSF 10 Metre Air Pistol
 *  101   ISSF 10 Metre Air Pistol Practice
 *  510   ISSF 50 Metre Rifle
 *  511   ISSF 50 Metre Rifle Practice
 *  500   ISSF 50 Metre .22 Pistol
 *  501   ISSF 50 Metre .22 Pistol Practice
 *
 *--------------------------------------------------------------*/
int http_target_type(void)
{
  int target_code = 0;

  /*
   * Practice or event
   */
  if ( instr(json_event, "Practice") != -1 )
  {
    target_code += 1;
  }

  /*
   * Rifle or pistol
   */
  if ( instr(json_target_name, "Rifle") != -1 )
  {
    target_code += 10;
  }

  /*
   * 50m
   */
  if ( instr(json_target_name, "50m") != -1 )
  {
    target_code += 500;
  }
  else
  {
    target_code += 100;
  }

  return target_code;
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
        set_status_LED(LED_WIFI_FAULT);         // Empty
        WiFi_reconnect();
      }
      else
      {
        wifi_is_connected = true;               // We are connected
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