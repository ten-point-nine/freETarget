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
void init_gpio(void);                                     // Initialize the GPIO ports
void arm_timers(void);                                    // Make the board ready
void clear_running(void);                                 // Clear the run flip flop 
unsigned int is_running(void);                            // Return a bit mask of running sensors 
void set_status_LED(char* new_state);                     // Manage the LEDs
void commit_status_LEDs(unsigned int blink_state);        // Write the LED control to the hardware
unsigned int read_DIP(void);                              // Read the DIP switch register
unsigned int read_counter(unsigned int direction);
void stop_timers(void);                                   // Turn off the counter registers
void read_timers(int* timer_count);                      // Read and return the counter registers
void drive_paper(void);                                   // Turn on the paper motor
void drive_paper_tick(void);                              // Turn the motor off when the time runs out
void aquire(void);                                        // Read the clock registers
// void enable_face_interrupt();                             // Turn on the face strike interrupt
void disable_face_interrupt(void);                        // Turn off the face strike interrupt
void enable_sensor_interrupt();                           // Turn on the sensor interrupt
void disable_sensor_interrupt(void);                      // Turn off the sensor strike interrupt

void digital_test(void);                                  // Execute the digital test
void paper_on_off(bool on, unsigned long duration);       // Turn the motor on or off
int is_paper_on();                                        // Return the current running state
void rapid_green(unsigned int state);                     // Drive the GREEN light
void rapid_red(unsigned int state);                       // Drive the RED light
void stepper_off_toggle(unsigned int state, unsigned long duration); // New state for the Stepper motor output

void status_LED_init(unsigned int gpio_number);           // Initialize the RMT driver 
void status_LED_test(void);                               // Cycle the status LEDs
void paper_test(void);                                    // Advance the motor
void target_test(void);                                   // Monitor the target sensors for a shot
void trigger_timers(void);                                // Trigger a self test 

void multifunction_init(void);                            // Initialize the multifunction switches
void multifunction_switch(void);                          // Handle the actions of the DIP Switch signal
void multifuction_display(void);                          // Display the MFS settings
void multifunction_wait_open(void);                       // Wait for both multifunction switches to be open
void multifunction_display(void);                         // Display the MFS settings as text
/*
 *  Port Definitions
 */
#define RUN_NORTH_LO   GPIO_NUM_5                  // Address port but locations
#define RUN_EAST_LO    GPIO_NUM_6
#define RUN_SOUTH_LO   GPIO_NUM_7
#define RUN_WEST_LO    GPIO_NUM_15
#define RUN_NORTH_HI   GPIO_NUM_16
#define RUN_EAST_HI    GPIO_NUM_9
#define RUN_SOUTH_HI   GPIO_NUM_10
#define RUN_WEST_HI    GPIO_NUM_11

#define BIT_NORTH_HI   0x80
#define BIT_EAST_HI    0x40
#define BIT_SOUTH_HI   0x20
#define BIT_WEST_HI    0x10
#define BIT_NORTH_LO   0x08
#define BIT_EAST_LO    0x04
#define BIT_SOUTH_LO   0x02
#define BIT_WEST_LO    0x01

#define RUN_MASK       0x00ff
#define REF_CLK        GPIO_NUM_8

#define PAPER          GPIO_NUM_12                  // Paper advance drive active high
#define PAPER_ON       1
#define PAPER_OFF      0

#if (BUILD_REV == REV_500)
#define STOP_N          GPIO_NUM_47                 // Stop the RUN flipflops       
#define CLOCK_START     GPIO_NUM_21                 // Trigger a test cycle
#define OSC_CONTROL     GPIO_NUM_48                 // Enable / kill 10MHz Oscillator
#endif 
#if ((BUILD_REV == REV_510) || (BUILD_REV == REV_520))
#define STOP_N          GPIO_NUM_21                 // Stop the RUN flipflops       
#define CLOCK_START     GPIO_NUM_47                 // Trigger a test cycle
#define OSC_CONTROL     GPIO_NUM_48                 // Enable / kill 10MHz Oscillator
#endif 
#define OSC_ON          1                           // Enable the oscillator
#define OSC_OFF         0                           // Tristate the oscillator
#define LDAC            GPIO_NUM_42

#define DIP_0           9
#define RED_OUT         9                  // Rapid fire RED on DIP0

#define DIP_A           38      // V
#define DIP_B           37      // V
#define DIP_C           35      // V
#define DIP_D           36      // V
#define HOLD_C_GPIO GPIO_NUM_36 // Rapid Fire controls when enabled
#define HOLD_D_GPIO GPIO_NUM_35
//#define GREEN_OUT   12                  // Rapid fire GREEN on DIP3

//#define RED_MASK     1                  // Use DIP 0
//#define GREEN_MASK   8                  // Use DIP 3

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD_A(x)    LO10((x))          // Low digit        xxxx2
#define HOLD_B(x)    HI10((x))          // High digit       xxx2x
#define HOLD_C(x)    LO10((x))          // Low digit        xxxx2
#define HOLD_D(x)    HI10((x))          // High digit       xxx2x
#define TAP_A(x)     HLO10((x))         // High Low digit   xx2xx
#define TAP_B(x)     HHI10((x))         // High High digit  x2xxx
#define HOLD_AB(x)   HHH10((x))         // Highest digit    2xxxx

/*
 * DIP Switch Use. 
 */
#define DIP_SW_A        (gpio_get_level(DIP_A) == 0)  // Switch Input A
#define DIP_SW_B        (gpio_get_level(DIP_B) == 0)  // Switch Input B
#define DIP_SW_C        (gpio_get_level(DIP_C) == 0)  // Switch Input C
#define DIP_SW_D        (gpio_get_level(DIP_D) == 0)  // Switch Input D
#define VERBOSE_TRACE   (DIP_D)         // 8 Show the verbose software trace

#define FACE_SENSOR  19



#endif
