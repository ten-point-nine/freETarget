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
  char str_c[TINY_TEXT]; // Place to store the name{""}
  char str_x[SHORT_TEXT];

  /*
   * Abort the test if the AUX port is not available
   */
  if ( json_aux_mode == 0 )
  {
    SEND(ALL, sprintf(_xs, "\r\nAUX port not enabled.  Use {\"AUX_MODE\": 2} to enable");)
    SEND(ALL, sprintf(_xs, _DONE_);)
    return;
  }

  /*
   * Make sure the module is ready
   */
  SEND(SOME, sprintf(_xs, "\r\nBluetooth configuration.  Make sure the EN switch is pressed when powering up.");)
  if ( prompt_for_confirm() == false )
  {
    SEND(SOME, sprintf(_xs, "\r\nAborting configuration");)
  }
  SEND(SOME, sprintf(_xs, "\r\n");)

  /*
   * Set the baud rate to the correct value and program
   */
  serial_bt_init_config();                           // Setup the baud rate

  SEND(ALL, sprintf(_xs, "AT\r\n");)                 // Flush out any junk
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(ALL, sprintf(_xs, "\r\nAT\r\n");)             // Flush out any junk
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  target_name(str_c);
  SEND(ALL, sprintf(_xs, "AT+NAME=%s\r\n", str_c);)  // Set in the name
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(ALL, sprintf(_xs, "AT+UART=115200,1,0\r\n");) // Set in the baud rate, stop, parity
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  SEND(ALL, sprintf(_xs, "AT+PSWD=1090\r\n");)       // Set in the baud rate, stop, parity
  echo_serial(ONE_SECOND, BLUETOOTH, SOME);          // Echo the serial port

  /*
   * Mark that the module is ready
   */
  json_aux_mode = BLUETOOTH;
  nonvol_write_i32(NONVOL_AUX_PORT_ENABLE, json_aux_mode); // Remember that BT is enabled

  SEND(SOME, sprintf(_xs, "\r\nRelease switch, cycle power to module.");)

  /*
   *  Programmed
   */
  serial_bt_config(); // Setup the baud rate
  SEND(SOME, sprintf(_xs, _DONE_);)
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
  int i, j;

  /*
   *  Inform the PC what is going on
   */
  SEND(BLUETOOTH, sprintf(_xs, "{\"%s\":%10.6f}", _GREETING_, esp_timer_get_time() / 100000.0 / 60.0);)

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    if ( (record[i].session_type & SESSION_VALID) != 0 )
    {
      send_replay(&record[i], i);
      serial_to_all(_xs, BLUETOOTH);
    }
  }

  /*
   *  All done, return
   */
  return;
}
