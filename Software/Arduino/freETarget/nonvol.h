/*
 * NON Vol Storage
 */

#define NONVOL_INIT           0x0
#define NONVOL_SENSOR_DIA     (NONVOL_INIT +  sizeof(int) + 2)            // Sensor diameter
#define NONVOL_DIP_SWITCH     (NONVOL_SENSOR_DIA + sizeof(double) + 2)    // DIP switch setting
#define NONVOL_PAPER_TIME     (NONVOL_DIP_SWITCH + sizeof(int) + 2)       // Paper advance time
#define NONVOL_TEST_MODE      (NONVOL_PAPER_TIME + sizeof(int) + 2)       // Self stest
#define NONVOL_OFFSET         (NONVOL_TEST_MODE  + sizeof(int) + 2)       // Offset applied to compensate for pellet diameter
