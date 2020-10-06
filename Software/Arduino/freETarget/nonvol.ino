/*-------------------------------------------------------
 * 
 * nonvol.ino
 * 
 * Nonvol storage managment
 * 
 * ----------------------------------------------------*/
#include "nonvol.h"

/*----------------------------------------------------------------
 * 
 * void read_nonvol()
 * 
 * Read nonvol and set up variables
 * 
 *---------------------------------------------------------------
 *
 * Read the nonvol into RAM.  
 * 
 * If the results is uninitalized then force the factory default.
 * Then check for out of bounds and reset those values
 *
 *------------------------------------------------------------*/
void read_nonvol(void)
{
  unsigned int nonvol_init;
/*
 * Read the nonvol marker and if uninitialized then set up values
 */
  EEPROM.get(NONVOL_INIT, nonvol_init);
  if ( nonvol_init != 0xABCD )                        // Has this already been initialized
  {
    Serial.print("\n\rInitializing NON-VOL");
    json_dip_switch = 0;
    EEPROM.put(NONVOL_DIP_SWITCH, json_dip_switch);   // No, set up the defaults
    json_sensor_dia = 230.0;
    EEPROM.put(NONVOL_SENSOR_DIA, json_sensor_dia);
    json_paper_time = 0;
    EEPROM.put(NONVOL_PAPER_TIME, json_paper_time);
    nonvol_init = 0xabcd;
    EEPROM.put(NONVOL_INIT, nonvol_init);
    json_test = 0;
    EEPROM.put(NONVOL_TEST_MODE, json_test);
    json_calibre_x10 = 45;
    EEPROM.put(NONVOL_CALIBRE_X10, json_calibre_x10);
    json_sensor_angle = 0;
    EEPROM.put(NONVOL_SENSOR_ANGLE, json_sensor_angle);
  }

/*
 * Read in the values and check against limits
 */
  EEPROM.get(NONVOL_DIP_SWITCH, json_dip_switch);     // Read the nonvol settings
  EEPROM.get(NONVOL_SENSOR_DIA, json_sensor_dia);

  EEPROM.get(NONVOL_TEST_MODE,  json_test);
  
  EEPROM.get(NONVOL_PAPER_TIME, json_paper_time);
  if ( (json_paper_time * PAPER_STEP) > (PAPER_LIMIT) )
  {
    json_paper_time = 0;                              // Check for an infinit loop
    EEPROM.put(NONVOL_PAPER_TIME, json_paper_time);   // and limit motor on time
  }
  
  EEPROM.get(NONVOL_CALIBRE_X10,  json_calibre_x10);
  if ( json_calibre_x10 > 100 )
  {
    json_calibre_x10 = 45;                            // Check for an undefined pellet
    EEPROM.put(NONVOL_CALIBRE_X10, json_calibre_x10); // Default to a 4.5mm pellet
  }
  
  EEPROM.get(NONVOL_SENSOR_ANGLE,  json_sensor_angle);
  if ( json_sensor_angle == 0xffff )
  {
    json_sensor_angle = 0;                             // Check for an undefined pellet
    EEPROM.put(NONVOL_SENSOR_ANGLE, json_sensor_angle);// Default to a 4.5mm pellet
  }

/*
 * All done, begin the program
 */
  return;
}
