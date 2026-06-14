/*-------------------------------------------------------
 *
 * file: NTP.c
 *
 * Network Time Protocol
 *
 *-------------------------------------------------------
 *
 * Simple NTP for Target / Trace time syncronization
 *
 * ----------------------------------------------------*/

#include "stdbool.h"
#include "esp_timer.h"
#include "driver\timer.h"

#include "freETarget.h"
// #include "trace.h"
//  #include "helpers.h"
#include "diag_tools.h"
// #include "gpio_types.h"
// #include "json.h"
#include "serial_io.h"
#include "timer.h"
// #include "WiFi.h"
#include "NTP.h"

/*
 * variables
 */
static time_count_t NTP_base_time;       // Time reference
static time_count_t NTP_offset_time;     // Time difference in the slave
time_count_t        sync_time_remaining; // How long before we have to synch again

/*-----------------------------------------------------
 *
 * @function: NTP_ttg
 *
 * @brief:    Verify time to go before next sync
 *
 * @return:   TRUE if it is time to sync
 *
 *-----------------------------------------------------
 *
 * The sync timer is a timer started in trace.c and
 * monitored here.
 *
 *---------------------------------------------------*/
bool NTP_ttg(void)
{
  static time_count_t sync_time_remaining = 0;

  if ( sync_time_remaining == 0 )                                                      // Time ran out?
  {                                                                                    // Reset
    ft_timer_new(&sync_time_remaining, NETWORK_TIME_PERIOD, NULL, "Synchronize time"); // Start the synch timer
    return true;
  }
  return false;
}

/*-----------------------------------------------------
 *
 * @function: NTP_master
 *
 * @brief:    Command the slave to reset
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * Reset the time base and command the  slave to
 * resynchronize
 *
 *---------------------------------------------------*/
void NTP_master(void)
{
  NTP_base_time = esp_timer_get_time();                // Reset the time
  SEND(TCPIP, sprintf(_xs, "{\"%s\"}", _NTP_MASTER_);) // Send back the ack
  sync_time_remaining = NETWORK_TIME_PERIOD;           // Reset the watchdog
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "{\"%s\"}", _NTP_MASTER_);))

  return;
}

/*-----------------------------------------------------
 *
 * @function: NTP_slave
 *
 * @brief:    Reset the time base when commanded
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * A command to reset the network time has arrived
 *
 * Reset the time and reply to the master
 *
 *---------------------------------------------------*/
void NTP_slave(void)
{
  NTP_base_time = esp_timer_get_time();               // Reset the time
  SEND(TCPIP, sprintf(_xs, "{\"%s\"}", _NTP_SLAVE_);) // Send back the ack
  run_state |= TIME_VALID;                            // The trime is valid
  sync_time_remaining = NETWORK_TIME_PERIOD;          // Reset the watchdog
  return;
}

/*-----------------------------------------------------
 *
 * @function: NTP_offset
 *
 * @brief:    Compute the loop time and offset
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * The loop time is the current time - base time.
 *
 * We assume the loop is symetric, so the slave
 * is 1/2 the time differece behind us
 *
 *---------------------------------------------------*/
void NTP_offset(void)
{
  NTP_offset_time = (esp_timer_get_time() - NTP_base_time) / 2;
  return;
}