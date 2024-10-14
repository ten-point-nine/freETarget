
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

#include "stdbool.h"
#include "stdio.h"
#include "serial_io.h"
#include "gpio_types.h"
#include "driver\gpio.h"
#include "ctype.h"

#include "freETarget.h"
#include "gpio.h"
#include "diag_tools.h"
#include "analog_io.h"
#include "json.h"
#include "timer.h"
#include "esp_timer.h"
#include "dac.h"
#include "pwm.h"
#include "pcnt.h"
#include "analog_io.h"
#include "gpio_define.h"
#include "WiFi.h"
#include "compute_hit.h"

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define GRID_SIDE 25                              // Should be an odd number
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))

extern volatile unsigned long paper_time;


static void show_test(void);

/* 
 * JSON message typedefs
 */
typedef struct  {
  char*             help;     // Help test ID 
  int             test_ID;    // Test number
  void         (*f)(void);    // Function to execut the test
  } self_test_t;

static const self_test_t test_list[] = {
  {"Factory test",                    T_FACTORY,      &factory_test}, 
  {"-DIGITAL", 0, 0},
  {"Digital inputs",                  T_DIGITAL,      &digital_test},
  {"Advance paper backer",            T_PAPER,        &paper_test},
  {"LED brightness test",             T_LED,          &LED_test},
  {"Status LED driver",               T_STATUS,       &status_LED_test},
  {"Temperature and sendor test",     T_TEMPERATURE,  &analog_input_test},
  {"DAC test",                        T_DAC,          &DAC_test},
  {"Rapid fire LED test",             T_RAPID_LEDS,   &rapid_LED_test},
  {"-Timer & PCNT test", 0, 0},
  {"PCNT test all",                   T_PCNT,         &pcnt_test},
  {"PCNT calibration",                T_PCNT,         &pcnt_cal},
  {"Sensor POST test",                T_SENSOR,       &sensor_test},
  {"Timers not stopping",             T_PCNT_STOP,    &pcnt_1},
  {"Timers not running",              T_PCNT_SHORT,   &pcnt_2},
  {"Timers start - stop together",    T_PCNT_FREE,    &pcnt_3},
  {"Timers cleared",                  T_PCNT_CLEAR,   &pcnt_4},
  {"Turn the oscillator on and off",  T_SENSOR,       &sensor_test},
  {"Turn the RUN lines on and off",   T_RUN_ALL,      &timer_run_all},
  {"Trigger NORTH from a function generator", T_RUN_NORTH, &test_north},
  {"-Communiations Tests", 0, 0},
  {"AUX serial port test",            T_AUX_SERIAL,   &serial_port_test},
  {"Test WiFi as a station",          T_WIFI_STATION, &wifi_station_init},
  {"Enable the WiFi Server",          T_WIFI_SERVER,  &wifi_server_test},
  {"Enable the WiFi AP",              T_WIFI_AP,      &WiFi_AP_init,

  {"Loopback the TCPIP data",         T_LOOPBACK,     &WiFi_loopback_test},
  {"Loopback WiFi",                   T_LOOPBACK,     &wifi_loopback_test},
  {"-Interrupt Tests", 0, 0},
  {"Polled target test",              T_POLLED,       &sensor_polled_test},
  {"Interrupt target test",           T_TARGET_2,     &sensor_polled_test},
  {"", 0, 0}
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

void self_test
(
  unsigned int test                 // What test to execute
)
{
  unsigned int i;

/*
 *  Switch over to test mode 
 */
  run_state |= IN_TEST;                 // Show the test is running 
  
  while ( run_state & IN_OPERATION )
  {
    vTaskDelay(10);                     // Wait for everyone else to turn off
  }
  freeETarget_timer_pause();            // Stop interrupts

/*
 * Figure out what test to run
 */
  i = 0;
  while ( test_list[i].help[0] != 0 )   // Look through the list
  {
    if ( test_list[i].test_id == test ) // Found the test
    {
      test_list[i].f();                 // Execute the test
      run_state &= ~IN_TEST;            // Exit the test 
      freeETarget_timer_start();        // Start interrupts
      return;
    }
  }

/*
 * Have not found the test
 */
  show_test();

 /* 
  *  All done, return;
  */
  run_state &= ~IN_TEST;            // Exit the test 
  freeETarget_timer_start();        // Start interrupts
  return;
}

static void show_test(void)
{
  int i;

  i = 0;
  while ( test_list[i].help[0] != 0 )
  {
    SEND(sprintf(_xs, "\r\n%d - %c", test_list[i].test_id, test_list[i].test_help);)
  }

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

#define PASS_RUNNING 0x00FF
#define PASS_A       0x0100
#define PASS_B       0x0200
#define PASS_C       0X4000
#define PASS_D       0x8000
#define PASS_MASK    (PASS_RUNNING | PASS_A | PASS_B)
#define PASS_TEST    (PASS_RUNNING | PASS_C)

bool factory_test(void)
{
  int i, percent;
  int running;            // Bit mask from run flip flops
  int dip;                // Input from DIP input
  char ch;
  char ABCD[] = "DCBA";   // DIP switch order
  int  pass;              // Pass YES/NO
  bool passed_once;       // Passed all of the tests at least once
  float volts[4];
  int  motor_toggle;      // Toggle motor on an off

/*
 *  Force the refernce voltages - Incase the board has been uninitialized
 */
  if ( (json_vref_lo == 0) || (json_vref_hi == 0))
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
  SEND(sprintf(_xs, "\r\nFactory Test");)
  SEND(sprintf(_xs, "\r\nFirmware version: %s   Board version: %d",SOFTWARE_VERSION, revision());)
  SEND(sprintf(_xs, "\r\n");)
  SEND(sprintf(_xs, "\r\nHas the tape seal been removed from the temperature sensor?");)
  SEND(sprintf(_xs, "\r\nPress 1 & 2 or ! to continue\r\n");)
  while ( (DIP_SW_A == 0) || (DIP_SW_B == 0) )
  {
    if ( serial_available(ALL) )
    {
      if ( serial_getch(ALL) == '!' )
      {
        break;
      }
    }
    continue;
  }

/*
 *  Begin test
 */
  arm_timers();
  pass = 0;
  passed_once = false;
  percent = 0;
  motor_toggle = 0;
/*
 * Loop and poll the various inputs and output
 */
  while (1)
  {
    running = is_running();
    if ( running == 0x00FF )
    {
      pass |= PASS_RUNNING;
    }
    SEND(sprintf(_xs, "\r\nSens: ");)
    for (i=0; i != 8; i++)
    {
      if ( i == 4 )
      {
        SEND(sprintf(_xs, " ");)
      }
      if (running & (1<<i))
      {
        SEND(sprintf(_xs, "%c", find_sensor(1<<i)->short_name );)
      }
      else
      {
        SEND(sprintf(_xs, "-");)
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

    for (i=3; i >= 0; i--)
    {
      if ((dip & (1<<i)) == 0)
      {
        SEND(sprintf(_xs, "%c", ABCD[i] );)
      }
      else
      {
        SEND(sprintf(_xs, "-");)
      }
    }

    SEND(sprintf(_xs, "  12V: %4.2fV", v12_supply());)
    SEND(sprintf(_xs, "  Temp: %4.2fC", temperature_C());)
    SEND(sprintf(_xs, "  Humidiity: %4.2f%%", humidity_RH());)

    if ( v12_supply() >= V12_WORKING)            // Skip the motor and LED test if 12 volts not used
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

    if ( (pass == PASS_MASK) || (pass == PASS_TEST) ) 
    {
      set_status_LED(LED_GOOD);
      SEND(sprintf(_xs, "  PASS");)
      vTaskDelay(ONE_SECOND);
      arm_timers();
      pass = 0;
      passed_once = true;
    }

/*
 *  See if there is any user controls 
 */
    if ( serial_available(ALL) )
    {
      ch = serial_getch(ALL);
      switch (ch)
      {
        default:
        case 'R':               // Reset the test
        case 'r':
          pass = 0;             // Reset the pass/fail
          arm_timers();
          break;

        case 'X':               // Exit
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
  unsigned int i;                          // Iteration counter
  unsigned int count, toggle, running;     // Cycle counter
  DLT(DLT_INFO, SEND(sprintf(_xs, "POST_counters()");))
  
/*
 *  Test 1, Make sure we can turn off the reference clock
 */
  count = 0;
  gpio_set_level(OSC_CONTROL, OSC_OFF);   // Turn off the oscillator
  toggle = gpio_get_level(REF_CLK);
  for  (i=0; i != 1000; i++)               // Try 1000 times
  {
    if ( (gpio_get_level(REF_CLK) ^ toggle) != 0 )  // Look for a change
    {
      count++;
      toggle = gpio_get_level(REF_CLK);
    }
  }
  
  if ( count != 0 )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Reference clock cannot be stopped");))
    set_diag_LED(LED_FAIL_CLOCK_STOP, 0);
  }

/*
 *  Test 2, Make sure we can turn the reference clock on
 */
  count = 0;
  gpio_set_level(OSC_CONTROL, OSC_ON);
  toggle = gpio_get_level(REF_CLK);
  for  (i=0; i != 1000; i++)               // Try 1000 times
  {
    if ( (gpio_get_level(REF_CLK) ^ toggle) != 0 )  // Look for a change
    {
      count++;
      toggle = gpio_get_level(REF_CLK);
    }
  }

  if ( count == 0  )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Reference clock cannot be started");))
    set_diag_LED(LED_FAIL_CLOCK_START, 0);
  }

/*
 *  Test 3, Make sure we can turn the triggers off
 */
  gpio_set_level(STOP_N, 0);        // Clear the latch
  gpio_set_level(STOP_N, 1);        // and reenable it
  running = is_running();
  if ( running != 0  )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Stuck bit in run latch: ");))
    for (i=N; i <= W; i++)
    {
      if ( running & s[i].low_sense.run_mask ) 
      {
        set_diag_LED(s[i].low_sense.diag_LED,0);
      }
      if ( running & s[i].high_sense.run_mask ) 
      {
        set_diag_LED(s[i].high_sense.diag_LED,0);
      }

    }
  }      
  vTaskDelay(ONE_SECOND);

/*
 * Test 4, Trigger the timers
 */
  gpio_set_level(STOP_N, 0);          // Clear the latch
  gpio_set_level(STOP_N, 1);
  gpio_set_level(CLOCK_START, 1);     // Triger the run latch
  gpio_set_level(CLOCK_START, 0);
  gpio_set_level(CLOCK_START, 1);
  if ( is_running() != 0xFF  )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to start clock in run latch: %02X", is_running());))
    set_diag_LED(LED_FAIL_RUN_STUCK, 0);
  }

/*
 * We get here only if all of the tests pass
 */
  return 1;
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
void show_sensor_status
  (
  unsigned int   sensor_status
  )
{
  unsigned int i;
  
  SEND(sprintf(_xs, " Latch:");)

  for (i=N; i<=W; i++)
  {
    if ( sensor_status & (1<<i) )   {SEND(sprintf(_xs, "%c", find_sensor(1<<i)->short_name);)}
    else                            {SEND(sprintf(_xs, ".");)}
  }

  SEND(sprintf(_xs, "  Face Strike: %d", face_strike);)
  
  SEND(sprintf(_xs, "  Temperature: %4.2f", temperature_C());)
  
  SEND(sprintf(_xs, "  Switch:");)
  
  if ( DIP_SW_A == 0 )
  {
    SEND(sprintf(_xs, "--");)
  }
  else
  {
    SEND(sprintf(_xs, "A1");)
  }
  SEND(sprintf(_xs, " ");)

  if ( DIP_SW_B == 0 )
  {
    SEND(sprintf(_xs, "--");)
  }
  else
  {
    SEND(sprintf(_xs, "B2");)
  }

  if (( sensor_status & 0x0f) == 0x0f)
  {
    SEND(sprintf(_xs, " PASS");)
    vTaskDelay(ONE_SECOND);                // Wait for click to go away
  }    

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
void show_sensor_fault
(
  unsigned int   sensor_status
)
{
  unsigned int i;
  
  for (i=N; i<=W_HI; i++)
  {
    if ( (sensor_status & (1<<i)) == 0)
    {
      set_diag_LED(find_sensor(1<<i)->diag_LED, 2);
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
bool do_dlt
  (
  unsigned int level
  )
{ 
  char dlt_id = 'I';

  if ( (level & (is_trace | DLT_INFO | DLT_CRITICAL)) == 0 )  //  DLT_INFO | DLT_CRITICAL are always set in is_trace
  {
    return false;                 // Send out if the trace is higher than the level 
  }

  if ( level & DLT_CRITICAL)      { dlt_id = 'E'; }    // Red
  if ( level & DLT_INFO)          { dlt_id = 'I'; }    // Green
  if ( level & DLT_APPLICATION)   { dlt_id = 'W'; }    // Yellow
  if ( level & DLT_DEBUG)         { dlt_id = 'D'; }    // White - Debug
  if ( level & DLT_DIAG)          { dlt_id = 'H'; }    // White - Hardware
  if ( level & DLT_COMMUNICATION) { dlt_id = 'C'; }    // White - Communications

  SEND(sprintf(_xs, "\r\n%c (%d) ", dlt_id, (int)(esp_timer_get_time()/1000) );)

  return true;
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
void set_diag_LED
(
  char* new_LEDs,           // NEW LED display
  unsigned int duration     // How long the display is present for in seconds
)
{
  set_status_LED(new_LEDs);

/*
 *  Test for infinit wait
 */
  if ( duration == 0 )      // Wait here forever 
  {
    while (1)
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
#define NONE  0
#define SOME  1
#define V12OK 2
#define UNKNOWN 99

bool check_12V(void)
{
  static unsigned int fault_V12 = UNKNOWN;
  float  v12;

/*
 *  Check to see that the witness paper is enabled
 */
  if ( json_paper_time == 0 )             // The witness paper is not used
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
    if ( fault_V12 != NONE)
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

  if ( fault_V12 != V12OK )              // Did we have an error last time?
  {
    set_status_LED(LED_OK_12V);         // Gone, clear the error
    fault_V12 = V12OK;
  }

  return true;
}