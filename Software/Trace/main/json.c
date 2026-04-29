/*-------------------------------------------------------
 *
 * JSON.c
 *
 * JSON driver
 *
 * ----------------------------------------------------*/
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"

#define JSON_C // This is the JSON file
#include "trace.h"
#include "board_assembly.h"
#include "helpers.h"
#include "ctype.h"
#include "diag_tools.h"
#include "json.h"
#include "mechanical.h"
#include "nonvol.h"
#include "serial_io.h"
#include "stdio.h"
#include "serial_io.h"
#include "wifi.h"
#include "timer.h"

/*
 *  Function Prototypes
 */
static void handle_json(void);                                                 // Breakdown the JSON and execute it
void        show_echo(void);                                                   // Display the current settings
static void show_names(int v);
static void set_trace(int v);                                                  // Set the trace on and off
static bool good_input(unsigned int conversion, char next, unsigned int show); // Determine if the input is valid

/*
 *  Variables
 */
char input_JSON[EXTRA_LARGE_STRING]; // JSON input buffer

void        show_echo(void);         // Display the current settings
static void show_names(int v);
static void set_trace(int v);        // Set the trace on and off

const json_message_t JSON[] = {
    //  show     token        value stored in RAM             convert                 service fcn()     NONVOL location      Initial Value
    //  PS Value
    {HIDE, "\"ECHO\"",     0,                   IS_VOID,  &show_echo,    0,                0,      0},
    {HIDE, "\"INIT\"",     0,                   IS_VOID,  &init_nonvol,  0,                0,      0},
    {SHOW, "\"NAME_ID\":", &json_name_id,       IS_INT32, &show_names,   NONVOL_NAME_ID,   0,      0},
    {HIDE, "\"RESET\"",    0,                   IS_VOID,  &esp_restart,  0,                0,      0},
    {SHOW, "\"SN\":",      &json_serial_number, IS_FIXED, 0,             NONVOL_SERIAL_NO, 0xffff, 0},
    {HIDE, "\"TEST\":",    0,                   IS_INT32, &self_test,    0,                0,      0},
    {SHOW, "\"TRACE\":",   0,                   IS_INT32, &set_trace,    0,                0,      0},
    {SHOW, "\"VERSION\"",  0,                   IS_INT32, &POST_version, 0,                0,      0},
    {0,    0,              0,                   0,        0,             0,                0,      0}
};

/*-----------------------------------------------------
 *
 * @function: trace_json
 *
 * @brief: Accumulate input from the serial port
 *
 * @return: None
 *
 *-----------------------------------------------------
 *
 * The format of the JSON stings used here is
 *
 * {"LABLE":value }
 *
 * {"ECHO":0"}
 * {"ECHO":0, "DIP":8}
 * {"DIP":9, "SENSOR":230.0, "ECHO":0}
 * {"TEST":7, "ECHO":0}
 * {"PAPER":1, "DELAY":5, "PAPER":0, "TEST":16}
 *
 * Find the lable, ex "DIP": and save in the
 * corresponding memory location
 *
 *-----------------------------------------------------*/
static unsigned int in_JSON           = 0;
static unsigned int got_right_bracket = 0;
static bool         not_found;
static bool         keep_space;         // Set to 1 if keeping spaces
static bool         got_left_bracket;   // Set to 1 if we have a bracket
unsigned int        from_BlueTooth = 0; // Count of characters from the BlueTooth port

