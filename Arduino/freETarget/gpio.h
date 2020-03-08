/*
 * Global functions
 */
void init_gpio(void);                         // Initialize the GPIO ports
void arm_counters(void);                      // Make the board ready
bool is_running(void);                        // Return TRUE if a shot has been detected
void set_LED(unsigned int led, bool state);   // Manage the LEDs
unsigned int read_DIP(void);                  // Read the DIP switch register
unsigned int read_counters(unsigned int direction);
void stop_counters(void);                     // Turn off the counter registers

/*
 *  Port Definitions
 */
#define D0          13                     //  Byte Port bit locations
#define D1          14                     //
#define D2          15                     //
#define D3          16                     //
#define D4          17                     //
#define D5          18                     //
#define D6          13                     //
#define D7          14                     //

#define NORTH_HI    15                    // Address port but locations
#define NORTH_LO    16
#define EAST_HI     17
#define EAST_LO     18
#define SOUTH_HI    19                    // Address port but locations
#define SOUTH_LO    20
#define WEST_HI     21
#define WEST_LO     22

#define RUN_NORTH   24
#define RUN_EAST    25
#define RUN_SOUTH   26
#define RUN_WEST    27

#define QUIET       30
#define READ        31
#define CLEAR       32
#define STOP        33

#define DIP_A       34
#define DIP_B       35
#define DIP_C       36
#define DIP_D       37

#define LED_S       38
#define LED_X       39
#define LED_Y       40

#define NORTH     0
#define EAST      1
#define SOUTH     2
#define WEST      3

#define EOF 0xFF
