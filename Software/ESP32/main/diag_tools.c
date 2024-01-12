
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

const char* which_one[] = {"North_lo", "East_lo ", "South_lo", "West_lo ", "North_hi", "East_hi ", "South_hi", "West_hi "};

#define TICK(x) (((x) / 0.33) * OSCILLATOR_MHZ)   // Distance in clock ticks
#define RX(Z,X,Y) (16000 - (sqrt(sq(TICK(x)-s[(Z)].x) + sq(TICK(y)-s[(Z)].y))))
#define GRID_SIDE 25                              // Should be an odd number
#define TEST_SAMPLES ((GRID_SIDE)*(GRID_SIDE))

/*******************************************************************************
 *
 * @function: void self_test
 *
 * @brief: Execute self tests based on the jumper settings
 * 
 * @return: None
 *
 *******************************************************************************
 *   
 *   This function is a large case statement with each element
 *   of the case statement 
 * 
 *   Supports a test mode (TEST:99) that emulates an old style Zapple monitor
 *   
 ******************************************************************************/
unsigned int tick;
void zapple(unsigned int test)
{ 
  int run_test;                 // Running Semephore
  char ch;                      // Input character

  run_test = 1;

  while (run_test != 0)
  {
    printf("\r\nTest >");
    test = 0;

    while (1)
    {
      if ( serial_available(CONSOLE) != 0 )
      {
        ch = serial_getch(CONSOLE);
        ch = toupper(ch);
        printf("%c", ch);
        if ( ch != '\r' )
        {
            test = ((test * 10) + (ch -'0')) % 100;
          }
          if ( (ch == '\r') || (ch == '\n') )
          {
            self_test(test);
            break;
          }
          if ( ch == 'X' )
          {
            run_test = 0;
            break;
          }
        }
        vTaskDelay(1);
      }
    }

/*
 *  GO back to normal operation
 */
  return;
}


