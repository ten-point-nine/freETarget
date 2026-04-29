/*----------------------------------------------------------------
 *
 * timer.h
 *
 * Header file for timer functions
 *
 *---------------------------------------------------------------*/
#ifndef _TIMER_H_
#define _TIMER_H_

/*
 * Variables
 */

/*
 * function Prototypes
 */
int          ft_timer_new(time_count_t *timer_new, long duration, void *(callback)(), char *name); // Start a new timer in ms
int          ft_timer_delete(time_count_t *timer);                                                 // Stop a running timer
void         trace_synchronous(void *pvParameters);                                                // Synchronou scheduler
void         trace_timers(void *pvParameters);                                                     // Update the free running timers
void         show_time(void);                                                                      // Show the current time
time_count_t run_time_seconds(void);                                                               // Show how long we have been running for
time_count_t run_time_ms(void);    // Show how long we have been running for in ms
void         reset_run_time(void); // Reset the clock back to zero

/*
 *  Definitions
 */

#endif
