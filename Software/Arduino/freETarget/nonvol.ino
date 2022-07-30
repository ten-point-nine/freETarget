/*-------------------------------------------------------
 * 
 * nonvol.ino
 * 
 * Nonvol storage managment
 * 
 * ----------------------------------------------------*/

/*----------------------------------------------------------------
 * 
 * funciton: check_nonvol
 * 
 * brief:    Read nonvol and set up variables
 * 
 * return:   Nonvol values copied to RAM
 * 
 *---------------------------------------------------------------
 *
 * Check to see if the NONVOL has been initialized correctly
 *
 *------------------------------------------------------------*/
void check_nonvol(void)
{
  unsigned int nonvol_init;
  
  if ( is_trace )
  {
    Serial.print(T("\r\nChecking NONVOL"));
  }
  
/*
 * Read the nonvol marker and if uninitialized then set up values
 */
  EEPROM.get(NONVOL_INIT, nonvol_init);
  
  if ( nonvol_init != INIT_DONE)                       // EEPROM never programmed
  {
    factory_nonvol(true);                              // Force in good values
  }

/*
 * Check to see if there has been a change to the persistent storage version
 */
  EEPROM.get(NONVOL_PS_VERSION, nonvol_init);
  if ( nonvol_init != PS_VERSION )                    // Is what is in memory not the same as now
  {
    update_nonvol(nonvol_init);                       // Then update the version
  }
  
/*
 * All OK now
 */
  return;
}

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
  unsigned int serial_number;             // Board serial number
  char         ch;
  unsigned int x;                         // Temporary Value
  double       dx;                        // Temporarty Value

  EEPROM.get(NONVOL_SERIAL_NO, serial_number); // record the staring serial number
  Serial.print(serial_number);
  Serial.print(new_serial_number);
  delay(5);
/*
 * Fill up all of memory with a known (bogus) value
 */
  Serial.print(T("\r\nReset to factory defaults. This may take a while\r\n"));
  ch = 0xAB;
  for ( i=0; i != NONVOL_SIZE; i++)
  {
    if ( (i < NONVOL_SERIAL_NO) || (i >= NONVOL_SERIAL_NO + sizeof(int) + 2) ) // Bypass the serial number
    {
      EEPROM.put(i, ch);                    // Fill up with a bogus value
      if ( (i % 64) == 0 )
      {
        Serial.print(T("."));
      }
    }
  }
  
  gen_position(0); 
  x = 0;
  EEPROM.put(vset_PWM, x);
  if ( new_serial_number == false )
  {
    EEPROM.put(NONVOL_SERIAL_NO, serial_number);  // Put the serial number back
  }
 
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
        if ( JSON[i].non_vol != 0 )
        {
          EEPROM.put(JSON[i].non_vol, x);                    // Read in the value
        }
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
 *     Test the board only if it is a factor init
 */
  if ( new_serial_number )
  {
    Serial.print(T("\r\n Testing motor drive "));
    for (x=10; x != 0; x--)
    {
      Serial.print(x); Serial.print(T("+ "));
      paper_on_off(true);
      delay(ONE_SECOND/4);
      Serial.print(T("- "));
      paper_on_off(false);
      delay(ONE_SECOND/4);
    }
    Serial.print(T(" Test Complete\r\n"));
  }
/*
 * Set the trip point
 */
  set_trip_point(0);                      // And stay forever in the set trip mode

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
  nonvol_init = PS_VERSION;
  EEPROM.put(NONVOL_PS_VERSION, nonvol_init); // Write in the version number

  nonvol_init = INIT_DONE;
  EEPROM.put(NONVOL_INIT, nonvol_init);

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
 #define INIT_ALLOWED         1234        // Number user must enter to allow initialization
 #define INIT_SERIAL_NUMBER   1235        // Number used to re-enter the serial number
 