void trace_json(void *pvParameters)
{
  char ch;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "trace_json()");))

  while ( 1 )
  {
#if ( 0 )
    IF_NOT(IN_OPERATION)
    {
      vTaskDelay(ONE_SECOND);
      continue;
    }
#endif

    /*
     * See if anything is waiting and if so, add it in
     */
    while ( (serial_available(ALL) != 0) ) // Something waiting for us?
    {
      ch = serial_getch(ALL);
      serial_putch(ALL, ch);               // Echo the character back

                                           /*
                                            * Parse the stream
                                            */

      if ( ch == '\n' ) // New Line
      {
        ch = ',';       // Convert to a comma
      }

      if ( ch == '\r' ) // Carriage Return, ignore it
      {
        continue;
      }

      switch ( ch )
      {
        case '}':
          if ( in_JSON != 0 )
          {
            got_left_bracket  = false;
            got_right_bracket = in_JSON;
            handle_json(); // Fall through to manage the JSON message
            vTaskDelay(TICK_10ms);
            serial_flush(ALL);
          }

        case '{':
          in_JSON           = 0;
          input_JSON[0]     = 0;
          got_right_bracket = 0;
          got_left_bracket  = true;
          keep_space        = 0;
          break;

        case 0x08:                    // Backspace
          if ( in_JSON != 0 )
          {
            in_JSON--;
          }
          input_JSON[in_JSON] = 0;    // Null terminate
          break;

        case '*':                     // Connected to PC over BT or Wifi
          POST_version();

        case '^':                     // Special case for European keyboards which have a different "*" key
          ch = '"';                   // Convert and fall through

        case '"':                     // Start or end of text
          keep_space = (keep_space ^ 1) & 1;

        default:
          if ( (ch != ' ') || keep_space )
          {
            input_JSON[in_JSON] = ch; // Add in the latest
            if ( in_JSON < (sizeof(input_JSON) - 1) )
            {
              in_JSON++;
            }
            input_JSON[in_JSON] = 0;  // Null terminate
          }
          break;
      } // End switch

    } // End if char available
    vTaskDelay(TICK_10ms);
  }

  /*
   *  Never get here
   */
}

/*-----------------------------------------------------
 *
 * @function: handle_json
 *
 * @brief:  Breakdown the JSON and handle it
 *
 * @return: None
 *
 *-----------------------------------------------------
 *
 * The input stream is parsed one JSON token at a time
 * and the JSON[] is used to determine the action
 *
 *-----------------------------------------------------*/
static void handle_json(void)
{
  int   x;
  float f;
  int   i, j, k;
  char  s[64];          // Place to store a string
  int   m;

  run_state |= IN_HTTP; // Parsing a JON string

  /*
   * Found out where the braces are, extract the contents.
   */
  not_found = true;
  k         = 0;

  for ( i = 0; i != got_right_bracket; i++ )      // Go across the JSON input
  {
    j = 0;                                        // Index across the JSON token table

    while ( (JSON[j].token != 0) )                // Cycle through the tokens
    {
      x = 0;
      if ( JSON[j].token != 0 )
      {
        k = instr(&input_JSON[i], JSON[j].token); // Compare the input against the list of JSON tags
        if ( k > 0 )                              // Non zero, found something
        {
          not_found = false;                      // Read and convert the JSON value
          if ( good_input(JSON[j].convert, input_JSON[i + k], JSON[j].show) == false )
          {
            SEND(ALL, sprintf(_xs, "\r\nInvalid input or locked: {%s}\r\n", input_JSON);)
            break;                                // Invalid input
          }

          switch ( JSON[j].convert & IS_MASK )
          {
            default:
            case IS_VOID:                         // Void, default to zero
            case IS_FIXED:                        // Fixed cannot be changed
              x = 0;
              break;

            case IS_TEXT_1:
              if ( hamming_weight(connection_list) > 1 )
              {
                break;
              }
            case IS_TEXT:                                   // Convert to text
            case IS_SECRET:

              while ( input_JSON[i + k] != '"' )            // Skip to the opening quote
              {
                k++;
              }
              k++;                                          // Advance to the text

              m    = 0;
              s[0] = 0;                                     // Put in a null
              while ( input_JSON[i + k] != '"' )            // Skip to the opening quote
              {
                s[m] = input_JSON[i + k];                   // Save the value
                m++;
                s[m] = 0;                                   // Null terminate
                k++;
              }
              if ( JSON[j].non_vol != 0 )                   // Save to persistent storage if present
              {
                nvs_set_str(my_handle, JSON[j].non_vol, s); // Store into NON-VOL
              }

              break;

            case IS_MFS:
            case IS_INT32:                                  // Convert an integer

              if ( (input_JSON[i + k] == '0') && ((input_JSON[i + k + 1] == 'X') || (input_JSON[i + k + 1] == 'x')) ) // Is it Hex?
              {
                x = (to_int(input_JSON[i + k + 2]) << 4) + to_int(input_JSON[i + k + 3]);
              }
              else
              {
                x = atoi(&input_JSON[i + k]);
              }
              if ( JSON[j].value != 0 )
              {
                *JSON[j].value = x;                         // Save the value
              }
              if ( JSON[j].non_vol != 0 )
              {
                nvs_set_i32(my_handle, JSON[j].non_vol, x); // Store into NON-VOL
              }

              break;

            case IS_FLOAT:                                  // Convert a floating point number

              f = atof(&input_JSON[i + k]);                 // Float
              x = f * FLOAT_SCALE;                          // Integer
              if ( JSON[j].value != 0 )
              {
                *(double *)JSON[j].value = f;               // Working Value
              }
              if ( JSON[j].non_vol != 0 )
              {
                nvs_set_i32(my_handle, JSON[j].non_vol,
                            x);                             // Store into NON-VOL as an integer * 1000
              }

              break;
          }

          {
            if ( JSON[j].f != 0 ) // Call the handler if it is available
            {
              JSON[j].f(x);
            }
          }
        }

        j++;
      }
      nvs_commit(my_handle); // Save to memory
    }
  }
  /*
   * Report an error if input not found
   */
  if ( (not_found == true) )
  {
    SEND(ALL, sprintf(_xs, "\r\n\r\nCannot decode: {%s}\r\n", input_JSON);)
  }

  /*
   * All done
   */
  run_state &= ~IN_HTTP; // FInished parsing the JSON input
  in_JSON           = 0; // Start Over
  got_right_bracket = 0; // Need to wait for a new Right Bracket
  got_left_bracket  = false;
  input_JSON[0]     = 0; // Clear the input
  return;
}

