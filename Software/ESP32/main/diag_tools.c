
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
#include "gpio_types.h"
#include "serial_io.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

#include "freETarget.h"
#include "http_client.h"
#include "http_test.h"
#include "WiFi.h"
#include "analog_io.h"
#include "compute_hit.h"
#include "dac.h"
#include "diag_tools.h"
#include "gpio.h"
#include "gpio_define.h"
#include "json.h"
#include "pcnt.h"
#include "pwm.h"
#include "timer.h"

extern volatile unsigned long paper_time;

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
    {"Help",                              &show_test_help          },
    {"Factory test",                      &factory_test            },
    {"- DIGITAL",                         0                        },
    {"Digital inputs",                    &digital_test            },
    {"Advance paper backer",              &paper_test              },
    {"LED brightness test",               &LED_test                },
    {"Status LED driver",                 &status_LED_test         },
    {"Analog input test",                 &analog_input_test       },
    {"DAC test",                          &DAC_test                },
    {"- Timer & PCNT test",               0                        },
    {"PCNT timers not stopping",          &pcnt_1                  },
    {"PCNT timers not running",           &pcnt_2                  },
    {"PCNT timers start - stop together", &pcnt_3                  },
    {"PCNT Timers cleared",               &pcnt_4                  },
    {"PCNT test all",                     &pcnt_all                },
    {"PCNT calibration",                  &pcnt_cal                },
    {"Sensor POST test",                  &POST_counters           },
    {"Turn the oscillator on and off",    &timer_cycle_oscillator  },
    {"Turn the RUN lines on and off",     &timer_run_all           },
    {"Show the current time",             &show_time               },
    {"- Communiations Tests",             0                        },
    {"AUX serial port test",              &serial_port_test        },
    {"Test WiFi as a station",            &WiFi_station_init       },
    {"Enable the WiFi Server",            &WiFi_server_test        },
    {"Enable the WiFi AP",                &WiFi_AP_init            },
    {"Loopback the TCPIP data",           &WiFi_loopback_test      },
    {"Loopback WiFi",                     &WiFi_loopback_test      },
    {"- HTTP tests",                      0                        },
    {"DNS Lookup test",                   &http_DNS_test           },
    {"Send to server test",               &http_send_to_server_test},
    {"Start web server",                  &http_server_test        },
    {"-Interrupt Tests",                  0                        },
    {"Polled target test",                &polled_target_test      },
    {"Interrupt target test",             &interrupt_target_test   },
    {"",                                  0                        }
};

