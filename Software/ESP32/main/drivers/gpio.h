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
void         arm_timers(void);                             // Make the board ready
void         clear_running(void);                          // Clear the run flip flop
unsigned int is_running(void);                             // Return a bit mask of running sensors
void         set_status_LED(char new_state[]);             // Manage the LEDs
void         commit_status_LEDs(unsigned int blink_state); // Write the LED control to the hardware
void         toggle_status_LEDs(void);                     // Toggle the status LEDs on every pass
unsigned int read_DIP(void);                               // Read the DIP switch register
unsigned int read_counter(unsigned int direction);
void         stop_timers(void);                            // Turn off the counter registers
void         read_timers(int *timer_count);                // Read and return the counter registers
void         paper_start(void);                            // Turn on the witness paper
void         paper_stop(void);                             // Turn off the paper drive if
                                                           // it is running
void paper_drive_tick(void);                               // Turn the motor off when the time runs out
void paper_stop(void);                                     // Stop the paper transport
void aquire(void);                                         // Read the clock registers
void enable_face_interrupt();                              // Turn on the face strike interrupt
void disable_face_interrupt(void);                         // Turn off the face strike interrupt

void digital_test(void);                                   // Execute the digital test
void DCmotor_on_off(bool on, unsigned long duration);      // Turn the motor on or off
int  is_paper_on();                                        // Return the current running state
void rapid_green(unsigned int state);                      // Drive the GREEN light
void rapid_red(unsigned int state);                        // Drive the RED light
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

void multifunction_switch(void);                           // Handle the actions of the DIP Switch signal
void multifuction_display(void);                           // Display the MFS settings
void multifunction_wait_open(void);                        // Wait for both multifunction switches to be open
void multifunction_display(void);                          // Display the MFS settings as text

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

#if ( BUILD_REV == REV_500 )
#define STOP_N      GPIO_NUM_47                                            // Stop the RUN flipflops
#define CLOCK_START GPIO_NUM_21                                            // Trigger a test cycle
#define OSC_CONTROL GPIO_NUM_48                                            // Enable / kill 10MHz Oscillator
#endif
#if ( (BUILD_REV == REV_510) || (BUILD_REV == REV_520) )
#define STOP_N      GPIO_NUM_21                                            // Stop the RUN flipflops
#define CLOCK_START GPIO_NUM_47                                            // Trigger a test cycle
#define OSC_CONTROL GPIO_NUM_48                                            // Enable / kill 10MHz Oscillator
#endif
#define OSC_ON            1                                                // Enable the oscillator
#define OSC_OFF           0                                                // Tristate the oscillator
#define RUN_OFF           0                                                // Clear the run flip flops
#define RUN_GO            1                                                // Let the flip flops go
#define CLOCK_TRIGGER_OFF 0                                                // The clock can be triggered by 0-1
#define CLOCK_TRIGGER_ON  1                                                // The clock can be triggered by 0-1
#define LDAC              GPIO_NUM_42                                      // No longer used

#define DIP_0   9
#define RED_OUT 9                                                          // Rapid fire RED on DIP0

#define DIP_A       GPIO_NUM_38                                            // V
#define DIP_B       GPIO_NUM_37                                            // V
#define DIP_C       36                                                     // V
#define DIP_D       35                                                     // V
#define HOLD_C_GPIO GPIO_NUM_36                                            // Rapid Fire controls when enabled
#define HOLD_D_GPIO GPIO_NUM_35

/*
 * DIP Switch Use.
 */
#define DIP_SW_A (gpio_get_level(DIP_A) == 0) // Switch Input A
#define DIP_SW_B (gpio_get_level(DIP_B) == 0) // Switch Input B
#define DIP_SW_C (gpio_get_level(DIP_C) == 0) // Switch Input C
#define DIP_SW_D (gpio_get_level(DIP_D) == 0) // Switch Input D

#define FACE_SENSOR 19

/*
 *  Driver Settings
 */
#define STEP_ON      1 // Pulse stepper on
#define STEP_OFF     0 // Pulse setpper off
#define STEP_ENABLE  0 // Enable the stepper circuit
#define STEP_DISABLE 1 // Disable stepper circuit

#endif