/*-----------------------------------------------------
 *
 * @function: show_echo
 *
 * @brief: Display the current settings
 *
 * @return: None
 *
 *-----------------------------------------------------
 *
 * Loop and display the settings
 *
 *-----------------------------------------------------*/

void show_echo(void)
{
  int  i, j;
  char str_c[32]; // String holding buffers

  SEND(ALL, sprintf(_xs, "\r\n{\r\n");)
  SEND(ALL, sprintf(_xs, "\"NAME\":              \"%s\",\r\n", str_c);)

  /*
   * Loop through all of the JSON tokens
   */

  serial_to_all(NULL, EVEN_ODD_BEGIN);
  i = 0;
  while ( JSON[i].token != 0 )             // Still more to go?
  {
    if ( ((JSON[i].show & SHOW) != 0)      // The entry is visible
         && (JSON[i].value != NULL) )      // and it has a value ?
    {
      switch ( JSON[i].convert & IS_MASK ) // Display based on it's type
      {
        default:
        case IS_VOID:
          break;

        case IS_TEXT_1:
        case IS_TEXT:
        case IS_SECRET:
          strcpy(str_c, (char *)(JSON[i].value));
          if ( (JSON[i].convert & IS_MASK) == IS_SECRET )
          {
            strncpy(str_c, "*************************************************", strlen(str_c));
          }

          SEND(ALL, sprintf(_xs, "%-18s \"%s\", ", JSON[i].token, str_c);)
          break;

        case IS_INT32:
        case IS_FIXED:
          SEND(ALL, sprintf(_xs, "%-18s %d, ", JSON[i].token, *JSON[i].value);)
          break;

        case IS_FLOAT:
          SEND(ALL, sprintf(_xs, "%-18s %6.2f, ", JSON[i].token, *(double *)JSON[i].value);)
          break;
      }
      vTaskDelay(10);
    }
    i++;
  }

  /*
   * Finish up with the special cases
   */
  serial_to_all(_xs, EVEN_ODD_END);                                                    // End the even odd line
  SEND(ALL, sprintf(_xs, "\r\n*** STATUS ***\r\n");)
  serial_to_all(NULL, EVEN_ODD_BEGIN);                                                 // Start over again
  SEND(ALL, sprintf(_xs, "\"SN\":                %d", json_serial_number);)
  SEND(ALL, sprintf(_xs, "\"TRACE\":             %d,", is_trace);)                     //
  SEND(ALL, sprintf(_xs, "\"RUN_STATE\":         %d,", run_state);)                    // Internal running state is enabled
  SEND(ALL, sprintf(_xs, "\"CONNECTION_LIST\":   %02X,", connection_list);)            // Who is attached
  SEND(ALL, sprintf(_xs, "\"RUNNING_MINUTES\":   %0.2f,", run_time_seconds() / 60.0);) // On Time
  WiFi_MAC_address(str_c);
  SEND(ALL, sprintf(_xs, "\"WiFi_MAC\":          \"%02X:%02X:%02X:%02X:%02X:%02X\",", str_c[0], str_c[1], str_c[2], str_c[3], str_c[4],
                    str_c[5]);)
  WiFi_my_IP_address(str_c);
  SEND(ALL, sprintf(_xs, "\"WiFi_IP_ADDRESS\":   \"%s\",", str_c);)

  if ( json_wifi_ssid[0] == 0 )                                                     // The SSID is undefined
  {
    SEND(ALL, sprintf(_xs, "\"WiFi_MODE\":         \"Access Point: %s\",", str_c);) // Print out the IP address
  }
  else
  {
    SEND(ALL, sprintf(_xs, "\"WiFi_MODE\":         \"Connected to %s\",", (char *)&json_wifi_ssid);)
  }

  strcat(_xs, "\"");
  serial_to_all(_xs, ALL);

  SEND(ALL, sprintf(_xs, "\"VERSION\":          %s, ", SOFTWARE_VERSION);) // Current software version

  nvs_get_i32(my_handle, NONVOL_PS_VERSION, &j);
  SEND(ALL, sprintf(_xs, "\"PS_VERSION\":        %d,", j);)                // Current persistent storage version
                                                                           /*
                                                                            *  All done, return
                                                                            */
  serial_to_all(_xs, EVEN_ODD_END);                                        // End the even odd line
  SEND(ALL, sprintf(_xs, "}\r\n");)

  return;
}

