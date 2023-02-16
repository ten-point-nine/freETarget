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
void set_LED(int state_RDY, int state_X, int state_y);    // Manage the LEDs
unsigned int read_DIP(void);                              // Read the DIP switch register
unsigned int read_counter(unsigned int direction);
void stop_timers(void);                                   // Turn off the counter registers
void trip_timers(void);
bool read_in(unsigned int port);                          // Read the selected port
void read_timers(unsigned int* timer_counts);             // Read and return the counter registers
void drive_paper(void);                                   // Turn on the paper motor
void enable_face_interrupt();                             // Turn on the face strike interrupt
void disable_face_interrupt(void);                        // Turn off the face strike interrupt
void enable_sensor_interrupt();                           // Turn on the sensor interrupt
void disable_sensor_interrupt(void);                      // Turn off the sensor strike interrupt
void multifunction_init(void);                            // Initialize the multifunction switches
void multifunction_switch(void);                          // Handle the actions of the DIP Switch signal
void multifuction_display(void);                          // Display the MFS settings
void multifunction_wait_open(void);                       // Wait for both multifunction switches to be open
void output_to_all(char* s);                              // Multipurpose driver
void char_to_all(char ch);                                // Output a single character
void digital_test(void);                                  // Execute the digital test
void paper_on_off(bool on);                               // Turn the motor on or off

/*
 *  Port Definitions
 */
#define D0          37                     //  Byte Port bit locations
#define D1          36                     //
#define D2          35                     //
#define D3          34                     //
#define D4          33                     //
#define D5          32                     //
#define D6          31                     //
#define D7          30                     //

#define NORTH_HI    50                    // Address port but locations
#define NORTH_LO    51
#define EAST_HI     48
#define EAST_LO     49
#define SOUTH_HI    43                    // Address port but locations
#define SOUTH_LO    47
#define WEST_HI     41
#define WEST_LO     42

#define RUN_LSB      3
#define RUN_NORTH   25                    // PA3
#define RUN_N_MASK  (1<<3)
#define RUN_EAST    26                    // PA4
#define RUN_E_MASK  (1<<4)
#define RUN_SOUTH   27                    // PA5
#define RUN_S_MASK  (1<<5)
#define RUN_WEST    28                    // PA6
#define RUN_W_MASK  (1<<6)
#define RUN_A_MASK  (RUN_N_MASK + RUN_E_MASK + RUN_S_MASK + RUN_W_MASK)
#define RUN_PORT    PINA

#define QUIET       29
#define RCLK        40
#define CLR_N       39
#define STOP_N      52       
#define CLOCK_START 53

#define DIP_0        9                    // 
#define DIP_1       10
#define DIP_2       11
#define DIP_3       12
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
//                      From DIP                   From Software
#define CALIBRATE       ((digitalRead(DIP_3) == 0)    + 0)   // 1 Go to Calibration Mode
#define DIP_SW_A        ((digitalRead(DIP_2) == 0)    + 0)   // 2 When CALIBRATE is asserted, use lower trip point
#define CAL_LOW         (DIP_SW_A)
#define DIP_SW_B        ((digitalRead(DIP_1) == 0)    + 0)   // 4 When CALIBRATE is asserted, use higher trip point
#define CAL_HIGH        (DIP_SW_B)
#define VERBOSE_TRACE   ((digitalRead(DIP_0) == 0)    + 0)   // 8 Show the verbose software trace

#define VSET_PWM     8          // VREF setting
#define CTS_U        7
#define RTS_U        6
#define LED_PWM      5          // PWM Port
#define LED_RDY      4
#define LED_X        3
#define LED_Y        2
#define LON          1          // Turn the LED on
#define LOF          0          // Turn the LED off
#define LXX         -1          // Leave the LED alone
#define L(A, B, C)  (A), (B), (C)

#define NORTH        0
#define EAST         1
#define SOUTH        2
#define WEST         3
#define TRIP_NORTH   0x01
#define TRIP_EAST    0x02
#define TRIP_SOUTH   0x04
#define TRIP_WEST    0x08

#define PAPER        18                    // Paper advance drive active low (TX1)
#define PAPER_ON      0
#define PAPER_OFF     1
#define PAPER_ON_300  1
#define PAPER_OFF_300 0

#define FACE_SENSOR  19

#define SPARE_1      22
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

#define J10_1      VCC
#define J10_2       14                    // TX3
#define J10_3       15                    // RX3
#define J10_4       19                    // RX1
#define J10_5       18                    // TX1
#define J10_6      GND



#define EOF 0xFF

#endif
