/*----------------------------------------------------------------
 *
 * gpio.h
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _GPIO_H_
#define _GPIO_H_

/*
 * Global functions
 */
void init_gpio(void);                     // Initialize the GPIO ports

void digital_test(void);                  // Execute the digital test
void set_status_LED(unsigned int status); // Set the status LED
void status_LED_timer(void);              // Timer to drive the status LED
void timer_run_all(void);                 // Run all fo the timers at once
void timer_cycle_oscillator(void);        // Turn the oscillator on and off

/*
 *  Global Variables
 */
extern volatile unsigned int step_count; // Number of steps before stopping

/*
 *  Port Definitions
 */
#define SWITCH_GPIO GPIO_NUM_6 // Control switch input
#define STATUS_LED  GPIO_NUM_2 // Status LED

#endif
