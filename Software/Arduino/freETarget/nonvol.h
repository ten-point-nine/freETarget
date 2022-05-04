
#ifndef _NONVOL_H
#define _NONVOL_H

#define PS_VERSION        4                       // Persistent storage version
#define PS_UNINIT(x)     ( ((x) == 0xABAB) || ((x) == 0xFFFF))  // Uninitilized value
 
/*
 * Function prototypes
 */
void factory_nonvol(bool new_serial_number);      // Factory reset nonvol
void init_nonvol(int v);                          // Reset to defaults
void read_nonvol(void);                           // Read in the locations
void update_nonvol(unsigned int current_version); // Update the database if needed
void gen_position(int  v);                        // Reset the position values
void dump_nonvol(void);

/*
 * NON Vol Storage
 */

#define NONVOL_INIT           0x0
#define NONVOL_SENSOR_DIA     (NONVOL_INIT        + sizeof(int) + 2)       // Sensor diameter
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
#define NONVOL_POWER_SAVE     (NONVOL_WEST_Y      + sizeof(int) + 2)       // Power saver time
#define NONVOL_NAME_ID        (NONVOL_POWER_SAVE  + sizeof(int) + 2)       // Name Identifier
#define NONVOL_1_RINGx10      (NONVOL_NAME_ID     + sizeof(int) + 2)       // Size of the 1 ring in mm
#define NONVOL_LED_PWM        (NONVOL_1_RINGx10   + sizeof(int) + 2)       // LED PWM value
#define NONVOL_SEND_MISS      (NONVOL_LED_PWM     + sizeof(int) + 2)       // Send the MISS message when true
#define NONVOL_SERIAL_NO      (NONVOL_SEND_MISS   + sizeof(int) + 2)       // EIN
#define NONVOL_STEP_COUNT     (NONVOL_SERIAL_NO   + sizeof(int) + 2)       // Number of paper pulse steps
#define NONVOL_MFS            (NONVOL_STEP_COUNT  + sizeof(int) + 2)       // Multifunction switch operation 
#define NONVOL_STEP_TIME      (NONVOL_MFS         + sizeof(int) + 2)       // Stepper motor pulse duration
#define NONVOL_Z_OFFSET       (NONVOL_STEP_TIME   + sizeof(int) + 2)       // Distance from sensor plane to paper plane
#define NONVOL_PAPER_ECO      (NONVOL_Z_OFFSET    + sizeof(int) + 2)       // Advance witness paper if the shot is less than paper_eco
#define NONVOL_TARGET_TYPE    (NONVOL_PAPER_ECO   + sizeof(int) + 2)       // Modify the target processing (0 == Regular single bull)
#define NONVOL_TABATA_ENBL    (NONVOL_TARGET_TYPE + sizeof(int) + 2)       // Disable Tabata from the switches
#define NONVOL_TABATA_ON      (NONVOL_TABATA_ENBL + sizeof(int) + 2)       // Time that the Tabata timer is on
#define NONVOL_TABATA_REST    (NONVOL_TABATA_ON   + sizeof(int) + 2)       // Time that the Tabata timer is OFF
#define NONVOL_TABATA_CYCLES  (NONVOL_TABATA_REST + sizeof(int) + 2)       // Number of cycles in an event
#define NONVOL_RAPID_ON       (NONVOL_TABATA_CYCLES+sizeof(int))           // Time that the Tabata timer is on
#define NONVOL_RAPID_REST     (NONVOL_RAPID_ON    + sizeof(int))           // Time that the Tabata timer is OFF
#define NONVOL_RAPID_CYCLES   (NONVOL_RAPID_REST  + sizeof(int))           // Number of cycles in an event
#define NONVOL_vset_PWM      (NONVOL_RAPID_CYCLES + sizeof(int))          // Starting PWM setting
#define NONVOL_VSET           (NONVOL_vset_PWM   + sizeof(int))           // Desired Voltage value (floating point)
#define NONVOL_RAPID_TYPE     (NONVOL_VSET        + sizeof(double))        // Type of rapid fire event
#define NONVOL_PS_VERSION     (NONVOL_RAPID_TYPE  + sizeof(int))           // Persistent storage version
#define NONVOL_FOLLOW_THROUGH (NONVOL_PS_VERSION  + sizeof(int))           // Follow through timer
#define NONVOL_KEEP_ALIVE     (NONVOL_FOLLOW_THROUGH + sizeof(int))        // Send out a keep alive at a rate
#define NONVOL_TABATA_WARN_ON (NONVOL_KEEP_ALIVE + sizeof(int))            // Time to turn the warning on
#define NONVOL_TABATA_WARN_OFF (NONVOL_TABATA_WARN_ON + sizeof(int))       // Time to turn the warning off

#define NEXT_NONVOL           (NONVOL_TABATA_WARN_OFF + sizeof(int) )
#define NONVOL_SIZE           4096                                         // 4K available

#endif
