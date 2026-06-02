
/******************************************************************************
 *
 * @file: IMU.h
 *
 * API interface for the IMU functions
 *
 *****************************************************************************
 *
 * See: https://www.analog.com/en/products/BMI270.html
 *
 *****************************************************************************/
#ifndef _IMU_H_
#define _IMU_H_

/*
 * Data as pulled in from the BMK270.
 * Note, the register order is LSB then MSB
 *
 * IMPORTANT
 *
 * The SPI driver will deliver the bytes in the order they come out of the BMI270.
 * On the other hand, the ESP32 will word align any uint16_t entries in the structure
 * meaning that the alignment between what the program thinks is aligned and how they
 * arrive from the BMI270 will be off by one byte.
 *
 * For example,    typedef struct {uint8_t dummy; uint16_t sample}
 * will have an empty byte inserted between dummy and sample so that sample is word
 * aligned.
 *
 * Any attempt to use sample will result in a mangled value.
 *
 * To get around this issue, an empty byte is added at the beginning of the
 * structure so that dummy is byte aligned, and uint16_t x becomes word aligned
 * and can be accessed in the correct little-big endian that is provided by the
 * BMI270.
 *
 */

extern trace_index_t index_in;  // Pointer to the input side
extern trace_index_t index_out; // Pointer to the output side

/*
 *  Functions
 */
FIFO_raw_frame_t *trace_first(void);                     // Reset the trace pointers
FIFO_raw_frame_t *trace_next(trace_index_t *index);      // Go to the next pointer
FIFO_raw_frame_t *trace_previous(trace_index_t *index);  // Go to the prior pointer
FIFO_raw_frame_t *trace_FIFO_next(trace_index_t *index); // Point to the next input buffer
bool              trace_ready();                         // Is there data in the FIFO to read?
void              IMU_test(void);                        // Test the IMU
void              trace_build(int timestamp);            // Build up the trace
void              trace_build_and_send(int timestamp);   // Build and send a trace
void              trace_send(int oversample);            // Build and send a trace
void IMU_real_time(void);                                // Output the trace in real time

#endif