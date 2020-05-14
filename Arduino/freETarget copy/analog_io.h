/*
 * Global functions
 */

void init_analog_io(void);          // Setup the analog hardware
unsigned int read_reference(void);  // Read the feedback channel
void show_analog(void);             // Display the analog values
void cal_analog(void);              // Calibrate the analog threshold

/*
 *  Port Definitions
 */

#define NORTH_ANA    1          // North Analog Input
#define EAST_ANA     2          // East Analog Input
#define SOUTH_ANA    3          // South Analog Input
#define WEST_ANA     4          // West Analog Input

#define REF_OUT      7          // Reference Output
#define V_REFERENCE  0          // Reference Input

#define TO_VOLTS(x) ( ((x) * 5.0) / 1024.0 )
