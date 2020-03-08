/*
 * Global functions
 */

void init_analog_io(void);      // Setup the analog hardware

/*
 *  Port Definitions
 */

#define NORTH_ANA    1          // North Analog Input
#define EAST_ANA     2          // East Analog Input
#define SOUTH_ANA    3          // South Analog Input
#define WEST_ANA     4          // West Analog Input

#define REF_OUT      7          // Reference Output
#define REF_IN       0          // Reference Input

#define EOF 0xFF
