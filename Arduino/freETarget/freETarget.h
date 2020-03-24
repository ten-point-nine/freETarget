
/*----------------------------------------------------------------
 * 
 * freETarget.h
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
#ifndef _FREETARGET_H
#define _FREETARGET_H

/*
 * Compilation Flags
 */
 #define TRACE_PWM      true
 
 // Trace the PWM registers
#define SAMPLE_CALCULATIONS true     // Trace the COUNTER values

 
#define OSCILLATOR_MHZ     8    // 8000 cycles in 1 ms
#define ONE_SECOND      1000    // 1000 ms delay
#define SHOT_TIME        500    // Wait 500 ms for the shot to end

#define RUNNING_MODE_CALIBRATION 1
#define VERBOSE_TRACE            2


#define N 0
#define E 1
#define S 2
#define W 3

struct history
{
  unsigned int shot;    // Current shot number
  double       x;       // X location of shot
  double       y;       // Y location of shot
};

typedef struct history history_t;

#endif

