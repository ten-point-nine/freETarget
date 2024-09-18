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

/*
 * function Prototypes
 */
void freeETarget_timer_init(void);                                         // Initialize the timers
void freeETarget_timer_pause(void);                                        // Stop the timer
void freeETarget_timer_start(void);                                        // Start the timer
int  timer_new(volatile unsigned long* timer_new, unsigned long duration); // Start a new timer
int  timer_delete(volatile unsigned long* long_timer);                     // Remove a timer
void freeETarget_synchronous(void *pvParameters);                          // Synchronou scheduler
void freeETarget_timers(void *pvParameters);                               // Update the free running timers

/*
 *  Definitions
 */
#define timer_delay(duration) vTaskDelay(duration)
#endif
