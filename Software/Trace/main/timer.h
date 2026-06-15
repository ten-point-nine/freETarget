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
bool         ft_timer_new(time_count_t *timer_new, time_count_t duration, void *(callback)(), char *name); // Start a new timer in ms
int          ft_timer_delete(time_count_t *timer);                                                         // Stop a running timer
void         trace_synchronous(void *pvParameters);                                                        // Synchronou scheduler
void         trace_timers(void *pvParameters);                                                             // Update the free running timers
void         show_time(void);                                                                              // Show the current time
time_count_t run_time_us(void);       // Show how long we have been running for in us
void         reset_run_time_us(void); // Reset the clock back to zero

/*
 *  Definitions
 */

#endif
