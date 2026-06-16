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

#include "trace.h"
// #include "helpers.h"
#include "diag_tools.h"
// #include "gpio_types.h"
// #include "json.h"
#include "serial_io.h"
#include "timer.h"
// #include "WiFi.h"
#include "NTP.h"
#include "json.h"

/*
 * variables
 */
static time_count_64_t NTP_base_time;       // Time reference
static time_count_64_t NTP_offset_time;     // Time difference in the slave
time_count_64_t        sync_time_remaining; // How long before we have to synch again

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
  if ( sync_time_remaining == 0 ) // Time ran out?
  {                               // Reset
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
 * The master is the device that is telling everyone
 * to reset thier clocks to thier time.
 *
 * Target                           Trace
 *    |<----------------------------{ASK}
 *    |
 * {MASTER} ------------------------->|
 *                                    |
 *    |<---------------------------{SLAVE}
 *    |
 * {OFFSET}
 *---------------------------------------------------*/
void NTP_master(void)
{
  NTP_base_time = esp_timer_get_time();                                      // Reset the time
  SEND(TARGET, sprintf(_xs, "{\"%s\":%'llu}", _NTP_MASTER_, NTP_base_time);) // Send back the ack
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "{\"%s\":%'llu}", _NTP_MASTER_, NTP_base_time);))
  return;
}

/*-----------------------------------------------------
 *
 * @function: NTP_slave
 *35
 * @brief:    Reset the time base when commanded
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * The slave is the remote device that has been told
 * to reset it's time to line up with the master
 *
 *---------------------------------------------------*/
void NTP_slave(void)
{
  NTP_base_time = esp_timer_get_time();                                     // Reset the time
  run_state |= TIME_VALID;                                                  // The trime is valid
  sync_time_remaining = json_NTP_period;                                    // Reset the watchdog
  SEND(TARGET, sprintf(_xs, "{\"%s\":%'llu}", _NTP_SLAVE_, NTP_base_time);) // Send back the ack
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "{\"%s\":%'llu}", _NTP_SLAVE_, NTP_base_time);))
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
  NTP_offset_time     = (esp_timer_get_time() - NTP_base_time) / 2;
  sync_time_remaining = json_NTP_period; // Reset the watchdog
  run_state |= TIME_VALID;               // Boths sides are happy
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "{\"%s\":%'llu}", _NTP_OFFSET_, NTP_offset_time);))
  return;
}