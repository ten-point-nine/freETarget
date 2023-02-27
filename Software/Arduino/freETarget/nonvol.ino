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
  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("Checking NONVOL"));
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
    backup_nonvol();                                  // Copy what we have 
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
  unsigned int i, j;                      // Iteration Counter
  
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
       
       case IS_TEXT:
       case IS_SECRET:
        Serial.print(T("\r\n")); Serial.print(JSON[i].token); Serial.print(T(" \"\""));
        if ( JSON[i].non_vol != 0 )
        {
          EEPROM.put(JSON[i].non_vol, 0);                    // Zero out the text
        }
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
 *      Make a backup of the settings
 */  backup_nonvol();

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
  show_echo();                            // Display these settings
  
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
  unsigned int  i, j;           // Iteration Counter
  unsigned int  x;              // 16 bit number
  double       dx;              // Floating point number
  unsigned char ch;             // Text value

  if ( DLT(DLT_CRITICAL) )
  {
    Serial.print(T("read_nonvol()"));
  }
  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("Reading NONVOL"));
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
          
        case IS_TEXT:
        case IS_SECRET:
          if ( JSON[i].non_vol != 0 )                           // Is persistent storage enabled?
          {
            j=0;
            while (1)                                           // Loop and copy the NONVOL 
            {
              EEPROM.get((JSON[i].non_vol+j), ch );
              *((unsigned char*)(JSON[i].value) + j) = ch;     // Over to the working storage
              if ( ch == 0 )
              {
                break;
              }
              j++;
            }
          }
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
  multifunction_switch();                                   // Look for an override on the target type
  
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
  double       y;                         // Floating point number
  
  if ( DLT(DLT_CRITICAL) )
  {
    Serial.print(T("update_nonvol(")); Serial.print(current_version); Serial.print(T(")")); 
  }
  
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
 * Look through the list of PS versions and see if we have initialized before
 * Old memory has a completly bogus verion number that is not in range
 */
  for (i=0; i <= (PS_VERSION+1); i++)
  {  
    if ( current_version == i )
    {
      break;
    }
  }
  if ( i == (PS_VERSION+1) )
  {
    current_version = 1;
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

  if ( current_version == 6 )                             // Version 6 removed
  {
    current_version = 7;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 7 )                     
  {
    x = 1;
    EEPROM.put(NONVOL_WIFI_DHCP, x);                      // Default DHCP to be on
    x = 0;
    EEPROM.put(NONVOL_WIFI_SSID, x);                      // No default SSID
    EEPROM.put(NONVOL_WIFI_PWD, x);                       // No default password
    EEPROM.put(NONVOL_WIFI_IP, x);                        // No default IP Address
    current_version = 8;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 8 )                     
  {
    x = 0;
    EEPROM.put(NONVOL_WIFI_IP, x);                        // No default IP address
    current_version = 9;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  
  if ( current_version == 9 )                             // Extend SSID to 32 bits                  
  {
    for (i=0; i != esp01_SSID_SIZE; i++ )
    {
      EEPROM.get(NONVOL_WIFI_SSID+i, x);
      EEPROM.put(NONVOL_WIFI_SSID_32+i, x);
    }
    current_version = 10;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 10 )                             // Fix PS Version 10 bug                
  {
    x=0;
    EEPROM.put(NONVOL_WIFI_SSID, x);                      // Version 10 put the SSID_32 in the
    EEPROM.put(NONVOL_WIFI_SSID_32, x);                   // wrong place so this patch
    EEPROM.put(NONVOL_WIFI_IP, x);                        // zero's out the variables.
    EEPROM.put(NONVOL_WIFI_PWD, x);
    current_version = 11;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 11 )                            // Add in Relative Humidity
  {
    x=50;
    EEPROM.put(NONVOL_RH, x);                             // Set RH to 50%
    current_version = 12;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 12 )                            // Add in minimum ring time
  {
    x=500;
    EEPROM.put(NONVOL_MIN_RING_TIME, x);                  // Set ring time to 500ms
    current_version = 13;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
  }

  if ( current_version == 13 )                            // Correction for Doppler Inverse Square
  {
    y = 7.0d / (sq(700.0d));                      
    EEPROM.put(NONVOL_DOPPLER, y);                        //  Adjust to 7 us per 700 us delay
    current_version = 14;
    EEPROM.put(NONVOL_PS_VERSION, current_version);
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
  int i, j;
  char x;
  char line[128];
  
/*
 * Loop and print out the nonvol
 */
  for (i=0; i != NONVOL_SIZE; i+= 16)
  {
    sprintf(line, "\r\n%04X: ", i);
    Serial.print(line);
    
    for ( j=0; j!= 16; j++)
    {
      EEPROM.get(i+j, x);
      print_hex(x);
      if ( ((j+1) % 4) == 0 )
      {
      Serial.print(" ");
      }
    }
        
    for ( j=0; j!= 16; j++)
    {
      EEPROM.get(i+j, x);
      if ( (x>=' ') && ( x <= 127) )
      {
        Serial.print(x);
      }
      else
      {
        Serial.print(T("."));
      }
      if ( ((j+1) % 4) == 0 )
      {
      Serial.print(" ");
      }
    }
  }

 Serial.print("\n\r");
   
 /* 
  *  All done, return
  */
  return;
}


/*----------------------------------------------------------------
 *
 * function: backup_nonvol
 * 
 * brief:  Copy the nonvol to a safe place
 * 
 * return: Nothing
 * 
 *---------------------------------------------------------------
 *
 *  This function copies the contents of 0x000 - 0x7ff to
 *  0x800 - 0xfff
 *  
 *------------------------------------------------------------*/

void backup_nonvol(void)
{
  int i;
  char x;

  Serial.print(T("\r\nStarting backup"));
/*
 * Loop and print out the nonvol
 */
  for (i=0; i != NONVOL_SIZE/2-1; i++)
  {
      EEPROM.get(i, x);
      EEPROM.put(i + NONVOL_SIZE/2, x);
  }
  
 Serial.print(T(" - done"));
   
 /* 
  *  All done, return
  */
  return;
}

/*----------------------------------------------------------------
 *
 * function: restore_nonvol
 * 
 * brief:  Copy the safe nonvol to memory
 * 
 * return: Nothing
 * 
 *---------------------------------------------------------------
 *
 *  This function copies the contents of 0x800 - 0xfff to
 *  0x000 - 0x7ff
 *  
 *------------------------------------------------------------*/

void restore_nonvol(void)
{
  int i;
  char x;

  Serial.print(T("\r\nStarting restore"));
/*
 * Loop and print out the nonvol
 */
  for (i=0; i != NONVOL_SIZE/2-1; i++)
  {
      EEPROM.get(i + NONVOL_SIZE/2, x);
      EEPROM.put(i, x);
  }
  
 Serial.print(T(" - done"));
   
 /* 
  *  All done, return
  */
  return;
}
