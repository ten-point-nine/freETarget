/*----------------------------------------------------------------
 *
 * gpio.h
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _TIMER_H_
#define _TIMER_H_

/*
 * Variables
 */
extern time_count_t ring_timer; // Let the ring on the backstop end
extern time_count_t time_to_go; // Time remaining in event in seconds

/*
 * function Prototypes
 */
void              freeETarget_timer_init(void);                                  // Initialize the timers
void              freeETarget_timer_pause(void);                                 // Stop the timer
void              freeETarget_timer_start(void);                                 // Start the timer
int               ft_timer_new(time_count_t *timer_new, unsigned long duration); // Start a new timer
int               ft_timer_delete(time_count_t *timer);                          // Stop a running timer
void              freeETarget_synchronous(void *pvParameters);                   // Synchronou scheduler
void              freeETarget_timers(void *pvParameters);                        // Update the free running timers
void              show_time(void);                                               // Show the current time
long unsigned int run_time_seconds(void);                                        // Show how long we have been running for
long unsigned int run_time_ms(void);                                             // Show how long we have been running for in ms
void              reset_run_time(void);                                          // Reset the clock back to zero

/*
 *  Definitions
 */
#define timer_delay(duration) vTaskDelay(duration)
#endif
