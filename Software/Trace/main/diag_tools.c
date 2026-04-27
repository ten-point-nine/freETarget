
/*******************************************************************************
 *
 * diag_tools.c
 *
 * Debug and test tools
 *
 * See
 * https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
 *
 ******************************************************************************/

#include "ctype.h"
#include "driver\gpio.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "gpio_types.h"
#include "serial_io.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

#include "freETarget.h"
#include "board_assembly.h"
#include "helpers.h"
#include "http_client.h"
#include "http_test.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "gpio.h"
#include "gpio_define.h"
#include "json.h"
#include "timer.h"

extern volatile time_count_t paper_time;

static void show_test_help(void);

/*
 * Diagnostic typedefs
 */
typedef struct
{
  char *help;      // Help text
  void (*f)(void); // Function to execute the test
} self_test_t;

static const self_test_t test_list[] = {
    {"Help",                              &show_test_help            },
    {"Factory test",                      &factory_test              },
    {"- Digital",                         0                          },
    {"Digital inputs",                    &digital_test              },
    {"- Timer & PCNT test",               0                          },
    {"Show the current time",             &show_time                 },
    {"- Communiactions Tests",            0                          },
    {"Test WiFi as a station",            &WiFi_station_init         },
    {"Enable the WiFi Server",            &WiFi_server_test          },
    {"Scan for access points (APs)",      &WiFi_AP_scan_test         },
    {"- HTTP tests",                      0                          },
    {"DNS Lookup test",                   &http_DNS_test             },
    {"Send to server test",               &http_send_to_server_test  },
    {"Start web server",                  &http_server_test          },
    {"-Interrupt Tests",                  0                          },

    {"- Software tests",                  0                          },

    {"",                                  0                          }
};

const dlt_name_t dlt_names[] = {
    {DLT_CRITICAL,      "DLT_CRITICAL",      'E'}, // Prevents target from working
    {DLT_INFO,          "DLT_INFO",          'I'}, // Running information
    {DLT_APPLICATION,   "DLT_APPLICATION",   'A'}, // FreeTarget.c and compute.c logging
    {DLT_COMMUNICATION, "DLT_COMMUNICATION", 'C'}, // WiFi and other communications information
    {DLT_DIAG,          "DLT_DIAG",          'H'}, // Hardware diagnostics
    {DLT_DEBUG,         "DLT_DEBUG",         'D'}, // Software debugging information
    {DLT_VERBOSE,       "DLT_VERBOSE",       'x'}, // Calibration verbose information
    {DLT_AMB,           "DLT_AMB",           'M'}, // Special debug messages
    {0,                 0,                   0  }
};

/*-----------------------------------------------------
 *
 * @function: self_test()
 *
 * @brief:    General purpose test driver
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * The tests are contained in a structure.  This function
 * look into the structure to execute the appropriate test
 *
 *-----------------------------------------------------*/
unsigned int next_test(char ch, unsigned int test_ID)
{
  if ( ch == '-' )           // Test separator?
  {
    test_ID += 20;           // Go to the next second decade
    test_ID -= test_ID % 10; // Round down to the zero
    test_ID--;
  }
  else
  {
    test_ID++;
  }

  return test_ID;
}

void self_test(unsigned int test) // What test to execute
{
  unsigned int i;
  unsigned int test_ID;           // Computed test ID

  /*
   *  Switch over to test mode
   */
  run_state |= IN_TEST;      // Show the test is running

  while ( run_state & IN_OPERATION )
  {
    vTaskDelay(10);          // Wait for everyone else to turn off
  }

  /*
   * Figure out what test to run
   */
  i       = 0;
  test_ID = 0;
  while ( test_list[i].help[0] != 0 )                         // Look through the list
  {
    if ( (test_ID == test) && (test_list[i].help[0] != '-') ) // Found the test
    {
      SEND(ALL, sprintf(_xs, "\r\nTest Number %2d - %s\r\n", test_ID, test_list[i].help);)
      test_list[i].f();                                       // Execute the test
      run_state &= ~IN_TEST;                                  // Exit the test
      return;
    }
    i++;
    test_ID = next_test(test_list[i].help[0], test_ID);
  }

  /*
   * Have not found the test
   */
  show_test_help();

  /*
   *  All done, return;
   */
  run_state &= ~IN_TEST;     // Exit the test
  return;
}

/*---------------------------------------------------------------------
 * Print out the help text
 *-------------------------------------------------------------------*/
