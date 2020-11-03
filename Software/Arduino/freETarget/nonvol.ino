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
 * void reinit_nonvol()
 * 
 * Reinitialize the nonvol storage
 * 
 *---------------------------------------------------------------
 *
 *  Force the init to a bad value and then do a read_nonvol
 *  
 *------------------------------------------------------------*/
 void reinit_nonvol(void)
 {
    int nonvol_init = 0;                    // Corrupt the semephore
    EEPROM.put(NONVOL_INIT, nonvol_init);
    
    read_nonvol();                          // Regen the numbers
    Serial.print("\r\nReset to factory defaults\r\n");
    show_echo();
 /*
  * Nothing more to do, return
  */
    return;
 }
 
/*----------------------------------------------------------------
 * 
 * void init_nonvol()
 * 
 * Initialize the NONVOL back to factory settings
 * 
 *---------------------------------------------------------------
 *
 * The variable NONVOL_INIT is corrupted and the NONVOL read back
 * in and initialized.
 * 
 *------------------------------------------------------------*/
void init_nonvol(void)
{
  unsigned int nonvol_init;

  nonvol_init = 0;                        // Corrupt the init location
  EEPROM.put(NONVOL_INIT, nonvol_init);
  read_nonvol();                          // Force in new values
  show_echo();                            // Display these settings
  
/*
 * All done, return
 */
  return;
}

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
  Serial.print(nonvol_init); Serial.print(" "); Serial.print(read_DIP() & FACTORY);
  if ( (nonvol_init != INIT_DONE)                       // EEPROM never programmed
      || ((read_DIP() & FACTORY) != 0) )                       // Reset back to factory defaults
  {
    Serial.print("\r\nInitializing NON-VOL");
    json_dip_switch = 0;
    EEPROM.put(NONVOL_DIP_SWITCH, json_dip_switch);   // No, set up the defaults
    json_sensor_dia = 230.0;
    EEPROM.put(NONVOL_SENSOR_DIA, json_sensor_dia); 
    json_paper_time = 0;
    EEPROM.put(NONVOL_PAPER_TIME, json_paper_time);
    json_test = 0;
    EEPROM.put(NONVOL_TEST_MODE, json_test);
    json_calibre_x10 = 45;
    EEPROM.put(NONVOL_CALIBRE_X10, json_calibre_x10);
    json_sensor_angle = 45;
    EEPROM.put(NONVOL_SENSOR_ANGLE, json_sensor_angle);
    gen_position();    
    json_trip_point = INIT_TRIP_POINT; 
    EEPROM.put(NONVOL_TRIP_POINT, json_trip_point);
    json_name_id = 0; 
    EEPROM.put(NONVOL_NAME_ID, json_name_id);
    nonvol_init = INIT_DONE;
    EEPROM.put(NONVOL_INIT, nonvol_init);
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
    json_sensor_angle = 45;                             // Check for an undefined Angle
    EEPROM.put(NONVOL_SENSOR_ANGLE, json_sensor_angle);// Default to a 4.5mm pellet
  }

  EEPROM.get(NONVOL_NAME_ID,  json_name_id);
  if ( json_name_id == 0xffff )
  {
    json_name_id = 0;                                 // Check for an undefined Name
    EEPROM.put(NONVOL_NAME_ID, json_name_id);    // Default to a 4.5mm pellet
  }
  
  EEPROM.get(NONVOL_NORTH_X, json_north_x);  
  EEPROM.get(NONVOL_NORTH_Y, json_north_y);  
  EEPROM.get(NONVOL_EAST_X,  json_east_x);  
  EEPROM.get(NONVOL_EAST_Y,  json_east_y);  
  EEPROM.get(NONVOL_SOUTH_X, json_south_x);  
  EEPROM.get(NONVOL_SOUTH_Y, json_south_y);  
  EEPROM.get(NONVOL_WEST_X,  json_west_x);  
  EEPROM.get(NONVOL_WEST_Y,  json_west_y);  

  EEPROM.get(NONVOL_TRIP_POINT, json_trip_point);
  
/*
 * All done, begin the program
 */
  return;
}

/*----------------------------------------------------------------
 *
 * void gen_postion()
 * 
 * Generate new position varibles based on new sensor diameter
 * 
 *---------------------------------------------------------------
 *
 *  This function resets the offsets to 0 whenever a new 
 *  sensor diameter is entered.
 *  
 *------------------------------------------------------------*/
void gen_position(void)
{
 /*
  * Work out the geometry of the sensors
  */
  json_north_x = 0;
  json_north_y = 0;
  
  json_east_x = 0;
  json_east_y = 0;

  json_south_x = 0;
  json_south_y = 0;
  
  json_west_x = 0;
  json_west_y = 0;

 /*
  * Save to persistent storage
  */
  EEPROM.put(NONVOL_NORTH_X, json_north_x);  
  EEPROM.put(NONVOL_NORTH_Y, json_north_y);  
  EEPROM.put(NONVOL_EAST_X,  json_east_x);  
  EEPROM.put(NONVOL_EAST_Y,  json_east_y);  
  EEPROM.put(NONVOL_SOUTH_X, json_south_x);  
  EEPROM.put(NONVOL_SOUTH_Y, json_south_y);  
  EEPROM.put(NONVOL_WEST_X,  json_west_x);  
  EEPROM.put(NONVOL_WEST_Y,  json_west_y);  
   
 /* 
  *  All done, return
  */
  return;
}
