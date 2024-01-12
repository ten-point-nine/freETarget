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
void aquire(void);                                        // Read the clock registers
// void enable_face_interrupt();                             // Turn on the face strike interrupt
void disable_face_interrupt(void);                        // Turn off the face strike interrupt
void enable_sensor_interrupt();                           // Turn on the sensor interrupt
void disable_sensor_interrupt(void);                      // Turn off the sensor strike interrupt

void digital_test(void);                                  // Execute the digital test
void paper_on_off(bool on);                               // Turn the motor on or off
void rapid_green(unsigned int state);                     // Drive the GREEN light
void rapid_red(unsigned int state);                       // Drive the RED light

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
#define RUN_MASK     0x00ff
#define REF_CLK        GPIO_NUM_8

#define PAPER          GPIO_NUM_12                  // Paper advance drive active high
#define PAPER_ON       1
#define PAPER_OFF      0

#define STOP_N          GPIO_NUM_47                 // Stop the RUN flipflops       
#define CLOCK_START     GPIO_NUM_21                 // Trigger a test cycle
#define OSC_CONTROL       GPIO_NUM_48                 // Enable / kill 10MHz Oscillator
#define OSC_ON          1                           // Enable the oscillator
#define OSC_OFF         0                           // Tristate the oscillator
#define LDAC            GPIO_NUM_42

#define DIP_0           9
#define RED_OUT         9                  // Rapid fire RED on DIP0

#define DIP_A           38      // V
#define DIP_B           37      // V
#define DIP_C           36      // V
#define DIP_D           35      // V

#define GREEN_OUT   12                  // Rapid fire GREEN on DIP3

#define RED_MASK     1                  // Use DIP 0
#define GREEN_MASK   8                  // Use DIP 3

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD1(x)    LO10((x))          // Low digit        xxxx2
#define HOLD2(x)    HI10((x))          // High digit       xxx2x
#define TAP1(x)     HLO10((x))         // High Low digit   xx2xx
#define TAP2(x)     HHI10((x))         // High High digit  x2xxx
#define HOLD12(x)   HHH10((x))         // Highest digit    2xxxx

/*
 * DIP Switch Use. 
 */
#define DIP_SW_A        (gpio_get_level(DIP_A) == 0)  // Switch Input A
#define DIP_SW_B        (gpio_get_level(DIP_B) == 0)  // Switch Input B
#define DIP_SW_C        (gpio_get_level(DIP_C) == 0)  // Switch Input C
#define DIP_SW_D        (gpio_get_level(DIP_C) == 0)  // Switch Input D
#define VERBOSE_TRACE   (DIP_D)         // 8 Show the verbose software trace



#define FACE_SENSOR  19

/*
 *  MFS Uset
 */
#define POWER_TAP     0                   // DIP A/B used to wake up
#define PAPER_FEED    1                   // DIP A/B used as a paper feed
#define LED_ADJUST    2                   // DIP A/B used to set LED brightness
#define PAPER_SHOT    3                   // DIP A/B Advance paper one cycle
#define PC_TEST       4                   // DIP A/B used to trigger fake shot
#define ON_OFF        5                   // DIP A/B used to turn the target ON or OFF
#define MFS_SPARE_6   6
#define MFS_SPARE_7   7
#define MFS_SPARE_8   8
#define TARGET_TYPE   9                   // Sent target type with score

#define NO_ACTION     0                   // DIP usual function
#define RAPID_RED     1                   // Rapid Fire Red Output
#define RAPID_GREEN   2                   // Rapid Fire Green Output

#define J10_1      VCC
#define J10_2       14                    // TX3
#define J10_3       15                    // RX3
#define J10_4       19                    // RX1
#define J10_5       18                    // TX1
#define J10_6      GND

#endif