void init_nonvol
  (
    int verify                            // Verification code entered by user
  )
{
  unsigned int nonvol_init;               // Initialization token
           int serial_number;             // Board serial number
  char         ch;
  unsigned int x;                         // Temporary Value
  double       dx;                        // Temporarty Value

/*
 * Ensure that the user wants to init the unit
 */
  if ( (verify != INIT_ALLOWED) && (verify != INIT_SERIAL_NUMBER) )
  {
    Serial.print(T("\r\nUse {\"INIT\":1234}\r\n"));
    return;
  }

  factory_nonvol(verify == INIT_SERIAL_NUMBER);
  
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

  EEPROM.get(NONVOL_PS_VERSION, nonvol_init);         // See if ther has been a change to the
  if ( nonvol_init != PS_VERSION )                    // persistent storage version
  {
    update_nonvol(nonvol_init);
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
          if ( JSON[i].non_vol != 0 )                          // Is persistent storage enabled?
          {
            EEPROM.get(JSON[i].non_vol, x);                    // Read in the value
            if ( x == 0xABAB )                                 // Is it uninitialized?
            {
              x = JSON[i].init_value;                          // Yes, overwrite with the default
              EEPROM.put(JSON[i].non_vol, x);
            }
            *JSON[i].value = x;
          }
          else
          {
            *JSON[i].value = JSON[i].init_value;              // Persistent storage is not enabled, force a known value
          }
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
  EEPROM.get(NONVOL_TABATA_ENBL, x);                          // Override the Tabata
  if ( x == 0 )
  {
    json_tabata_on = 0;                                       // and turn it off
  }

  EEPROM.get(NONVOL_vset_PWM, json_vset_PWM);

  EEPROM.get(NONVOL_MFS, json_multifunction);                // Override the Multifunction switch if not initialized
  if ( json_multifunction > 29999 )
  {
    json_multifunction = (TABATA_ON_OFF * 100) + (ON_OFF * 10) + (PAPER_FEED);   // Put it to the default
    EEPROM.put(NONVOL_MFS, json_multifunction);
  }
  
/*
 * All done, begin the program
 */
  return;
}


/*----------------------------------------------------------------
 * 
 * function: update_nonvol
 * 
 * brief:  Update the nonvol values if there has been a change
 * 
 * return: None
 *---------------------------------------------------------------
 *
 * If the init_nonvol location is not set to INIT_DONE then
 * initilze the memory
 * 
 *------------------------------------------------------------*/

void update_nonvol
  (
    unsigned int current_version          // Version present in persistent storage
  )
{
  unsigned int i;                         // Iteration counter
  unsigned int ps_value;                  // Value read from persistent storage           
  unsigned int x;                         // Value read from table

  Serial.print("here");
/*
 * Check to see if this persistent storage has never had a version number
 */
  if ( PS_UNINIT(current_version) )
  {
    Serial.print(T("\n\rUpdating legacy persistent storage"));
    i=0;
    while ( JSON[i].token != 0 )
    { 
      switch ( JSON[i].convert )
      {        
      case IS_INT16:
        EEPROM.get(JSON[i].non_vol, ps_value);              // Pull up the value from memory
        if ( PS_UNINIT(ps_value) )                          // Uninitilazed?
        {
          EEPROM.put(JSON[i].non_vol, JSON[i].init_value);  // Initalize it from the table
        }
        break;

      default:
        break;
      }
      i++;
   }
   current_version = PS_VERSION;                            // Initialized, force in the current version
   EEPROM.put(NONVOL_PS_VERSION, current_version);
   Serial.print(T("\r\nDone\r\n"));
  }

/*
 * Previously initialized memory.  Add in the new fields and values
 */
  if ( current_version == 1 )                     
  {
    x = 3;                                                  // Use an int to make sure that
    EEPROM.put(NONVOL_FOLLOW_THROUGH, x);                   // EEPROM.put uses the right size
    current_version = 2;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }
  
  if ( current_version == 2 )                     
  {
    x = 120;                                                // Set to 120.  No harm if it's sent
    EEPROM.put(NONVOL_KEEP_ALIVE, x);
    current_version = 3;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }
  
  if ( current_version == 3 )                     
  {
    x = 20;                                                // 20 x 100 ms increments
    EEPROM.put(NONVOL_TABATA_WARN_ON, x);
    x = 20;                                                // 20 x 100 ms increments
    EEPROM.put(NONVOL_TABATA_WARN_OFF, x);
    current_version = 4;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }
  
  if ( current_version == 4 )                     
  {
    x = 5;                                                 // Five rings to accept a face strike
    EEPROM.put(NONVOL_FACE_STRIKE, x);
    current_version = 5;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 5 )                     
  {
    x = 0;                                                 // 0 shots in a rapid cycle
    EEPROM.put(NONVOL_RAPID_COUNT, x);
    x = 1;
    EEPROM.put(NONVOL_WIFI_CHANNEL, x);                     // Default to channel 1
    current_version = 6;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }
  if ( current_version != PS_VERSION )
  {
    Serial.print(T("\n\rVerify firmware"));
  }
/*
 * Up to date, return
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

/*----------------------------------------------------------------
 *
 * function: dump_nonvol
 * 
 * brief: Core dumo the nonvol memory
 * 
 * return: Nothing
 * 
 *---------------------------------------------------------------
 *
 *  This function resets the offsets to 0 whenever a new 
 *  sensor diameter is entered.
 *  
 *------------------------------------------------------------*/
void print_hex(unsigned int x)
{
  int i;

  i = (x >> 4) & 0x0f;
  Serial.print(to_hex[i]);
  i = x & 0x0f;
  Serial.print(to_hex[i]);
  return;
}
void dump_nonvol(void)
{
  int i;
  char x;
  char line[128];
  
/*
 * Loop and print out the nonvol
 */
  for (i=0; i != NONVOL_SIZE; i++)
  {
    if ( (i % 16) == 0 )
    {
      sprintf(line, "\r\n%04X: ", i);
      Serial.print(line);
    }
    EEPROM.get(i, x);
    print_hex(x);
    if ( ((i+1) % 2) == 0 )
    {
      Serial.print(" ");
    }
  }

 Serial.print("\n\r");
   
 /* 
  *  All done, return
  */
  return;
}
