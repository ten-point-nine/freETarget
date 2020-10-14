
#ifndef _NONVOL_H
#define _NONVOL_H

void init_nonvol(void);         // Initialize nonvol memory
void read_nonvol(void);         // Read in the locations
void get_position(void);        // Reset the position values


/*
 * NON Vol Storage
 */

#define NONVOL_INIT           0x0
#define NONVOL_SENSOR_DIA     (NONVOL_INIT +  sizeof(int) + 2)             // Sensor diameter
#define NONVOL_DIP_SWITCH     (NONVOL_SENSOR_DIA  + sizeof(double) + 2)    // DIP switch setting
#define NONVOL_PAPER_TIME     (NONVOL_DIP_SWITCH  + sizeof(int) + 2)       // Paper advance time
#define NONVOL_TEST_MODE      (NONVOL_PAPER_TIME  + sizeof(int) + 2)       // Self stest
#define NONVOL_CALIBRE_X10    (NONVOL_TEST_MODE   + sizeof(int) + 2)       // Pellet Calibre
#define NONVOL_SENSOR_ANGLE   (NONVOL_CALIBRE_X10 + sizeof(int) + 2)       // Angular displacement of sensors
#define NONVOL_NORTH_X        (NONVOL_SENSOR_ANGLE + sizeof(int) + 2)      // Offset applied to North sensor
#define NONVOL_NORTH_Y        (NONVOL_NORTH_X     + sizeof(int) + 2)
#define NONVOL_EAST_X         (NONVOL_NORTH_Y     + sizeof(int) + 2)       // Offset applied to East sensor
#define NONVOL_EAST_Y         (NONVOL_EAST_X      + sizeof(int) + 2)
#define NONVOL_SOUTH_X        (NONVOL_EAST_Y      + sizeof(int) + 2)       // Offset applied to South sensor
#define NONVOL_SOUTH_Y        (NONVOL_SOUTH_X     + sizeof(int) + 2)
#define NONVOL_WEST_X         (NONVOL_SOUTH_Y     + sizeof(int) + 2)       // Offset applied to West sensor
#define NONVOL_WEST_Y         (NONVOL_WEST_X      + sizeof(int) + 2)

#endif
