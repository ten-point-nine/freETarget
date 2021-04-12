/*-------------------------------------------------------
 * 
 * nonvol.ino
 * 
 * Nonvol storage managment
 * 
 * ----------------------------------------------------*/
#include "nonvol.h"
#include "freeTarget.h"
#include "json.h"
 
/*----------------------------------------------------------------
 * 
 * function: init_nonvol
 * 
 * brief: Initialize the NONVOL back to factory settings
 * 
 * return: None
 *---------------------------------------------------------------
 *
 * The variable NONVOL_INIT is corrupted and the NONVOL read back
 * in and initialized.
 * 
 *------------------------------------------------------------*/
void init_nonvol(int v)
{
  unsigned int nonvol_init;

  nonvol_init = 0;                        // Corrupt the init location
  EEPROM.put(NONVOL_INIT, nonvol_init);
  Serial.print("\r\nReset to factory defaults\r\n");
  read_nonvol();                          // Force in new values
  show_echo(0);                           // Display these settings
  set_trip_point(false);
/*
 * All done, return
 */
  return;
}

/*----------------------------------------------------------------
 * 
 * funciton: read_nonvol
 * 
 * brief: Read nonvol and set up variables
 * 
 * return: Nonvol values copied to RAM
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
  if ( nonvol_init != INIT_DONE)                       // EEPROM never programmed
  {
    Serial.print("\r\nInitializing NON-VOL");
    gen_position(0); 
    EEPROM.put(NONVOL_DIP_SWITCH,  0);   // No, set up the defaults
    EEPROM.put(NONVOL_SENSOR_DIA,  230.0); 
    EEPROM.put(NONVOL_PAPER_TIME,  0);
    EEPROM.put(NONVOL_TEST_MODE,   0);
    EEPROM.put(NONVOL_CALIBRE_X10, 45);
    EEPROM.put(NONVOL_LED_PWM,     50);
    EEPROM.put(NONVOL_POWER_SAVE, 30);
    EEPROM.put(NONVOL_NAME_ID,    1);
    EEPROM.put(NONVOL_1_RINGx10, 1555);
    EEPROM.put(NONVOL_SEND_MISS,  0);
    
    nonvol_init = INIT_DONE;
    EEPROM.put(NONVOL_INIT, INIT_DONE);
  }

/*
 * Read in the values and check against limits
 */
  EEPROM.get(NONVOL_DIP_SWITCH, json_dip_switch);     // Read the nonvol settings
  EEPROM.get(NONVOL_SENSOR_DIA, json_sensor_dia);
  EEPROM.get(NONVOL_TEST_MODE,  json_test);
  EEPROM.get(NONVOL_LED_PWM,    json_LED_PWM);
  
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
  json_calibre_x10 = 0;                               // AMB
  
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
    EEPROM.put(NONVOL_NAME_ID, json_name_id);         // Default to a 4.5mm pellet
  }
  
  json_sensor_angle = 45;                            // AMB
  
  EEPROM.put(NONVOL_SENSOR_ANGLE, json_sensor_angle);
    
  EEPROM.get(NONVOL_NORTH_X, json_north_x);  
  EEPROM.get(NONVOL_NORTH_Y, json_north_y);  
  EEPROM.get(NONVOL_EAST_X,  json_east_x);  
  EEPROM.get(NONVOL_EAST_Y,  json_east_y);  
  EEPROM.get(NONVOL_SOUTH_X, json_south_x);  
  EEPROM.get(NONVOL_SOUTH_Y, json_south_y);  
  EEPROM.get(NONVOL_WEST_X,  json_west_x);  
  EEPROM.get(NONVOL_WEST_Y,  json_west_y);  

  EEPROM.get(NONVOL_1_RINGx10,  json_1_ring_x10);

  EEPROM.get(NONVOL_POWER_SAVE, json_power_save);
  EEPROM.get(NONVOL_LED_PWM,    json_LED_PWM);
  EEPROM.get(NONVOL_SEND_MISS,  json_send_miss);
  
/*
 * All done, begin the program
 */
  return;
}

/*----------------------------------------------------------------
 *
 * function: gen_postion
 * 
 * brief: Generate new position varibles based on new sensor diameter
 * 
 * return: Position values stored in NONVOL
 * 
 *---------------------------------------------------------------
 *
 *  This function resets the offsets to 0 whenever a new 
 *  sensor diameter is entered.
 *  
 *------------------------------------------------------------*/
void gen_position(int v)
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

