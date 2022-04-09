/*
 * Global functions
 */
void init_gpio(void);                                     // Initialize the GPIO ports
void arm_counters(void);                                  // Make the board ready
unsigned int is_running(void);                            // Return a bit mask of running sensors 
void set_LED(int state_RDY, int state_X, int state_y);    // Manage the LEDs
unsigned int read_DIP(void);                              // Read the DIP switch register
unsigned int read_counter(unsigned int direction);
void stop_counters(void);                                 // Turn off the counter registers
void trip_counters(void);
bool read_in(unsigned int port);                          // Read the selected port
void read_timers(void);                                   // Read and return the counter registers
void drive_paper(void);                                   // Turn on the paper motor
void enable_interrupt(unsigned int active);               // Turn on the face strike interrupt if active
void disable_interrupt(void);                             // Turn off the face strike interrupt
void multifunction_init(void);                            // Initialize the multifunction switches
void multifunction_switch(void);                          // Handle the actions of the DIP Switch signal
void multifuction_display(void);                          // Display the MFS settings
void output_to_all(char* s);                              // Multipurpose driver
void char_to_all(char ch);                                // Output a single character
void digital_test(void);                                  // Execute the digital test


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

#define RUN_NORTH   25
#define RUN_EAST    26
#define RUN_SOUTH   27
#define RUN_WEST    28

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
 * DIP Switch Use. 
 */
//                      From DIP                   From Software
#define CALIBRATE       ((digitalRead(DIP_3) == 0)    + 0)   // 1 Go to Calibration Mode
#define DIP_SW_A        ((digitalRead(DIP_2) == 0)    + 0)   // 2 When CALIBRATE is asserted, use lower trip point
#define CAL_LOW         (DIP_SW_A)
#define DIP_SW_B        ((digitalRead(DIP_1) == 0)    + 0)   // 4 When CALIBRATE is asserted, use higher trip point
#define CAL_HIGH        (DIP_SW_B)
#define VERBOSE_TRACE   ((digitalRead(DIP_0) == 0)    + 0)   // 8 Show the verbose software trace

#define V_SET_PWM     8          // VREF setting
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
#define POWER_TAP     0                   // DIP a/B used to wake up
#define PAPER_FEED    1                   // DIP A/B used as a paper feed
#define MFS_SPARE_2   2                   // DIP A/B 
#define MFS_SPARE_3   3                   // DIP A/B 
#define PC_TEST       4                   // DIP A/B used to trigger fake shot
#define ON_OFF        5                   // DIP A/B used to turn the target ON or OFF
#define TABATA_ON_OFF 6                   // Both Dip Switches pressed, manage Tabata


#define J10_1      VCC
#define J10_2       14                    // TX3
#define J10_3       15                    // RX3
#define J10_4       19                    // RX1
#define J10_5       18                    // TX1
#define J10_6      GND



#define EOF 0xFF
