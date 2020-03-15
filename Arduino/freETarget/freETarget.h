
/*----------------------------------------------------------------
 * 
 * freETarget.h
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
/*
 * Compilation Flags
 */
 #define TRACE_PWM      true
 
 // Trace the PWM registers
#define TRACE_COUNTERS true  // Trace the COUNTER values
#define ONE_SHOT       false // Just capture one shot
#define CLOCK_TEST     true  // Just sample the clock inputs
#define SHOW_PLOT      false // Disable the 2D graph
 
#define OSCILLATOR_MHZ     8    // 8000 cycles in 1 ms
#define ONE_SECOND      1000    // 1000 ms delay
#define SHOT_TIME        500    // Wait 500 ms for the shot to end

#define RUNNING_MODE_CALIBRATION 1
#define VERBOSE_TRACE            2

#define N 0
#define E 1
#define S 2
#define W 3
