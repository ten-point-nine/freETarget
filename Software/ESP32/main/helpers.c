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

#define SHOT_TIME_TO_SECONDS(x) ((float)(x)) / 1000000.0

double sq(double x)
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

      default:
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
  int   i;
  char *start;

  start = match;                               // Save the start of the comparison string

  i = 0;
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
  /*
   * Woken up again.  Turn things back on
   */
  SEND(ALL, sprintf(_xs, "{\"Hello_World\":0}");)
  set_status_LED(LED_READY);
  set_LED_PWM_now(json_LED_PWM);
  ft_timer_new(&power_save, json_power_save * (unsigned long)ONE_SECOND * 60L);
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

  if ( (json_keep_alive != 0) && (keep_alive == 0) ) // Time in seconds
  {
    sprintf(_xs, "{\"KEEP_ALIVE\":%d}", keep_alive_count++);
    serial_to_all(_xs, TCPIP);
    ft_timer_new(&keep_alive, (unsigned long)json_keep_alive * ONE_SECOND);
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
 * This is called every second from the synchronous scheduler
 *
 *--------------------------------------------------------------*/
static enum bye_state {
  BYE_BYE = 0, // Wait for the timer to run out
  BYE_HOLD,    // Wait for the MFS to be pressed
  BYE_START    // Go back into service
};

void bye_tick(void)
{
  if ( time_to_go > 0 )
  {
    time_to_go--;                // Decriment the time left in the session
  }
  bye(0);
}
void bye(unsigned int force_bye) // Set to true to force a shutdown
{
  static int bye_state = BYE_BYE;

  /*
   * The BYE function does not work if we are a token ring.
   */
  if ( force_bye == 0 )             // Regular
  {
    if ( (json_token != TOKEN_NONE) // Skip if token ring enabled
         || (json_power_save == 0)  // Power down has not been enabled
         || (power_save != 0) )     // Power down has not run out
    {
      bye_state = BYE_BYE;
      return;
    }
  }

  switch ( bye_state )
  {
    case BYE_BYE:                          // Say Good Night Gracie!
      SEND(ALL, sprintf(_xs, "{\"GOOD_BYE\":0}");)
      json_tabata_enable = false;          // Turn off any automatic cycles
      json_rapid_enable  = false;
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
  unsigned char          ch;
  volatile unsigned long test_time;

  ft_timer_new(&test_time, (unsigned long)duration);

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
        sprintf(str, "{");                             // Start the opening bracket
        break;

      case SCORE_RIGHT_BRACE:
        sprintf(str, "}");                             // End the closing bracket
        break;

      case SCORE_NEW_LINE:                             // Add a newline
        sprintf(str, "\n");
        break;

      case SCORE_TEST:                                 // Prime for HTTP
        sprintf(str, "\"shot\":%d, \"x\":%d, \"y\":%d, \"r\":0, \"a\":0,\"target\":%d", ts, tx, ty, http_target_type());
        ts++;
        tx = (tx + 5) % 103;
        ty = (ty + 7) % 103;                           // Test values to show that the function works
        break;

      case SCORE_PRIME:                                // Prime for HTTP
        sprintf(str, "\"shot\":0, \"x\":0, \"y\":0, \"r\":0, \"a\":0,\"target\":%d", http_target_type());
        break;

      case SCORE_SHOT:                                 // Shot number
        sprintf(str, "\"shot\":%d", (shot->shot) + 1); // The client wants shots to start at 1
        break;

      case SCORE_MISS:                                 // Miss
        sprintf(str, ", \"miss\":%d", shot->miss);
        break;

      case SCORE_SESSION:                              // Session type
        sprintf(str, ", \"session_type\":%d", shot->session_type);
        break;

      case SCORE_TIME:                                 // Time
        sprintf(str, ", \"time\":%ld, \"time_to_go\":%ld", shot->shot_time, time_to_go);
        break;

      case SCORE_ELAPSED:                              // Time since shooting began
        sprintf(str, ", \"elapsed_time\":%lds", run_time_seconds());
        break;

      case SCORE_XY:                                   // X
        sprintf(str, ", \"x\":%4.2f, \"y\":%4.2f", shot->x_mm, shot->y_mm);
        break;

      case SCORE_POLAR:                                // Polar
        sprintf(str, ", \"r\":%6.2f, \"a\":%6.2f", shot->radius, shot->angle);
        break;

      case SCORE_HARDWARE:                             // Hardware
        sprintf(str, ", \"n\":%d, \"e\":%d, \"s\":%d, \"w\":%d", (int)shot->timer_count[N + 0], (int)shot->timer_count[E + 0],
                (int)shot->timer_count[S + 0], (int)shot->timer_count[W + 0]);
        break;

      case SCORE_TARGET:                               // Target type
        sprintf(str, ", \"target\":%d ", http_target_type());
        break;

      case SCORE_EVENT:                                // Event data
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