void self_test
(
  unsigned int test                 // What test to execute
)
{
  int   i;

/*
 *  Switch over to test mode 
 */
  run_state |= IN_TEST;             // Show the test is running 
  while ( (run_state & IN_OPERATION) == IN_OPERATION )
  {
    vTaskDelay(10);
  }
  freeETarget_timer_pause();        // Stop interrupts

/*
 * Figure out what test to run
 */
  switch (test)
  {
/*
 * Test 0, Display the help
 */
    default:                // Undefined, show the tests
    case T_HELP:                
      printf("\r\n 1 - Digital inputs");
      printf("\r\n 2 - Advance paper backer");
      printf("\r\n 3 - LED brightness test");
      printf("\r\n 4 - Status LED driver");
      printf("\r\n 5 - Temperature and sendor test");
      printf("\r\n 6 - DAC test");
      printf("\r\n 7 - PCNT Test");
      printf("\r\n 8 - Sensor POST test");
      printf("\r\n 9 - AUX Port loopback");
      printf("\r\n10 - Polled Target Test");
      printf("\r\n11 - Interrupt Target Test");
      printf("\r\n12 - Test WiFi as an Access Point");
      printf("\r\n13 - Test WiFI as a station"); 
      printf("\r\n14 - Enable the WiFi Server");
      printf("\r\n15 - Loopback the TCPIP data");
      printf("\r\n16 - Loopback AUX connector");
      printf("\r\n17 - Turn the oscillator on and off");
      printf("\r\n18 - Turn the RUN lines on and off");
      printf("\r\n19 - pcnt(1) - Timers not running"); 
      printf("\r\n20 - pcnt(2) - Timers start - stop together"); 
      printf("\r\n21 - pcnt(3) - Timers free running"); 
      printf("\r\n22 - pcnt(4) - Timers cleared"); 
      printf("\r\n99 - Zapple Debug Monitor");
      printf("\r\n");
    break;

/*
 * Test 1, Display GPIO inputs
 */
    case T_DIGITAL: 
      digital_test();
      break;

/*
 * Test 2, Advance the paper
 */
    case T_PAPER:
      paper_test();
      break;

/*
 * Test 3, Set the LED bightness
 */
    case T_LED:
      printf("\r\nCycling the LED");
      for (i=0; i <= 100; i++)
      {
        pwm_set(LED_PWM, i);       
        vTaskDelay(ONE_SECOND/10);
      }
      for (i=100; i >= 0; i--)
      {
        pwm_set(LED_PWM,i);       
        vTaskDelay(ONE_SECOND/10);
      }
      printf(" done\r\n");
      break;

/*
 * Test 4, Set status LEDs
 */
    case T_STATUS:
      status_LED_test();
      break;

/*
 * Test 5,  Analog In
 */
    case T_TEMPERATURE:
      analog_input_test();
      break;

/*
 * Test 6, DAC
 */
    case T_DAC:
      DAC_test();
      break;

/*
 * Test 7, PCNT test
 */
    case T_PCNT:
      pcnt_test(1);
      pcnt_test(2);
      pcnt_test(3);
      pcnt_test(4);
      break;

/*
 *  Test 8: Sensor Trigger
 */
    case T_SENSOR:
      POST_counters();
      break;

/*
 *  Test 9: AUX Serial Port
 */
    case T_AUX_SERIAL:
      serial_port_test();
      break;

/*
 *  Test 10: Polled Target Test
 */
    case T_TARGET:
      polled_target_test();
      break;      
/*
 *  Test 11: Polled Target Test
 */
    case T_TARGET_2:
      interrupt_target_test();
      break; 

/*
 *  Test 12: Start WiFi AP
 */
    case T_WIFI_AP:
      WiFi_AP_init();
      break; 

/*
 *  Test 13: Start WiFi station
 */
    case T_WIFI_STATION:
      WiFi_station_init();
      break; 

/*
 *  Test 14: Enable the WiFi Server
 */
    case T_WIFI_SERVER:
      xTaskCreate(WiFi_tcp_server_task,    "WiFi_tcp_server",      4096, NULL, 5, NULL);
      break; 

/*
 *  Test 15: Send and receive something
 */
    case T_WIFI_STATION_LOOPBACK:
      WiFi_station_init();
      xTaskCreate(WiFi_tcp_server_task,    "WiFi_tcp_server",      4096, NULL, 5, NULL);
      xTaskCreate(tcpip_accept_poll,       "tcpip_accept_poll",    4096, NULL, 4, NULL);
      WiFi_loopback_test();
      break; 

/*
 *  Test 16: Send and receive something
 */
    case T_WIFI_AP_LOOPBACK:
      WiFi_AP_init();
      xTaskCreate(WiFi_tcp_server_task,    "WiFi_tcp_server",      4096, NULL, 5, NULL);
      xTaskCreate(tcpip_accept_poll,       "tcpip_accept_poll",    4096, NULL, 4, NULL);
      WiFi_loopback_test();
      break; 

/*
 *  Test 17: Cycle the 10MHz clock input
 */
    case T_CYCLE_CLOCK:
      printf("\r\nCycle 10MHz Osc 2:1 duty cycle\r\n");
      while (serial_available(CONSOLE) == 0)
      {
        gpio_set_level(OSC_CONTROL, OSC_ON);       // Turn off the oscillator
        vTaskDelay(ONE_SECOND/2);                  // The oscillator should be on for 1/2 second
        gpio_set_level(OSC_CONTROL, OSC_OFF);      // Turn off the oscillator
        vTaskDelay(ONE_SECOND/4);                  // The oscillator shold be off for 1/4 seocnd
      }
      break; 
/*
 *  Test 18: Turn the RUN lines on and off
 */
    case T_RUN_ALL:
      printf("\r\nCycle RUN lines at 2:1 duty cycle\r\n");
      while (serial_available(CONSOLE) == 0)
      {
        gpio_set_level(STOP_N, 1);                  // Let the clock go
        gpio_set_level(CLOCK_START, 0);   
        gpio_set_level(CLOCK_START, 1);   
        gpio_set_level(CLOCK_START, 0);             // Strobe the RUN linwes
        vTaskDelay(ONE_SECOND/2);                   // The RUN lines should be on for 1/2 second
        gpio_set_level(STOP_N, 0);                  // Stop the clock
        vTaskDelay(ONE_SECOND/4);                   // THe RUN lines shold be off for 1/4 second
      }
      break; 

/*
 *  Rest 19-22 - Single PCNT test
 */
    case T_PCNT_STOP:
    case T_PCNT_SHORT:
    case T_PCNT_FREE:
    case T_PCNT_CLEAR:
      pcnt_test(test - (T_PCNT_STOP - 1));
      break;
  }

 /* 
  *  All done, return;
  */
    run_state &= ~IN_TEST;              // Exit the test 
    freeETarget_timer_start();          // Start interrupts
    return;
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
  char str[64];
  sprintf(str, "\r\nfreETarget %s\r\n", SOFTWARE_VERSION);
  serial_to_all(str, ALL);
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
 * @return: None
 * 
 *----------------------------------------------------------------
 *
 *  Trigger the counters from inside the circuit board and 
 *  read back the results and look for an expected value.
 *  
 *  Return TRUE if the complete circuit is working
 *  
 *  Test 1, Make sure the 10MHz clock is running
 *  Test 2, Clear the flip flops and make sure the run latches are clear
 *  Test 3, Trigger the flip flops and make sure the run latche are set
 *  
 *--------------------------------------------------------------*/
bool POST_counters(void)
{
  unsigned int i;                          // Iteration counter
  bool         test1, test2, test3, test4; // Record if the test failed
  unsigned int count, toggle;              // Cycle counter

  DLT(DLT_CRITICAL);  
  printf("POST_counters()");

  set_status_LED("   ");                  // Turn them all off
  
/*
 *  Test 1, Make sure we can turn off the reference clock
 */
  test1 = true;                           // Start of assuming it passes
  count = 0;
  DLT(DLT_CRITICAL); 
  printf("Turn Clock OFF");
  gpio_set_level(OSC_CONTROL, OSC_OFF);   // Turn off the oscillator
  set_status_LED("W--");
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
    set_status_LED("R--");
    test1 = false;
    DLT(DLT_CRITICAL); 
    printf("Reference clock cannot be stopped");
    vTaskDelay(5*ONE_SECOND);
  }
  else
  {
    set_status_LED("Y--");
  }
  vTaskDelay(ONE_SECOND);

/*
 *  Test 2, Make sure we can turn the reference clock on
 */
  test2 = false;
  count = 0;
  DLT(DLT_CRITICAL); 
  printf("Turn Clock ON");
  gpio_set_level(OSC_CONTROL, OSC_ON);
  toggle = gpio_get_level(REF_CLK);
  for  (i=0; i != 1000; i++)               // Try 1000 times
  {
    if ( (gpio_get_level(REF_CLK) ^ toggle) != 0 )  // Look for a change
    {
      count++;
      test2 = true;
      toggle = gpio_get_level(REF_CLK);
    }
  }

  if ( count == 0  )
  {
    set_status_LED("R--");
    DLT(DLT_CRITICAL); printf("Reference clock cannot be started");
    vTaskDelay(5*ONE_SECOND);
  }
  else
  {
    set_status_LED("G--");
  }
  vTaskDelay(ONE_SECOND);

/*
 *  Test 3, Make sure we can turn the triggers off
 */
  test3 = false;
  DLT(DLT_CRITICAL); 
  printf("Sensor trigger test OFF");
  gpio_set_level(STOP_N, 0);        // Clear the latch
  gpio_set_level(STOP_N, 1);        // and reenable it
  set_status_LED("-Y-");
  if ( is_running() == 0  )
  {
    test3 = true;
    set_status_LED("-G-");
  }    
  if ( test3 == false )
  {
      set_status_LED("-R-");
      DLT(DLT_CRITICAL);
      printf("Stuck bit in run latch: ");
      count = is_running();
      for (i=0; i != 8; i++)
      {
        if ( count & 0x80 )
        {
          printf("%s  ", which_one[i]);
        }

        count <<= 1;
      }
      vTaskDelay(5*ONE_SECOND);
  }      
  vTaskDelay(ONE_SECOND);

/*
 * Test 4, trigger the timers
 */
  test4 = false;
  DLT(DLT_CRITICAL); printf("Sensor trigger test ON");
  set_status_LED("--Y");
  gpio_set_level(STOP_N, 0);          // Clear the latch
  gpio_set_level(STOP_N, 1);
  gpio_set_level(CLOCK_START, 1);     // Triger the run latch
  gpio_set_level(CLOCK_START, 0);
  gpio_set_level(CLOCK_START, 1);
  if ( is_running() == 0xFF  )
  {
    set_status_LED("--G");
    test4 = true;
  }
  else
  {
    set_status_LED("--R");
    DLT(DLT_CRITICAL);
    printf("Failed to start clock in run latch: %02X", is_running());
    vTaskDelay(5*ONE_SECOND);
  }
  vTaskDelay(ONE_SECOND);

/*
 * We get here regardless of whether or not the test failed
 */
  return test1 && test2 && test3 && test4;
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
  
  printf(" Latch:");

  for (i=N; i<=W; i++)
  {
    if ( sensor_status & (1<<i) )   printf("%c", nesw[i]);
    else                            printf(".");
  }

  printf("  Face Strike: %d", face_strike);
  
  printf("  Temperature: %4.2f", temperature_C());
  
  printf("  Switch:");
  
  if ( DIP_SW_A == 0 )
  {
    printf("--");
  }
  else
  {
    printf("A1");
  }
  printf(" ");
  if ( DIP_SW_B == 0 )
  {
    printf("--");
  }
  else
  {
    printf("B2");
  }

  if (( sensor_status & 0x0f) == 0x0f)
  {
    printf(" PASS");
    vTaskDelay(ONE_SECOND);                // Wait for click to go away
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
 * DLT_CRItiCAL levels are always printed
 *   
 *--------------------------------------------------------------*/
bool do_dlt
  (
  unsigned int level
  )
{ 
  if ((level & (is_trace | DLT_CRITICAL)) == 0 )
  {
    return false;      // Send out if the trace is higher than the level 
  }

  printf("\r\nI (%d) ", (int)(esp_timer_get_time()/1000) );

  return true;
}

