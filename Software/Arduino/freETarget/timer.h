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
unsigned int timer_new(unsigned long* new_timer, unsigned long start); // Start a new timer
unsigned int timer_delete(unsigned long* old_timer);            // Remove a timer

/*
 * Timers
 */
extern volatile unsigned long  keep_alive;                      // Keep alive timer
extern volatile unsigned long  tabata_rapid_timer;              // Tabita or Rapid fire timer
extern volatile unsigned long  in_shot_timer;                   // Time inside of the shot window
extern volatile unsigned long  power_save;                      // Power save timer
extern volatile unsigned long  token_tick;                      // Token ring watchdog

/*
 *  Definitions
 */

#endif
