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
 * Function Prototypes
 */
void init_timer(void);                                          // Initialize the timers
void enable_timer_interrupt(void);                              // Enable the timer interrupt
void disable_timer_interrupt(void);                             // Turn of the the timer interrupt
void set_motor_time(unsigned int duration, unsigned int cycles);// Duration in milliseconds

/*
 *  Port Definitions
 */


#endif
