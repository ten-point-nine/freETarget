/******************************************************************************
 *
 * file: IMU.c
 *
 * AInertial Measurement Unit (IMU) driver
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

#include "trace.h"
#include "json.h"
#include "diag_tools.h"
#include "gpio.h"
#include "helpers.h"
#include "BMI270.h"
#include "spi.h"
#include "nonvol.h"
#include "IMU.h"

/*
 * Definitions
 */
#define SQ(x) ((x) * (x))

/*
 *  Typedefs
 */

trace_index_t index_in  = {0, 0}; // Pointer to the input side
trace_index_t index_out = {0, 0}; // Pointer to the output side

/*
 *  Local Functions
 */
extern FIFO_raw_t sample_raw_read[SAMPLE_BUFFER_COUNT]; // Space for 10 seconds of data

/*----------------------------------------------------------------
 *
 * @function: IMU_test()
 *
 * @brief:    Test the IMU
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * Use the FIFO data to calculate a path
 *
 *--------------------------------------------------------------*/
#define TIME_STEP    (0.010)
#define TIME_STEP_SQ (TIME_STEP * TIME_STEP)
#define G_mm_s2      9806.65

void IMU_test(void)
{
  real_t            vector_magnitude;                                             // Magnitude of the acceleration vector
  FIFO_raw_frame_t *present_raw;                                                  // Present sample
  trace_vector_t    previous;                                                     // Previous sample converted to g
  trace_vector_t    present;                                                      // Present sample converted to g

  present_raw = trace_first();                                                    // Start at the beginning of the FIFO buffer

  while ( present_raw != NULL )
  {
    BMI270_convert_to_g(&present_raw, &present);                                  // Remove the DC components and convert raw data to g

    vector_magnitude =
        sqrt(SQ(present.x_dotdot) + SQ(present.y_dotdot) + SQ(present.z_dotdot)); // Calculate the magnitude of the acceleration vector

    /*
     * Integrate the positition
     */
    present.x = (previous.x) + (previous.x_dot * TIME_STEP) +
                (present.x_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the X position using the acceleration data
    present.y = (previous.y) + (previous.y_dot * TIME_STEP) +
                (present.y_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Y position using the acceleration data
    present.z = (previous.z) + (previous.z_dot * TIME_STEP) +
                (present.z_dotdot * G_mm_s2 * (TIME_STEP_SQ) / 2); // Update the Z position using the acceleration data
    /*
     * Integrate the velocity
     */
    present.x_dot = previous.x_dot + (present.x_dotdot * G_mm_s2) * TIME_STEP;
    present.y_dot = previous.y_dot + (present.y_dotdot * G_mm_s2) * TIME_STEP;
    present.z_dot = previous.y_dot + (present.z_dotdot * G_mm_s2) * TIME_STEP;

    previous = present;

    SEND(ALL, sprintf(_xs,
                      "\r\nx..: %+.3fg, y..: %+.3fg, z..: %+.3fg, |a|: %.3fg  x.: %+.3fmm/s,  "
                      "y.:%+.3fmm/s, z.: %+.3fmm/s   x: %+.3fmm, y: %+.3fmm, z: %+.3fmm,   ",
                      present.x_dotdot, present.y_dotdot, present.z_dotdot, vector_magnitude, present.x_dot, present.y_dot, present.z_dot,
                      present.x, present.y, present.z);)

    present_raw = trace_next(&index_out); // Move to the next sample
  };

  /*
   * All done
   */
  SEND(ALL, sprintf(_xs, _DONE_);)
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
static bool FIFO_full = false;                         // Is the FIFO full

FIFO_raw_frame_t *trace_first(void)
{
  index_out.inner = index_in.inner;                    // Start at the same place
  index_out.outer = index_in.outer;
  trace_next(&index_out);                              // And move over to the 'first' entry
  return &sample_raw_read[index_out.outer].f[index_out.inner];
}

FIFO_raw_frame_t *trace_next(trace_index_t *index)
{
  index->inner = (index->inner + 1) % RAW_FRAME_COUNT; // Go to the next sample in the FIFO buffer

  if ( index->inner != 0 )                             // Now wrapped around, return the pointer
  {
    return &sample_raw_read[index->outer].f[index->inner];
  }
  else                                                 // Wrapped around, move to the next FIFO buffer
  {
    return trace_FIFO_next(index);                     //
  }

  return NULL;                                         // Should never get here
}

FIFO_raw_frame_t *trace_FIFO_next(trace_index_t *index)
{
  index->outer = ((index->outer) + 1) % SAMPLE_BUFFER_COUNT;
  if ( index_out.outer == index_in.outer )             // Wrapped around, the buffer is full
  {
    FIFO_full = true;                                  // Remember we have wrapped around and the FIFO is full
  }
  return &sample_raw_read[index->outer].f[0];
}

bool trace_ready(void)
{
  return FIFO_full; // FIFO is full, data is ready to read
}