const dlt_name_t dlt_names[] = {
    {DLT_CRITICAL,      "DLT_CRITICAL",      'E'}, // Prevents target from working
    {DLT_INFO,          "DLT_INFO",          'I'}, // Running information
    {DLT_APPLICATION,   "DLT_APPLICATION",   'A'}, // FreeTarget.c and compute.c logging
    {DLT_COMMUNICATION, "DLT_COMMUNICATION", 'C'}, // WiFi and other communications information
    {DLT_DIAG,          "DLT_DIAG",          'H'}, // Hardware diagnostics
    {DLT_DEBUG,         "DLT_DEBUG",         'D'}, // Software debugging information
    {DLT_SCORE,         "DLT_SCORE",         'S'}, // Display timing in the score message
    {DLT_HEARTBEAT,     "DLT_HEARTBEAT",     'H'}, // Heartbeat tick
    {DLT_HTTP,          "DLT_HTTP",          'H'}, // Log HTTP events
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

void self_test(unsigned int test // What test to execute
)
{
  unsigned int i;
  unsigned int test_ID;          // Computed test ID

  /*
   *  Switch over to test mode
   */
  run_state |= IN_TEST;      // Show the test is running

  while ( run_state & IN_OPERATION )
  {
    vTaskDelay(10);          // Wait for everyone else to turn off
  }
  freeETarget_timer_pause(); // Stop interrupts

  /*
   * Figure out what test to run
   */
  i       = 0;
  test_ID = 0;
  while ( test_list[i].help[0] != 0 )                         // Look through the list
  {
    if ( (test_ID == test) && (test_list[i].help[0] != '-') ) // Found the test
    {
      SEND(sprintf(_xs, "\r\nTest Number %2d - %s\r\n", test_ID, test_list[i].help);)
      test_list[i].f();                                       // Execute the test
      run_state &= ~IN_TEST;                                  // Exit the test
      freeETarget_timer_start();                              // Start interrupts
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
  freeETarget_timer_start(); // Start interrupts
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
      SEND(sprintf(_xs, "\r\n%2d - %s", test_ID, test_list[i].help);)
    }
    else
    {
      SEND(sprintf(_xs, "\r\n\n%s", test_list[i].help);)
    }
    i++;
    test_ID = next_test(test_list[i].help[0], test_ID);
  }
  SEND(sprintf(_xs, "\r\n\n");)
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
#define PASS_MASK    (PASS_RUNNING | PASS_A | PASS_B)
#define PASS_TEST    (PASS_RUNNING | PASS_C)

bool factory_test(void)
{
  int   i, percent;
  int   running;         // Bit mask from run flip flops
  int   dip;             // Input from DIP input
  char  ch;
  char  ABCD[] = "DCBA"; // DIP switch order
  int   pass;            // Pass YES/NO
  bool  passed_once;     // Passed all of the tests at least once
  float volts[4];
  int   motor_toggle;    // Toggle motor on an off

  pass         = 0;      // Pass YES/NO
  passed_once  = false;
  percent      = 0;
  motor_toggle = 0;

  /*
   *  Force the refernce voltages - Incase the board has been uninitialized
   */
  if ( (json_vref_lo == 0) || (json_vref_hi == 0) )
  {
    volts[VREF_LO] = 1.25;
    volts[VREF_HI] = 2.00;
    volts[VREF_2]  = 0.00;
    volts[VREF_3]  = 0.00;
    DAC_write(volts);
  }
  /*
   * Ready to start the test
   */
  SEND(sprintf(_xs, "\r\nFirmware version: %s   Board version: %d", SOFTWARE_VERSION, revision());)
  SEND(sprintf(_xs, "\r\n");)
  SEND(sprintf(_xs, "\r\nHas the tape seal been removed from the temperature sensor?");)
  SEND(sprintf(_xs, "\r\nPress 1 & 2 or ! to continue\r\n");)

  while ( pass != (PASS_A | PASS_B) )
  {
    if ( DIP_SW_A != 0 )
    {
      pass |= PASS_A;
    }
    if ( DIP_SW_B != 0 )
    {
      pass |= PASS_B;
    }
    if ( serial_available(ALL) )
    {
      if ( serial_getch(ALL) == '!' )
      {
        pass = (PASS_A | PASS_B);
      }
    }
  }

  /*
   *  Begin test
   */
  run_state = IN_TEST;
  arm_timers();

  /*
   * Loop and poll the various inputs and output
   */
  while ( 1 )
  {
    SEND(sprintf(_xs, "\r\nSens: ");)
    running = is_running();
    pass |= running & RUN_MASK;
    for ( i = 0; i != 8; i++ )
    {
      if ( i == 4 )
      {
        SEND(sprintf(_xs, " ");)
      }
      if ( running & (1 << i) )
      {
        SEND(sprintf(_xs, "%c", find_sensor(1 << i)->short_name);)
      }
      else
      {

        if ( ((find_sensor(1 << i)->short_name) == 'N') // Is this the North Sensor?
             && (json_pcnt_latency == 0) )              // and pcnt_latency is disabled?
        {
          SEND(sprintf(_xs, "-");)                      // Then mark it as unused
        }
        else
        {
          SEND(sprintf(_xs, ".");)                      // Show N/A for this iput
        }
      }
    }

    dip = read_DIP();
    SEND(sprintf(_xs, "  DIP: ");)
    if ( DIP_SW_A )
    {
      set_status_LED("-W-");
      pass |= PASS_A;
    }
    else
    {
      set_status_LED("- -");
    }

    if ( DIP_SW_B )
    {
      set_status_LED("--W");
      pass |= PASS_B;
    }
    else
    {
      set_status_LED("-- ");
    }

    if ( DIP_SW_C )
    {
      pass |= PASS_C;
    }

    if ( DIP_SW_D )
    {
      pass |= PASS_D;
    }

    for ( i = 3; i >= 0; i-- )
    {
      if ( (dip & (1 << i)) == 0 )
      {
        SEND(sprintf(_xs, "%c", ABCD[i]);)
      }
      else
      {
        SEND(sprintf(_xs, "-");)
      }
    }

    SEND(sprintf(_xs, "  12V: %4.2fV", v12_supply());)
    SEND(sprintf(_xs, "  Temp: %4.2fC", temperature_C());)
    SEND(sprintf(_xs, "  Humidiity: %4.2f%%", humidity_RH());)

    if ( v12_supply() >= V12_WORKING ) // Skip the motor and LED test if 12 volts not used
    {
      SEND(sprintf(_xs, "  M");)
      if ( motor_toggle )
      {
        SEND(sprintf(_xs, "+");)
        DCmotor_on_off(true, ONE_SECOND);
      }
      else
      {
        SEND(sprintf(_xs, "-");)
        DCmotor_on_off(false, 0);
      }
      motor_toggle ^= 1;

      set_LED_PWM_now(percent);
      SEND(sprintf(_xs, "  LED: %3d%% ", percent);)
      percent = percent + 25;
      if ( percent > 100 )
      {
        percent = 0;
      }
    }

    if ( ((pass & PASS_MASK) == PASS_MASK) )
    {
      set_status_LED(LED_GOOD);
      SEND(sprintf(_xs, "  PASS");)
      vTaskDelay(ONE_SECOND);
      arm_timers();
      pass        = 0;
      passed_once = true;
    }

    /*
     *  See if there is any user controls
     */
    if ( serial_available(ALL) )
    {
      ch = serial_getch(ALL);
      switch ( ch )
      {
        default:
        case 'R':   // Reset the test
        case 'r':
          pass = 0; // Reset the pass/fail
          arm_timers();
          break;

        case 'X':   // Exit
        case 'x':
        case '!':
          DCmotor_on_off(false, 0);
          if ( passed_once == true )
          {
            SEND(sprintf(_xs, "\r\nTest completed successfully\r\n");)
          }
          else
          {
            SEND(sprintf(_xs, "\r\nTest finished without PASS\r\n");)
          }
          return passed_once;
      }
    }

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
  SEND(sprintf(_xs, "\r\n{\"VERSION\": %s}\r\n", SOFTWARE_VERSION);)

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: void POST_counters()
 *
 * @brief: Verify the counter circuit operation
 *
 * @return: TRUE if the tests pass
 *          Never if the tests fail
 *
 *----------------------------------------------------------------
 *
 *  Trigger the counters from inside the circuit board and
 *  read back the results and look for an expected value.
 *
 *  Return only if all of the tests pass
 *
 *  Test 1, Make sure the 10MHz clock is running
 *  Test 2, Clear the flip flops and make sure the run latches are clear
 *  Test 3, Trigger the flip flops and make sure that no run latche are set
 *  Test 4, Trigger a run and verify that the counters change
 *
 *--------------------------------------------------------------*/
bool POST_counters(void)
{
  unsigned int i;                      // Iteration counter
  unsigned int count, toggle, running; // Cycle counter
  DLT(DLT_INFO, SEND(sprintf(_xs, "POST_counters()");))

  /*
   *  Test 1, Make sure we can turn off the reference clock
   */
  count = 0;
  gpio_set_level(OSC_CONTROL, OSC_OFF);            // Turn off the oscillator
  toggle = gpio_get_level(REF_CLK);
  for ( i = 0; i != 1000; i++ )                    // Try 1000 times
  {
    if ( (gpio_get_level(REF_CLK) ^ toggle) != 0 ) // Look for a change
    {
      count++;
      toggle = gpio_get_level(REF_CLK);
    }
  }

  if ( count != 0 )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Reference clock cannot be stopped");))
    set_diag_LED(LED_FAIL_CLOCK_STOP, 10);
    run_state |= IN_FATAL_ERR;
  }

  /*
   *  Test 2, Make sure we can turn the reference clock on
   */
  count = 0;
  gpio_set_level(OSC_CONTROL, OSC_ON);
  toggle = gpio_get_level(REF_CLK);
  for ( i = 0; i != 1000; i++ )                    // Try 1000 times
  {
    if ( (gpio_get_level(REF_CLK) ^ toggle) != 0 ) // Look for a change
    {
      count++;
      toggle = gpio_get_level(REF_CLK);
    }
  }

  if ( count == 0 )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Reference clock cannot be started");))
    set_diag_LED(LED_FAIL_CLOCK_START, 10);
    run_state |= IN_FATAL_ERR;
  }

  /*
   *  Test 3, Make sure we can turn the triggers off
   */
  gpio_set_level(STOP_N, RUN_OFF); // Clear the latch
  gpio_set_level(STOP_N, RUN_GO);  // and reenable it
  running = is_running();
  if ( running != 0 )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Stuck bit in run latch: ");))
    for ( i = N; i <= W; i++ )
    {
      if ( running & s[i].low_sense.run_mask )
      {
        set_diag_LED(s[i].low_sense.diag_LED, 10);
      }
      if ( running & s[i].high_sense.run_mask )
      {
        set_diag_LED(s[i].high_sense.diag_LED, 10);
      }
    }
    run_state |= IN_FATAL_ERR;
  }
  vTaskDelay(ONE_SECOND);

  /*
   * Test 4, Trigger the timers
   */
  gpio_set_level(STOP_N, RUN_OFF);                // Clear the latch
  gpio_set_level(STOP_N, RUN_GO);
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF); // Triger the run latch
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_ON);
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF);
  if ( (is_running() & RUN_MASK) != RUN_MASK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to start clock in run latch: %02X", is_running());))
    set_diag_LED(LED_FAIL_RUN_STUCK, 10);
    run_state |= IN_FATAL_ERR;
  }

  /*
   * Return the err status
   */
  return ((run_state & IN_FATAL_ERR) == 0); // Return TRUE if no errors detected
}

