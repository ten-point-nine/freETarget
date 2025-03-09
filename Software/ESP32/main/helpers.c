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

#include "freETarget.h"
#include "json.h"
#include "token.h"
#include "timer.h"
#include "gpio.h"
#include "gpio_define.h"
#include "diag_tools.h"
#include "analog_io.h"

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
                       "ODIN",   "WODEN",  "THOR",    "BALDAR",                                                          // 26
                       0};

void target_name(char *name_space)
{

  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF) )
  {
    if ( json_name_id != JSON_NAME_TEXT )
    {
      sprintf(name_space, "FET-%s", names[json_name_id]);
    }
    else
    {
      if ( json_name_text[0] != 0 )
      {
        sprintf(name_space, "FET-%s", json_name_text);
      }
      else
      {
        sprintf(name_space, "Undefined name");
      }
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
  timer_new(&power_save, json_power_save * (unsigned long)ONE_SECOND * 60L);
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
    timer_new(&keep_alive, (unsigned long)json_keep_alive * ONE_SECOND);
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

  timer_new(&test_time, (unsigned long)duration);

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
  timer_delete(&test_time);
  return;
}