/*-----------------------------------------------------
 *
 * @function: show_names
 *
 * @brief: Display the list of names
 *
 * @return: None
 *
 *-----------------------------------------------------
 *
 * If the name is Loop and display the settings
 *
 *-----------------------------------------------------*/

static void show_names(int v)
{
  return;

  /*
   *  All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: set_trace
 *
 * @brief:    Turn the software trace on and off
 *
 * @return: None
 *
 *-----------------------------------------------------
 *
 * XOR the current trace level with the new input from
 * the user.
 *
 * XOR allows the user to turn settings off without
 * affecting the other settings.
 *
 *-----------------------------------------------------*/
static void set_trace(int trace)         // Trace mask on or off
{
  unsigned int i;

  if ( trace == 0 )                      // Used to turn off tracing
  {
    is_trace = 0;
  }
  is_trace ^= trace;                     // XOR the input
  is_trace |= (DLT_CRITICAL | DLT_INFO); // Info and critical is always enabled

  i = 0;
  while ( dlt_names[i].dlt_text != 0 )   // Print the help
  {
    if ( (is_trace & dlt_names[i].dlt_mask) != 0 )
    {
      SEND(ALL, sprintf(_xs, "\r\n+ ");)
    }
    else
    {
      SEND(ALL, sprintf(_xs, "\r\n  ");)
    }
    SEND(ALL, sprintf(_xs, "%03d %s", dlt_names[i].dlt_mask, dlt_names[i].dlt_text);)
    i++;
  }

  SEND(ALL, sprintf(_xs, "\r\n");)

  return;
}

/*-----------------------------------------------------
 *
 * @function: good_input
 *
 * @brief:    Determine if the input is valid and can be used
 *
 * @return:   TRUE if the input is valid
 *
 *-----------------------------------------------------
 *
 * The input is valid if
 *
 * 1 - No input is required
 * 2 - The input text is not empty
 * 3 - The JSON is not locked
 * 4 - The JSON does not require a lock
 *
 *-----------------------------------------------------*/
static bool good_input(unsigned int conversion, // What kind of input is it?
                       char         next,       // Next input character
                       unsigned int show        // Item display status
)
{

  if ( (conversion == IS_VOID) || (conversion == IS_FIXED) )
  {
    return true;                                // No input required
  }

  if ( (next == ',') || (next == '}') )         // Empty field
  {
    return false;
  }

  if ( (show & LOCK) == 0 )                     // This item is not locked
  {
    return true;
  }

  return false;                                 // Must be locked
}

/*-----------------------------------------------------
 *
 * @function: json_find_first
 *
 * @brief:    Read a number from the input stream
 *
 * @return:   Next number from the input stream
 *
 *-----------------------------------------------------
 *
 * The input stream is a JSON array of numbers, ex:
 * [1, 2, 3, 4, 5]
 *
 * This function will read the next number from the array and return it.
 *
 * Exceptions.
 *
 * Quotes are removed.
 * Double quotes are converted to a space and comma
 *
 *-----------------------------------------------------*/
static int next_value;     // Index to the next value to read

bool json_find_first(void) // Find the first element starting with [
{
  next_value = 0;
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "json_find_first()");))

  /*
   *  Find the start of the JSON
   */
  while ( input_JSON[next_value] != '[' ) // Look for an opening array
  {
    if ( input_JSON[next_value] == 0 )
    {
      next_value = 0;                     // Reached the end, exit
      return false;                       // Report nothing here
    }
    next_value++;                         // Try the next
  }

  /*
   *  Found it, advance and return
   */
  next_value++; // Skip past the opening [
  DLT(DLT_CALIBRATION, SEND(ALL, sprintf(_xs, "Start of array @%d characters", next_value);))
  return true;  // Show we have something
}

