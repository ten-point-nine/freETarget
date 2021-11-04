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
 * function: factory_nonvol
 * 
 * brief: Initialize the NONVOL back to factory settings
 * 
 * return: None
 *---------------------------------------------------------------
 *
 * If the init_nonvol location is not set to INIT_DONE then
 * initilze the memory
 * 
 *------------------------------------------------------------*/
void factory_nonvol
  (
   bool new_serial_number
  )
{
  unsigned int nonvol_init;               // Initialization token
           int serial_number;             // Board serial number
  char         ch;
  unsigned int x;                         // Temporary Value
  double       dx;                        // Temporarty Value

  /*
   * Fill up all of memory with a known (bogus) value
   */
  Serial.print(T("\r\nReset to factory defaults. This may take a while\r\n"));
  ch = 0xAB;
  for ( i=0; i != NONVOL_SIZE; i++)
  {
    EEPROM.put(i, ch);                    // Fill up with a bogus value
    if ( (i % 64) == 0 )
    {
      Serial.print(T("."));
    }
  }
  
  gen_position(0); 

/*
 * Use the JSON table to initialize the local variables
 */
  i=0;
  while ( JSON[i].token != 0 )
  {
    switch ( JSON[i].convert )
    {
       case IS_VOID:                                        // Variable does not contain anything 
       case IS_FIXED:                                       // Variable cannot be overwritten
       break;
        
      case IS_INT16:
        x = JSON[i].init_value;                            // Read in the value 
        Serial.print(T("\r\n")); Serial.print(JSON[i].token); Serial.print(T(" ")); Serial.print(x);
        EEPROM.put(JSON[i].non_vol, x);                    // Read in the value
        break;

      case IS_FLOAT:
      case IS_DOUBLE:
        dx = (double)JSON[i].init_value;
        Serial.print(T("\r\n")); Serial.print(JSON[i].token); Serial.print(T(" ")); Serial.print(dx);
        EEPROM.put(JSON[i].non_vol, dx);                    // Read in the value
        break;
    }
   i++;
 }
  Serial.print(T("\r\nDone\r\n"));
  
/*
 * Ask for the serial number.  Exit when you get !
 */
  if ( new_serial_number )
  {
    ch = 0;
    serial_number = 0;
    while ( Serial.available() )    // Eat any pending junk
    {
      Serial.read();
    }
  
    Serial.print(T("\r\nSerial Number? (ex 223! or x))"));
    while (i)
    {
      if ( Serial.available() != 0 )
      {
        ch = Serial.read();
        if ( ch == '!' )
        {  
          EEPROM.put(NONVOL_SERIAL_NO, serial_number);
          Serial.print(T(" Setting Serial Number to: ")); Serial.print(serial_number);
          break;
        }
        if ( ch == 'x' )
        {
          break;
        }
        serial_number *= 10;
        serial_number += ch - '0';
      }
    }
  }
  
/*
 * Initialization complete.  Mark the init done
 */
  nonvol_init = INIT_DONE;
  EEPROM.put(NONVOL_INIT, nonvol_init);
  
/*
 * Read the NONVOL and print the results
 */
  read_nonvol();                          // Read back the new values
  show_echo(0);                           // Display these settings
  set_trip_point(0);                      // And stay forever in the set trip mode
  
/*
 * All done, return
 */    

  return;
}

 
/*----------------------------------------------------------------
 * 
 * function: init_nonvol
 * 
 * brief: Initialize the NONVOL back to factory settings
 * 
 * return: None
 *---------------------------------------------------------------
 *
 * init_nonvol requires an arguement == 1234 before the 
 * initialization command will be executed.
 * 
 * The variable NONVOL_INIT is corrupted. and the values
 * copied out of the JSON[] table.  If the serial number has
 * not been initialized before, the user is prompted to enter
 * a serial number.
 * 
 * The function then continues to display the current trip
 * point value.
 * 
 *------------------------------------------------------------*/
void init_nonvol(int v)
{
  unsigned int nonvol_init;               // Initialization token
           int serial_number;             // Board serial number
  char         ch;
  unsigned int x;                         // Temporary Value
  double       dx;                        // Temporarty Value

/*
 * Ensure that the user wants to init the unit
 */
  if ( v != 1234 )
  {
    Serial.print(T("\r\nUse {\"INIT\":1234}\r\n"));
    return;
  }

  factory_nonvol(false);
  
/*
 * Read the NONVOL and print the results
 */
  read_nonvol();                          // Read back the new values
  show_echo(0);                           // Display these settings
  
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
  unsigned int  i;              // Iteration Counter
  unsigned int  x;              // 16 bit number
  double       dx;              // Floating point number
  
  if ( is_trace )
  {
    Serial.print(T("\r\nReading NONVOL"));
  }
  
/*
 * Read the nonvol marker and if uninitialized then set up values
 */
  EEPROM.get(NONVOL_INIT, nonvol_init);
  
  if ( nonvol_init != INIT_DONE)                       // EEPROM never programmed
  {
    factory_nonvol(true);                              // Force in good values
  }
  
  EEPROM.get(NONVOL_SERIAL_NO, nonvol_init);
  
  if ( nonvol_init == (-1) )                          // Serial Number never programmed
  {
    factory_nonvol(true);                             // Force in good values
  }
  
/*
 * Use the JSON table to initialize the local variables
 */
 i=0;
 while ( JSON[i].token != 0 )
 {
   if ( (JSON[i].value != 0) || (JSON[i].d_value != 0)  )    // There is a value stored in memory
   {
     switch ( JSON[i].convert )
     {
        case IS_VOID:
          break;
        
        case IS_INT16:
        case IS_FIXED:
          EEPROM.get(JSON[i].non_vol, x);                    // Read in the value
          if ( x == 0xABAB )                                 // Is it uninitialized?
          {
            x = JSON[i].init_value;                          // Yes, overwrite with the default
            EEPROM.put(JSON[i].non_vol, x);
          }
          *JSON[i].value = x;
          break;

        case IS_FLOAT:
        case IS_DOUBLE:
          EEPROM.get(JSON[i].non_vol, dx);                    // Read in the value
          *JSON[i].d_value = dx;
          break;
      }
   }
   i++;
 }

/*
 * Go through and verify that the special cases are taken care of
 */
  if ( (json_paper_time * PAPER_STEP) > (PAPER_LIMIT) )
  {
    json_paper_time = 0;                              // Check for an infinit loop
  }
  
  if ( (json_paper_time == 0xffff)  )
  {
    json_paper_time = 0;                              // Check for undefined time
  }

  if ( (json_step_time == 0xffff)  )
  {
    json_step_time = 0;                              // Check for undefined time
  }

  if ( (json_step_count == 0xffff)  )
  {
    json_step_count = 0;                              // Check for undefined time
  }
  
  if ( (json_z_offset == 0xffff)  )
  {
    json_z_offset  = 0;                               // Check for undefined time
  }
  
  if ( json_calibre_x10 > 100 )
  {
    json_calibre_x10 = 45;                            // Check for an undefined pellet
  }
  
  if ( json_sensor_angle == 0xffff )
  {
    json_sensor_angle = 45;                             // Check for an undefined Angle
  }

  if ( json_name_id == 0xffff )
  {
    json_name_id = 0;                                 // Check for an undefined Name
  }

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
