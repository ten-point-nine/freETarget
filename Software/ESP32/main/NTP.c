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
// #include "helpers.h"
#include "diag_tools.h"
// #include "gpio_types.h"
// #include "json.h"
#include "serial_io.h"
#include "timer.h"
// #include "WiFi.h"
#include "NTP.h"
#include "json.h"

#define RUNNING_SAMPLES 5 // Keep five samples

/*
 *  Local functions
 */
void NTP_time_sort(time_count_64_t in_array[], int size);

/*
 * variables
 */
time_count_64_t NTP_start_time;                    // When did we start the time sync
time_count_64_t NTP_base_time;                     // Time to add to esp32_timer_get_time() to equal server time
time_count_64_t NTP_server_time;                   // Time the server synchronilzed
time_count_64_t NTP_offset_time;                   // Route time between client and server
time_count_64_t sync_time_remaining;               // How long before we have to synch again

time_count_64_t NTP_offset_array[RUNNING_SAMPLES]; // Array of offsets
time_count_64_t NTP_base_array[RUNNING_SAMPLES];

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
 * @function: NTP_ask
 *
 * @brief:    Start the NTP process by calling ask
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * This is the first call to the NTP process.
 *
 * It remembers the current time and sends a message
 * to the server asking for the server time.
 *
 * The master is the device that is telling everyone
 * to reset thier clocks to thier time.
 *
 *  NTP_ask
 *  Sensor                          Target
 *    |-----------{CLIENT} ---------->|
 *                                    |
 *    |<----------{SERVER} -----------|
 *    |
 * {OFFSET}
 *
 *---------------------------------------------------*/
void NTP_ask(void)
{
  NTP_start_time = esp_timer_get_time();
  SEND(TCPIP, sprintf(_xs, "{\"%s\"}", _NTP_SERVER_);) // Ask the server for the time
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "NTP_ASK: %s", _NTP_SERVER_);))
  return;
}

/*-----------------------------------------------------
 *
 * @function: NTP_server
 *
 * @brief:    Manage an NTP request to the time server
 *
 * @return:   Nothing
 *
 *---------------------------------------------------
 *
 * This is on the time server side to receive the
 * request and send out the current timer value
 *
 *---------------------------------------------------*/
void NTP_server(void)
{
  NTP_base_time = esp_timer_get_time();                                        // Send the server time
  SEND(TCPIP, sprintf(_xs, "{\"%s\":%lld}\r\n", _NTP_CLIENT_, NTP_base_time);) // back to the client
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "Server time: %lld", NTP_base_time);))
  return;
}

/*-----------------------------------------------------
 *
 * @function: NTP_client
 *
 * @brief:    Message from the client to begin NTP
 *
 * @return:   Nothing
 *
 *-----------------------------------------------------
 *
 * The clent is on the sensor side and needs to be
 * updated with the current server time.
 *
 * It receives the time from the server and uses
 * that number to adjust the time to match up with
 * the server.
 *
 * The client keeps a running list of the last 5
 * samples, sorts them low to high, and then takes
 * the middle three samples (discard outliers)
 * to get an average.
 *
 *---------------------------------------------------*/
void NTP_client(void)
{
  static int sample_count = 0; // Number of time samples we have

  /*
   *  Calculate the new input from the server
   */
  NTP_offset_time     = (esp_timer_get_time() - NTP_base_time) / 2; // The time for the loop
  NTP_base_time       = NTP_server_time - esp_timer_get_time();     // Time between client and server
  sync_time_remaining = json_NTP_period;                            // Reset the watchdog
  DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "NTP Base_time:%lld   NTP offset:%lld   NTP network time:%lld", NTP_base_time, NTP_offset_time,
                                       NTP_time_us());))

  /*
   * Sort the new input against the last samples
   */
  NTP_offset_array[0] = NTP_offset_time;             // Insert the latest
  NTP_base_array[0]   = NTP_base_time;
  NTP_time_sort(&NTP_offset_array, RUNNING_SAMPLES); // Sort the oldest
  NTP_time_sort(&NTP_base_array, RUNNING_SAMPLES);

                                                     /*
                                                      *  When we get to the right number of samples
                                                      */
  sample_count++;
  if ( sample_count >= RUNNING_SAMPLES ) // Collected the right number of samples
  {
    NTP_offset_time = (NTP_offset_array[1] + NTP_offset_array[2] + NTP_offset_array[3]) / 3;
    NTP_base_time   = (NTP_base_array[1] + NTP_base_array[2] + NTP_base_array[3]) / 3;
  }

  /*
   * Finished
   */
  return;
}

/*
 *  Bubble sort the time
 */
void NTP_time_sort(time_count_64_t in_array[], int size)
{
  int             i, j;
  time_count_64_t temp;

  for ( i = size - 1; i != 0; i-- )
  {
    for ( j = 0; j != i; j++ )
    {
      if ( in_array[j] < in_array[j + 1] )
      {
        temp            = in_array[j];
        in_array[j]     = in_array[j + 1];
        in_array[j + 1] = temp;
      }
    }
  }

  /*
   * Sorted, return
   */
  return;
}
/*-----------------------------------------------------
 *
 * @function: NTP_time_us    Time in microseconds
 *            NTP_time_ms    Time in milliseconds
 *            NTP_time_s     Time in seconds
 *
 * @brief:    Time base with correction to serverf
 *
 * @return:   Current time corrected for server
 *
 *-----------------------------------------------------
 *
 * The time functions NTP_client and NTP_server work
 * out the time difference between the client and server
 * and add offsets to the client to agree with the
 * server
 *
 *---------------------------------------------------*/
time_count_64_t NTP_time_us(void)
{
  return esp_timer_get_time() // What time is it here
         + NTP_base_time      // What is the offset to the target
         + NTP_offset_time;   // The for the message to get here);
}

time_count_64_t NTP_time_ms(void)
{
  return (NTP_time_us() / 1000);
}

time_count_64_t NTP_time_s(void)
{
  return (NTP_time_us() / 1000000);
}