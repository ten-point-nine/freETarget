/******************************************************************************
 *
 * file: IMU.c
 *
 * Inertial Measurement Unit (IMU) driver
 *
 *****************************************************************************
 *
 *
 *****************************************************************************/
#include "stdio.h"
#include "string.h"
#include "driver\gpio.h"
#include "math.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "nvs_flash.h"
#include "esp_random.h"

#include "trace.h"
#include "json.h"
#include "diag_tools.h"
#include "gpio.h"
#include "helpers.h"
#include "ICM45686.h"
#include "spi.h"
#include "nonvol.h"
#include "IMU.h"
#include "timer.h"

/*
 * Definitions
 */

/*
 *  Typedefs
 */
trace_index_t          index_in  = {0, 0}; // Pointer to the input side
trace_index_t          index_out = {0, 0}; // Pointer to the output side
extern time_count_64_t last_FIFO_read;

/*
 *  Local Functions
 */
extern FIFO_packet_t FIFO_queue[SAMPLE_BUFFER_COUNT]; // Space for 10 seconds of data

/*----------------------------------------------------------------
 *
 * @function: IMU_test
 *
 * @brief:    Fake an outpute
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *
 *---------------------------------------------------------------*/
#define TEST_BACKOFF 2000000                                                                   // Go back 2 seconds
#define TEST_JITTER  500000                                                                    // Add up to 0.5 seconds of jitter

