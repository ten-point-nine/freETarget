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
void         init_gpio(void);                              // Initialize the GPIO ports

void digital_test(void);                                   // Execute the digital test
void DCmotor_on_off(bool on, time_count_t duration);       // Turn the motor on or off
int  is_paper_on();                                        // Return the current running state
void rapid_D_LED(unsigned int state);                      // Drive the GREEN light
void rapid_C_LED(unsigned int state);                      // Drive the RED light
void rapid_LED_test(void);
void stepper_pulse(void);                                  // New state for the Stepper motor output
void status_LED_init(unsigned int gpio_number);            // Initialize the RMT driver
void status_LED_test(void);                                // Cycle the status LEDs
void paper_test(void);                                     // Advance the motor
void target_test(void);                                    // Monitor the target sensors for a shot
void LED_test(void);                                       // Cycle the target LED
void trigger_timers(void);                                 // Trigger a self test
void timer_run_all(void);                                  // Run all fo the timers at once
void timer_cycle_oscillator(void);                         // Turn the oscillator on and off


/*
 *  Global Variables
 */
extern volatile unsigned int step_count; // Number of steps before stopping

/*
 *  Port Definitions
 */
#define RUN_NORTH_LO GPIO_NUM_5 // Address port but locations
#define RUN_EAST_LO  GPIO_NUM_6
#define RUN_SOUTH_LO GPIO_NUM_7
#define RUN_WEST_LO  GPIO_NUM_15
#define RUN_NORTH_HI GPIO_NUM_16
#define RUN_EAST_HI  GPIO_NUM_9
#define RUN_SOUTH_HI GPIO_NUM_10
#define RUN_WEST_HI  GPIO_NUM_11

#define BIT_NORTH_HI 0x80
#define BIT_EAST_HI  0x40
#define BIT_SOUTH_HI 0x20
#define BIT_WEST_HI  0x10
#define BIT_NORTH_LO 0x08
#define BIT_EAST_LO  0x04
#define BIT_SOUTH_LO 0x02
#define BIT_WEST_LO  0x01

#define RUN_MASK (BIT_NORTH_LO | BIT_EAST_LO | BIT_SOUTH_LO | BIT_WEST_LO) // Include pcnt_lo bits and exclude pcnt_hi bits
#define REF_CLK  GPIO_NUM_8

#define PAPER     GPIO_NUM_12                                              // Paper advance drive active high
#define PAPER_ON  1
#define PAPER_OFF 0


#define OSC_ON            1                                                // Enable the oscillator
#define OSC_OFF           0                                                // Tristate the oscillator
#define RUN_OFF           0                                                // Clear the run flip flops
#define RUN_GO            1                                                // Let the flip flops go
#define CLOCK_TRIGGER_OFF 0                                                // The clock can be triggered by 0-1
#define CLOCK_TRIGGER_ON  1                                                // The clock can be triggered by 0-1
#define LDAC              GPIO_NUM_41                                      // No longer used

#define DIP_0   9
#define RED_OUT 9                                                          // Rapid fire RED on DIP0


#define FACE_SENSOR 19

/*
 *  Driver Settings
 */
#define STEP_ON      1 // Pulse stepper on
#define STEP_OFF     0 // Pulse setpper off
#define STEP_ENABLE  0 // Enable the stepper circuit
#define STEP_DISABLE 1 // Disable stepper circuit

#endif
