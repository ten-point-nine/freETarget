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
#include "BMI270.h"
#include "spi.h"
#include "nonvol.h"
#include "IMU.h"
#include "timer.h"

/*
 * Definitions
 */
#define APPROACH       7 // Go back in time 7 seconds
#define FOLLOW_THROUGH 2 // Go forwards 2 seconds
#define OVERSAMPLE     8 // Only send 1/8 samples
/*
 *  Typedefs
 */
trace_index_t       index_in  = {0, 0}; // Pointer to the input side
trace_index_t       index_out = {0, 0}; // Pointer to the output side
extern time_count_t last_FIFO_read;

/*
 *  Local Functions
 */
extern FIFO_raw_t sample_raw_read[SAMPLE_BUFFER_COUNT]; // Space for 10 seconds of data

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
  SEND(ALL, sprintf(_xs, _DONE_);)
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
    BMI270_convert_to_g(&sample_raw_read[working.outer].f[working.inner], &current);
    current.rho   = previous.rho + (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta + (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi + (current.phi_dot / SAMPLE_RATE);

    if ( (i % 50) == 0 )
    {
      printf("phi %f %f  theta  %f %f   xy: %f %f working: %d %d\r\n", current.phi, current.phi_dot, current.theta, current.theta_dot,
             current.x, current.y, working.outer, working.inner);
    }
    i++;

    current.x = sin(current.phi) * json_distance_to_target * 1000.0;
    current.y = sin(current.theta) * json_distance_to_target * 1000.0;

    previous = current;

    if ( trace_next(&working) == NULL ) // NULL = End of the FIFO buffer
    {
      SEND(ALL, sprintf(_xs, "X: %6.2f Y: %6.2f\r\n", current.x, current.y);)
      vTaskDelay(ONE_SECOND / 2);
      if ( check_for_exit() == '!' )
      {
        break;
      }
    }
  }

  /*
   *  All done, return
   */
  SEND(ALL, sprintf(_xs, "\r\n%s", _DONE_);)
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
trace_point_t approach[APPROACH * SAMPLE_RATE];             // Samples back in time
trace_point_t follow_through[FOLLOW_THROUGH * SAMPLE_RATE]; // Samples forward in time

void trace_build_and_send(int timestamp)                    // Build and send a trace
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
  int            i;             // Index

  run_state |= IN_REDUCTION;    // Stop collecting FIFO data

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "trace_build(%d)", timestamp);))

                                /*
                                 *  Starting points
                                 */
  BMI270_find_index_out((time_count_t)timestamp);
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

  for ( i = 0; i < FOLLOW_THROUGH * SAMPLE_RATE; i++ )
  {
    BMI270_convert_to_g(&sample_raw_read[working.outer].f[working.inner], &current);
    current.rho   = previous.rho + (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta + (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi + (current.phi_dot / SAMPLE_RATE);
    //   printf("phi %f   theta  %f", current.phi, current.theta);
    follow_through[i].x = sin(current.phi) * json_distance_to_target * 1000.0;
    follow_through[i].y = sin(current.theta) * json_distance_to_target * 1000.0;

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

  for ( i = (APPROACH * SAMPLE_RATE) - 1; i >= 0; i-- )
  {
    BMI270_convert_to_g(&sample_raw_read[working.outer].f[working.inner], &current);
    current.rho   = previous.rho - (current.rho_dot / SAMPLE_RATE);
    current.theta = previous.theta - (current.theta_dot / SAMPLE_RATE);
    current.phi   = previous.phi - (current.phi_dot / SAMPLE_RATE);
    //    printf("phi %f   theta  %f", current.phi, current.theta);
    approach[i].x = sin(current.phi) * json_distance_to_target * 1000.0;
    approach[i].y = sin(current.theta) * json_distance_to_target * 1000.0;

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
 * Format the previously built trace and send it to the host
 *
 *---------------------------------------------------------------*/
void trace_send(int oversample) // Build and send a trace
{
  int i;                        // Index

  run_state |= IN_REDUCTION;    // Stop collecting FIFO data

  /*
   *  Now send the trace
   */
  SEND(ALL, sprintf(_xs, "{\"TRACE\":");)

  for ( i = 0; i < (APPROACH * SAMPLE_RATE) - 1; i += oversample )
  {
    SEND(ALL, sprintf(_xs, "(%4.2f, %4.2f), ", approach[i].x, approach[i].y);)
  }

  for ( i = 0; i < (FOLLOW_THROUGH * SAMPLE_RATE) - 1; i += oversample )
  {
    SEND(ALL, sprintf(_xs, "(%4.2f, %4.2f), ", follow_through[i].x, follow_through[i].y);)
  }

  SEND(ALL, sprintf(_xs, "(0, 0) }");)

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
static bool FIFO_full = false;  // Is the FIFO full

FIFO_raw_frame_t *trace_first() // Find the oldest sample in the FIFO buffer
{
  index_out = index_in;         // Start at oldest sample in the FIFO buffer
  return &sample_raw_read[index_out.outer].f[index_out.inner];
}

FIFO_raw_frame_t *trace_next(trace_index_t *index)
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

  return &sample_raw_read[index->outer].f[index->inner];
}

FIFO_raw_frame_t *trace_previous(trace_index_t *index)
{
  index->outer--;         // Go backwards
  if ( index->outer < 0 ) // Wrap around
  {
    index->outer = RAW_FRAME_COUNT - 1;
    index->inner--;
    if ( index->inner < 0 )
    {
      index->inner = SAMPLE_BUFFER_COUNT - 1;
    }
  }

  return &sample_raw_read[index->outer].f[index->inner];
}

FIFO_raw_frame_t *trace_FIFO_next(trace_index_t *index)
{
  index->outer = ((index->outer) + 1) % SAMPLE_BUFFER_COUNT;
  if ( index_out.outer == index_in.outer ) // Wrapped around, the buffer is full
  {
    FIFO_full = true;                      // Remember we have wrapped around and the FIFO is full
  }
  return &sample_raw_read[index->outer].f[0];
}

bool trace_ready(void)
{
  return FIFO_full; // FIFO is full, data is ready to read
}