static void show_test_help(void)
{
  unsigned int i;
  unsigned int test_ID;

  i       = 0;
  test_ID = 0;
  while ( test_list[i].help[0] != 0 )
  {
    if ( test_list[i].help[0] != '-' )
    {
      SEND(ALL, sprintf(_xs, "\r\n%2d - %s", test_ID, test_list[i].help);)
    }
    else
    {
      SEND(ALL, sprintf(_xs, "\r\n\n%s", test_list[i].help);)
    }
    i++;
    test_ID = next_test(test_list[i].help[0], test_ID);
  }
  SEND(ALL, sprintf(_xs, "\r\n\n");)
  return;
}
/*-----------------------------------------------------
 *
 * @function: factory_test()
 *
 * @brief:    Test all the things we can test
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * This is the factory test to test all of the circuit
 * elements.
 *
 *-----------------------------------------------------*/

#define PASS_RUNNING RUN_MASK
#define PASS_A       0x0100
#define PASS_B       0x0200
#define PASS_C       0X0400
#define PASS_D       0x0800
#define PASS_VREF    0x1000
#define PASS_MASK    (PASS_RUNNING | PASS_A | PASS_B | PASS_VREF)
#define PASS_TEST    (PASS_RUNNING | PASS_C)

#define FACTORY_TEST 1 // Execute a full factory test
#define SENSOR_TEST  0 // Execute a sensor only test

bool factory_test(void)
{
  return do_factory_test(FACTORY_TEST);
}


bool do_factory_test(bool test_run)
{
  bool   passed_once;           // Passed all of the tests at least once

  passed_once  = false;

  /*
   *  Force the refernce voltages - Incase the board has been uninitialized
   */

  /*
   * Ready to start the test
   */
  SEND(ALL, sprintf(_xs, "\r\nFirmware version: %s", SOFTWARE_VERSION);)

  if ( test_run )
  {
    SEND(ALL, sprintf(_xs, "\r\n");)

  }
  /*
   *  Begin test
   */
  run_state = IN_TEST;

  /*
   * Loop and poll the various inputs and output
   */
  while ( 1 )
  {

    vTaskDelay(ONE_SECOND / 2);
  }

  /*
   *  The test has been terminated
   */
  return passed_once;
}

/*******************************************************************************
 *
 * @function: POST_version()
 *
 * @brief: Show the Version String
 *
 * @return: None
 *
 *******************************************************************************
 *
 *  Common function to show the version. Routed to the selected
 *  port(s)
 *
 *******************************************************************************/
void POST_version(void)
{
  SEND(ALL, sprintf(_xs, "\r\n{\"VERSION\": %s}\r\n", SOFTWARE_VERSION);)

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: do_dlt
 *
 * @brief:    Check for a DLT log and print the time
 *
 * @return:   TRUE if the DLT should be printed
 *
 *----------------------------------------------------------------
 *
 * is_trace is compared to the log level and if valid the
 * current time stamp is printed
 *
 * DLT_INFO level is  always printed
 * DLT_CRITICAL level is printed with an error
 *
 * Console colours
 *
 * E - Error       - Red
 * I - Information - Green
 * W - Warning     - Yellow
 *
 *--------------------------------------------------------------*/
bool do_dlt(           //
    unsigned int level // Trace logging level
)
{
  char         dlt_id = 'I';
  unsigned int i;

                       /*
                        * Return if the current level is not enabled in is_trace
                        */
  if ( (level & (is_trace | DLT_CRITICAL | DLT_INFO)) == 0 ) // DLT_CRITICAL are always set in is_trace
  {
    return false;                                            // Send out if the trace is higher than the level
  }

  if ( (level & DLT_VERBOSE)                                 // This message is Verbose
       && (is_trace & DLT_VERBOSE) == 0 )                    // but Verbose is not enabled
  {
    return false;                                            // Don't send out the message
  }

  level = level & ~DLT_VERBOSE;                              // Clear the verbose bit for the rest of the processing

  /*
   *  Loop through and see what trace level has been enabled
   */
  i = 0;
  while ( dlt_names[i].dlt_text != 0 )            // All the DLT levels
  {
    if ( (dlt_names[i].dlt_mask & (level)) != 0 ) // This level is active (with and withoug VERBOSE)
    {
      dlt_id = dlt_names[i].dlt_id;               // Use the Verbose ID

      SEND(ALL, sprintf(_xs, "\r\n%c (%.3f) ", dlt_id, run_time_ms() / 1000.);)
      return true;                                // Send out the message
    }

    i++;
  }

  /*
   *   We did not find the DLT level, return false to not send out the message
   */

  return false;
}