void IMU_test(void)
{
  trace_build(last_FIFO_read - ((TEST_BACKOFF - TEST_JITTER) + (esp_random() % TEST_JITTER))); // Go back 2 seconds
  SEND(CONSOLE, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: IMU_real_time
 *
 * @brief:    Output the trace in real time
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 *
 *---------------------------------------------------------------*/
void IMU_real_time(void)
{
#if ( 0 )
  trace_index_t  working;  // Working data point
  trace_vector_t current;  // Current computed location
  trace_vector_t previous; // Last computed location
  int            i = 0;

  run_state &= ~IN_TEST;   // Stop collecting FIFO data

  /*
   *  Work throught the approch
   */
  previous.x     = 0;
  previous.y     = 0;
  previous.z     = 0;
  previous.rho   = 0;
  previous.theta = 0;
  previous.phi   = 0;
  current        = previous;
  trace_first(); // Start at the oldest sample in the FIFO buffer
  working = index_out;

  while ( 1 )
  {
#if USE_BMI270
    BMI270_convert_to_g(&sample_raw_read[working.outer].f[working.inner], &current);
#endif
#if USE_ICM45686
    ICM45686_convert_to_g(&sample_raw_read[working.outer].f[working.inner], &current);
#endif
    current.rho   = previous.rho + (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta + (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi + (current.phi_dot / SAMPLE_RATE);

    if ( ((i++) % 200) == 0 )
    {
      printf("phi %f %f  theta  %f %f\r\n", current.phi, current.phi_dot, current.theta, current.theta_dot);
    }

    current.x = sin(current.phi) * json_distance_to_target * 1000.0;
    current.y = sin(current.theta) * json_distance_to_target * 1000.0;

    previous = current;

    if ( trace_next(&working) == NULL ) // NULL = End of the FIFO buffer
    {
      SEND(CONSOLE, sprintf(_xs, "X: %6.2f Y: %6.2f\r\n", current.x, current.y);)
      vTaskDelay(ONE_SECOND / 2);
      if ( check_for_exit() == '!' )
      {
        break;
      }
      if ( gpio_get_level(SWITCH_GPIO) == 0 ) // If the switch is pressed, reset
      {
        previous.x     = 0;
        previous.y     = 0;
        previous.z     = 0;
        previous.rho   = 0;
        previous.theta = 0;
        previous.phi   = 0;
      }
    }
  }

  /*
   *  All done, return
   */
  SEND(CONSOLE, sprintf(_xs, "\r\n%s", _DONE_);)
#endif
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_build
 *
 * @brief:  Build the shot trace
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The target has detected a shot and now wants to build a trace
 *
 * This function is called with the timestamp that the target
 * has detected the shot.
 *
 * The basic assumption of the trace is that the gun was perfectly
 * aligned to the target so that the shot location (wherever that
 * may actually be) is the starting point for computing the
 * gun path onto the target.
 *
 *---------------------------------------------------------------*/
trace_point_t approach[APPROACH * TRACE_RATE];             // Samples back in time
trace_point_t follow_through[FOLLOW_THROUGH * TRACE_RATE]; // Samples forward in time

void trace_build_and_send(int timestamp)                   // Build and send a trace
{
  trace_build(timestamp);
  trace_send(OVERSAMPLE);
  return;
}

void trace_build(int timestamp) // Build and send a trace
{
  trace_index_t  working;       // Working data point
  trace_vector_t current;       // Current computed location
  trace_vector_t previous;      // Last computed location
  int            i, j;          // Index

  run_state |= IN_REDUCTION;    // Stop collecting FIFO data

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "trace_build(%d)", timestamp);))

                                /*
                                 *  Starting points
                                 */
#if USE_BMI270
  BMI270_find_index_out((time_count_64_t)timestamp);
#endif
#if USE_ICMM45686
  ICM45686_find_index_out((time_count_64_t)timestamp);
#endif
  working = index_in;
  /*
   *  Work throught the follow through
   */
  previous.x     = 0;
  previous.y     = 0;
  previous.z     = 0;
  previous.rho   = 0;
  previous.theta = 0;
  previous.phi   = 0;

  for ( i = 0, j = 0; i < FOLLOW_THROUGH * SAMPLE_RATE; i++ )
  {
    ICM45686_convert_to_g(&FIFO_queue[working.outer].f[working.inner], &current);
    current.rho   = previous.rho + (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta + (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi + (current.phi_dot / SAMPLE_RATE);
    //   printf("phi %f   theta  %f", current.phi, current.theta);

    if ( (i % (TRACE_RATE)) == 0 )
    {
      follow_through[j].x = sin(current.phi) * json_distance_to_target * 1000.0;
      follow_through[j].y = sin(current.theta) * json_distance_to_target * 1000.0;
      j++;
    }

    previous = current;
    trace_next(&working);
  }

  /*
   *  Work throught the approch
   */
  previous.x     = 0;
  previous.y     = 0;
  previous.z     = 0;
  previous.rho   = 0;
  previous.theta = 0;
  previous.phi   = 0;
  working        = index_in;

  for ( i = (APPROACH * SAMPLE_RATE) - 1, j = (APPROACH * TRACE_RATE) - 1; i >= 0; i-- )
  {
    // ICM45686_convert_to_g(&FIFO_queue[working.outer].f[working.inner], &current);
    current.rho   = previous.rho - (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta - (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi - (current.phi_dot / SAMPLE_RATE);
    //    printf("phi %f   theta  %f", current.phi, current.theta);
    if ( (i % TRACE_RATE) == 0 )
    {
      approach[j].x = sin(current.phi) * json_distance_to_target * 1000.0;
      approach[j].y = sin(current.theta) * json_distance_to_target * 1000.0;
    }
    previous = current;
    trace_previous(&working);
  }

  /*
   *  All done, return
   */
  run_state &= ~IN_REDUCTION;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_send
 *
 * @brief:  Send the shot trace
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * Format the previously built trace and send it to the host.
 *
 * The device uses a crude form of compression.
 *
 * The data is saved as ordered pairs of x and y coordinates.
 *
 * When sending, the data is scaled to fit on the target surface
 * and is sent as deltas between each pair.
 *
 * ex
 *
 * (100, 200)  --> (100, 200)
 * (105, 195)  --> (5, -5)
 *
 * Each delta is sent as ASCII hex, the first field is fixed at 3
 * characters wide and the second is variable length.
 *
 * When decoding, the field length is determined and the first
 * three bytes are assigned to X, the remainder to Y.
 *
 *---------------------------------------------------------------*/
#define TRACE_FORMAT "%03X%X,"             // Format for sending the trace, 3 characters for X and the rest for Y

void trace_send(int oversample)            // Build and send a trace
{
  int    i;                                // Index
  real_t trace_scale;                      // Scale factor for the trace
  real_t last_x, last_y;                   // Last sent coordinates
  bool   start_sending = false;            // Have we started sending the trace yet?
  int    delta_x, delta_y;                 // Deltas to send
  real_t trace_size_squared = SQ(json_trace_size);

  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "trace_send(%d))", oversample);))

  run_state |= IN_REDUCTION;               // Stop collecting FIFO data

  trace_scale = json_trace_size / 32768.0; // Scale the trace to fit on the target

                                           /*
                                            *  Find the first point within the target to send
                                            */
  for ( i = 1; i < (APPROACH * SAMPLE_RATE) - 1; i += oversample )
  {
    if ( (SQ(approach[i].x) + SQ(approach[i].y)) <= trace_size_squared ) // Wait for the first point to be within the target
    {
      start_sending = true;
      break;
    }
  }

  if ( start_sending == false ) // Nothing to send, bail out
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Nothing to send\r\n");))
    return;
  }

  /*
   * Start off sending the approach
   */
  last_x = approach[i].x;
  last_y = approach[i].y;
  SEND(CONSOLE, sprintf(_xs, "{\"TRACE\": %f, [%6.4f, %6.4f], ", trace_scale, last_x, last_y);)

  for ( i = i + 1; i < (APPROACH * SAMPLE_RATE) - 1; i += oversample )
  {
    delta_x = (int)((approach[i].x - last_x) / trace_scale);
    delta_y = (int)((approach[i].y - last_y) / trace_scale);
    SEND(CONSOLE, sprintf(_xs, TRACE_FORMAT, delta_x & 0xfff, delta_y & 0xfff);)
    last_x = approach[i].x;
    last_y = approach[i].y;
  }

  for ( i = 0; i < (FOLLOW_THROUGH * SAMPLE_RATE) - 1; i += oversample )
  {
    if ( (SQ(follow_through[i].x) + SQ(follow_through[i].y)) >= trace_size_squared ) // See if we go out of bounds
    {
      break;                                                                         // Yes, nothing more to send
    }

    delta_x = (int)((follow_through[i].x - last_x) / trace_scale);
    delta_y = (int)((follow_through[i].y - last_y) / trace_scale);
    SEND(CONSOLE, sprintf(_xs, TRACE_FORMAT, delta_x & 0xfff, delta_y & 0xfff);)
    last_x = follow_through[i].x;
    last_y = follow_through[i].y;
  }

  SEND(CONSOLE, sprintf(_xs, "00000}");)
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DONE");))

  /*
   *  All done, return
   */
  run_state &= ~IN_REDUCTION;
  return;
}

/*----------------------------------------------------------------
 *
 * @function: trace_first()
 *            trace_next()
 *            trace_read_next();
 *
 * @brief:    Manage indexes
 *
 * @return:   Pointer to next sample in the FIFO buffer
 *
 *----------------------------------------------------------------
 *
 * The raw input is stored in two queues, inner and outer
 *
 * outer           ^ --> outer          ^ --> outer -->
 *                 |                    |
 *   inner --> inner     inner --> inner
 *
 * outer points to the next available FIFO input buffer
 * inner points to an individual sample in the FIFO input buffer
 *
 *--------------------------------------------------------------*/
static bool FIFO_full = false; // Is the FIFO full

FIFO_raw_t *trace_first()   // Find the oldest sample in the FIFO buffer
{
  index_out = index_in;        // Start at oldest sample in the FIFO buffer
  return &FIFO_queue[index_out.outer].f[index_out.inner];
}

FIFO_raw_t *trace_next(trace_index_t *index)
{
  /*
   *  Move to the next sample in the FIFO buffer
   */
  index->inner = (index->inner + 1) % RAW_FRAME_COUNT; // Go to the next sample in the FIFO buffer

  /*
   *  Check if we have wrapped around to the beginning of the FIFO buffer
   */
  if ( index->inner == 0 )                                   // Wrapped around, move to the next FIFO buffer
  {
    index->outer = (index->outer + 1) % SAMPLE_BUFFER_COUNT; // Move to the next FIFO buffer
  }

  /*
   *  Check if we have caught up to the input point
   */
  if ( (index->outer == index_in.outer) && (index->inner == index_in.inner) ) // Wrapped around to the input point, no more data
  {
    return NULL;                                                              // No more data
  }

  return &FIFO_queue[index->outer].f[index->inner];
}

FIFO_raw_t *trace_previous(trace_index_t *index)
{
  index->outer--;         // Go backwards
  if ( index->outer < 0 ) // Wrap around
  {
    index->outer = SAMPLE_BUFFER_COUNT - 1;
    index->inner--;
    if ( index->inner < 0 )
    {
      index->inner = RAW_FRAME_COUNT - 1;
    }
  }

  return &FIFO_queue[index->outer].f[index->inner];
}

FIFO_raw_t *trace_FIFO_next(trace_index_t *index)
{
  index->outer = ((index->outer) + 1) % SAMPLE_BUFFER_COUNT;
  if ( index_out.outer == index_in.outer ) // Wrapped around, the buffer is full
  {
    FIFO_full = true;                      // Remember we have wrapped around and the FIFO is full
  }
  return &FIFO_queue[index->outer].f[0];
}

bool trace_ready(void)
{
  return FIFO_full; // FIFO is full, data is ready to read
}