/*----------------------------------------------------------------
 *
 * @function: show_sensor_status()
 *
 * @brief:    Show which sensor flip flops were latched
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * The sensor state NESW or .... is shown for each latch
 * The clock values are also printed
 *
 *--------------------------------------------------------------*/
void show_sensor_status(unsigned int sensor_status)
{
  unsigned int i;

  SEND(sprintf(_xs, " Latch:");)

  for ( i = N; i <= W; i++ )
  {
    if ( sensor_status & (1 << i) )
    {
      SEND(sprintf(_xs, "%c", find_sensor(1 << i)->short_name);)
    }
    else
    {
      SEND(sprintf(_xs, ".");)
    }
  }

  if ( (sensor_status & RUN_MASK) != RUN_MASK )
  {
    SEND(sprintf(_xs, " FAULT");)
  }

  SEND(sprintf(_xs, "  Face Strike: %d", face_strike);)

  /*
   * All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: show_sensor_fault()
 *
 * @brief:    Use the LEDs to show if a sensor failed
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function is intended as a diagnostic to show if a sensor
 * failed to detect a show
 *
 *--------------------------------------------------------------*/
void show_sensor_fault(unsigned int sensor_status)
{
  unsigned int i;

  DLT(DLT_DEBUG, SEND(sprintf(_xs, "show_sensor_fault()");))

  for ( i = N; i <= W; i++ )
  {
    if ( (sensor_status & (1 << i)) == 0 )
    {
      set_diag_LED(find_sensor(1 << i)->diag_LED, 2);
      return;
    }
  }

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

                                                             /*
                                                              *  Loop through and see what trace level has been enabled
                                                              */
  i = 0;
  while ( dlt_names[i].dlt_text != 0 )          // All the DLT levels
  {
    if ( (dlt_names[i].dlt_mask & level) != 0 ) // This level is active
    {
      dlt_id = dlt_names[i].dlt_id;             // Put the DLT_ID at the start of the message
      break;
    }
    i++;
  }

  /*
   *   Print out the message
   */
  SEND(sprintf(_xs, "\r\n%c (%.3f) ", dlt_id, ((float)(esp_timer_get_time()) / 1000000.0));)

  return true;
}
/*----------------------------------------------------------------
 *
 * @function: heartbeat
 *
 * @brief:    Periodically send out the internal status
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function provides a simple output to indicate the current
 * running mode.
 *
 * It is turned on by enabling DLT_HEARTBEAT in the trace
 *
 *--------------------------------------------------------------*/
static char *run_state_text[] = {"IN_STARTUP", "IN_OPERATION", "IN_TEST", "IN_SLEEP", "IN_SHOT", "IN_REDUCTION", 0};
void         heartbeat(void)
{
  char s[128];
  int  i;

  i    = 0;
  s[0] = 0;
  while ( run_state_text[i] != 0 )
  {
    if ( (run_state & (1 << i)) != 0 )
    {
      strcat(s, ", ");
      strcat(s, run_state_text[i]);
    }
    i++;
  }

  DLT(DLT_HEARTBEAT, SEND(sprintf(_xs, "Heartbeat: 60s  run_state: 0X%02X%s", run_state, s);))

  return;
}

/*----------------------------------------------------------------
 *
 * @function: set_diag_LED
 *
 * @brief:    Set the state of the diagnostics LED
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This function is similar to set_status_LED() except that the
 * duration of the LED is configurable
 *
 *--------------------------------------------------------------*/
void set_diag_LED(char        *new_LEDs, // NEW LED display
                  unsigned int duration  // How long the display is present for in seconds
)

{
  set_status_LED(new_LEDs);

  /*
   *  Test for infinite wait
   */
  if ( duration == 0 ) // Wait here forever
  {
    while ( 1 )
    {
      vTaskDelay(ONE_SECOND);
    }
  }

  /*
   *  The fault is displayed for a short time
   */
  vTaskDelay(ONE_SECOND * duration);

  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: check_12V
 *
 * @brief:    Make sure the 12 Volt supply is within limits
 *
 * @return:   TRUE if the 12V is within spec
 *
 *----------------------------------------------------------------
 *
 * The 12V supply is read and compared against limits.
 *
 * V >= 10 V  Green LED          Working
 * 5 < V <=   10 Yellow LED      Caution
 * V <= 5     Red LED            Disabld
 *
 * Return TRUE if the motor can be driven
 *
 * The LEDs are only set on a change from (say) working to caution
 * this is done to prevent the LEDs from flickering.
 *
 * For targets that don't use witness paper, the status LED can be
 * turned off to indicate that that part has been disabled.
 *
 *--------------------------------------------------------------*/
#define NONE    0
#define SOME    1
#define V12OK   2
#define UNKNOWN 99

bool check_12V(void)
{
  static unsigned int fault_V12 = UNKNOWN;
  float               v12;

  /*
   *  Check to see that the witness paper is enabled
   */
  if ( json_paper_time == 0 ) // The witness paper is not used
  {
    set_status_LED(LED_12V_NOT_USED);
    fault_V12 = NONE;
    return false;
  }

  /*
   * Continue on to check the voltage
   */
  v12 = v12_supply();

  if ( v12 <= V12_CAUTION )
  {
    if ( fault_V12 != NONE )
    {
      set_status_LED(LED_NO_12V);
      fault_V12 = NONE;
    }
    return false;
  }

  if ( v12 <= V12_WORKING )
  {
    if ( fault_V12 != SOME )
    {
      set_status_LED(LED_LOW_12V);
      fault_V12 = SOME;
    }
    return false;
  }

  if ( fault_V12 != V12OK )     // Did we have an error last time?
  {
    set_status_LED(LED_OK_12V); // Gone, clear the error
    fault_V12 = V12OK;
  }

  return true;
}