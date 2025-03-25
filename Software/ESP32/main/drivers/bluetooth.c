/*-------------------------------------------------------
 *
 * BlueTooth.c
 *
 * General purpose Blootooth helper
 *
 *-------------------------------------------------------
 *
 * See https://components101.com/sites/default/files/component_datasheet/HC-05%20Datasheet.pdf
 *
 *------------------------------------------------------*/

#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "esp_timer.h"

#include "freETarget.h"
#include "bluetooth.h"
#include "helpers.h"
#include "compute_hit.h"
#include "diag_tools.h"
#include "serial_io.h"
#include "timer.h"
#include "json.h"
#include "nonvol.h"

/*
 *  Local functions
 */
static void send_AT(void); // Send out the AT commands to the BT module

/*******************************************************************************
 *
 * @function: BlueTooth_configuration
 *
 * @brief:    Program the bluetooth module
 *
 * @return:   None
 *
 *******************************************************************************
 *
 * The factory condition for the bluetooth module is to have
 *
 * baud rate = 9600
 * name = some manufacturer name
 *
 * This function programs the bluetooth module to the desired settings
 *
 * baud rate = 115200
 * name = target_name
 *
 * See https://components101.com/sites/default/files/component_datasheet/HC-05%20Datasheet.pdf
 *
 ******************************************************************************/

void BlueTooth_configuration(void)
{
  /*
   * Abort the test if the AUX port is not available
   */
  if ( json_aux_mode == 0 )
  {
    SEND(SOME, sprintf(_xs, "\r\nAUX port not enabled.  Use {\"AUX_MODE\": 2} to enable");)
    SEND(SOME, sprintf(_xs, _DONE_);)
    return;
  }

  SEND(SOME, sprintf(_xs, "\r\nMake sure that the target BlueTooth is not connected to anything ");)

  /*
   * Set the baud rate to the correct value and program
   */

  serial_bt_config(9600);
  vTaskDelay(TICK_10ms); // Wait for the port to settle
  send_AT();

  serial_bt_config(38400);
  vTaskDelay(TICK_10ms); // Wait for the port to settle
  send_AT();

  serial_bt_config(115200);
  vTaskDelay(TICK_10ms); // Wait for the port to settle
  send_AT();

  /*
   * Mark that the module is ready
   */
  json_aux_mode = BLUETOOTH;
  nonvol_write_i32(NONVOL_AUX_PORT_ENABLE, json_aux_mode); // Remember that BT is enabled

  /*
   *  Programmed
   */
  serial_bt_config(0); // Setup the baud rate to the default
  SEND(SOME, sprintf(_xs, _DONE_);)
  return;
}

/*******************************************************************************
 *
 * @function: send_AT
 *
 * @brief:    Send the AT command sequence for the BT module
 *
 * @return:   None
 *
 *******************************************************************************
 *
 * This function is made to support both the HC-05 and HC-06 modules.
 *
 * The HC-05 requires that the MASTER mode be programmed using th push button switch
 * The HC-06 is always in slave mode and will accept an AT command any time
 * that it is not connected to a master.
 *
 * LOL
 *
 * The two modules have differet AT commands.
 * The HC-05 requires that the AT command be terminated with a CR LF
 * The HC-06 requires that the AT command be terminated with two second delay
 *
 ******************************************************************************/
static void send_AT(void)
{
  char str_c[TINY_TEXT]; // Place to store the name{""}

  target_name(str_c);

#if ( BUILD_HC_05 && BUILD_HC_06 )
#error "Both HC-05 and HC-06 are defined"
#endif

  /*
   * Build for HC-05
   */
#if ( BUILD_HC_05 )
  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "\r\nAT\r\n");)             // Flush out any junk
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+NAME=%s\r\n", str_c);)  // Set in the name
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+UART=115200,1,0\r\n");) // Set in the baud rate, stop, parity
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+PSWD=1090\r\n");)       // Set in the baud rate, stop, parity
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+NAME=%s\r\n", str_c);)  // Set in the name
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port
#endif

/*
 * Build for HC-06
 */
#if ( BUILD_HC_06 )
  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT");)                                           // Flush out any junk
  echo_serial(2 * ONE_SECOND, BLUETOOTH, SOME);                            // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  if ( strlen(str_c) > 12 )
  {
    SEND(SOME, sprintf(_xs, "\r\n%s name too long. Stopping\r\n", str_c);) // Limit the name to 10 characters
    return;
  }
  SEND(ALL, sprintf(_xs, "AT+NAME%s", str_c);)                             // Set in the name
  echo_serial(2 * ONE_SECOND, BLUETOOTH, SOME);                            // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+PN");)                                        // Set to No parity
  echo_serial(2 * ONE_SECOND, BLUETOOTH, SOME);                            // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+PIN1090");)                                   // Set in the baud rate, stop, parity
  echo_serial(2 * ONE_SECOND, BLUETOOTH, SOME);                            // Echo the serial port

  SEND(SOME, sprintf(_xs, "\r\n");)
  SEND(ALL, sprintf(_xs, "AT+BAUD8");)                                     // Set in the baud rate to 115200
  echo_serial(2 * ONE_SECOND, BLUETOOTH, SOME);                            // Echo the serial port
#endif

  return;
}

/*****************************************************************************
 *
 * @function: Bluetooth_start_new_connection
 *
 * @brief:    Prepare a new connection on the BT port
 *
 * @return:   Nothing
 *
 ******************************************************************************
 *
 * A new BT connection has been made.

 *
 *******************************************************************************/
void Bluetooth_start_new_connection(void) // Socket token to use
{
  int i;

  /*
   *  Inform the PC what is going on
   */
  SEND(BLUETOOTH, sprintf(_xs, "{\"%s\":%ld}", _GREETING_, run_time_seconds());)

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    if ( (record[i].session_type & SESSION_VALID) != 0 )
    {
      build_json_score(&record[i], SCORE_BLUETOOTH);
      serial_to_all(_xs, BLUETOOTH);
    }
  }

  /*
   *  All done, return
   */
  return;
}