/*----------------------------------------------------------------
 *
 * function: json_get_array_next()
 *
 * brief: Extract the next number from an array
 *
 * return: Value extracted from the array
 *
 *----------------------------------------------------------------
 *
 * The input is a text array of numbers in the form
 *
 * 1, 2, 3, 4....<NULL>
 *
 * The function looks for the first comma and then does an atof
 * conversion of the text.
 *
 * The function also handles the case where the text array is
 * of the form "1", "2", "3""4" by converting the quotes to spaces
 * and "" to <space><comma>
 *
 *----------------------------------------------------------------*/
bool json_get_array_next(int   type,  //  Expected input type
                         void *value) // Where to put the result
{
  int i;

  /*
   *  Check to see if this is the first time through and if so, filter the data
   */
  if ( type == IS_FIRST )
  {
    i = 0;
    while ( (input_JSON[i] != 0) && (input_JSON[i] != '!') )
    {
      if ( input_JSON[i] == '"' )       // The next character is a quote
      {
        input_JSON[i] = ' ';            // Make it a space
        if ( input_JSON[i + 1] == '"' ) // And check that the one after that
        {                               // isn't another quote
          input_JSON[i + 1] = ',';      // And if it is, make it a comma
        }
      }

      if ( input_JSON[i] == '\r' )      // The next character is a carriage return
        input_JSON[i] = ',';            // Make it a comma
      i++;
    }
    next_value = 0;
    return 0;
  }

  /*
   *  Check to see if it is time to leave
   */
  if ( (input_JSON[next_value] == 0) || (input_JSON[next_value] == ']') || (input_JSON[next_value] == '!') ) // Bumped up to the end
  {
    return false;
  }

  /*
   *  Convert the next field
   */
  switch ( type )
  {
    case IS_FLOAT:
      *(real_t *)value = atof(&input_JSON[next_value]); // Float
      break;

    case IS_VOID:
      break;
  }

  /*
   *  Prepare the next field
   */
  while ( input_JSON[next_value] != ',' )                                                                      // Got the next field
  {
    next_value++;
    if ( (input_JSON[next_value] == 0) || (input_JSON[next_value] == ']') || (input_JSON[next_value] == '!') ) // Bumped up to the end
    {
      return true;
    }
  }

  next_value++;

  /*
   * All done, return
   */
  return true;
}
