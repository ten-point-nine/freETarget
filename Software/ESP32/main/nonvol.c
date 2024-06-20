/*-------------------------------------------------------
 * 
 * nonvol.c
 * 
 * Nonvol storage managment
 * 
 *-------------------------------------------------------
 *
 * See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
 *
 * ----------------------------------------------------*/
#include "nvs_flash.h"
#include "nvs.h"
#include "string.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "json.h"
#include "serial_io.h"
#include "nonvol.h"

/*
 *  Local variables
 */
nvs_handle_t my_handle;                    // Handle to NVS space

/*----------------------------------------------------------------
 * 
 * @functoon: read_nonvol
 * 
 * @brief: Read nonvol and set up variables
 * 
 * @return: Nonvol values copied to RAM
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
  long          nonvol_init;
  unsigned int  i;             // Iteration Counter
  long          x;             // 32 bit number
  size_t        length;        // Length of input string
  esp_err_t     err;           // ESP32 error type

  DLT(DLT_CRITICAL, printf("read_nonvol()");)

 /*
  * Initialize NVS
  */
    err = nvs_flash_init();
    if (err != 0)
    {
        DLT(DLT_CRITICAL, printf("read_nonvol(): Failed to initialize NVM");)
        ESP_ERROR_CHECK(nvs_flash_erase());        // NVS partition was truncated and needs to be erased
        err = nvs_flash_init();
    }

/*
 * Read the nonvol marker and if uninitialized then set up values
 */

  if (nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle) != ESP_OK)
  {
    DLT(DLT_CRITICAL, printf("read_nonvol(): Failed to open NVM");)
  }
        
  nvs_get_i32(my_handle, "NONVOL_INIT", &nonvol_init);

  if ( nonvol_init != INIT_DONE)                       // EEPROM never programmed
  {
    factory_nonvol(true);                              // Force in good values
  }

  nvs_get_i32(my_handle, "NVM_SERIAL_NO", &nonvol_init);

  if ( nonvol_init == (-1) )                          // Serial Number never programmed
  {
    factory_nonvol(true);                             // Force in good values
  }

  nvs_get_i32(my_handle, NONVOL_PS_VERSION, &nonvol_init);
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
     switch ( JSON[i].convert & IS_MASK )
     {
        case IS_VOID:
          break;
          
        case IS_TEXT:
        case IS_SECRET:
          if ( JSON[i].non_vol != 0 )                           // Is persistent storage enabled?
          {
            length = JSON[i].convert & FLOAT_MASK;
            nvs_get_str(my_handle, JSON[i].non_vol, (char*)JSON[i].value, &length);
          }
          break;

        case IS_INT32:
        case IS_FIXED:
        case IS_MFS:
          if ( JSON[i].non_vol != 0 )                          // Is persistent storage enabled?
          {
            nvs_get_i32(my_handle, JSON[i].non_vol, &x);       // Read in the value
            *JSON[i].value = x;
          }
          else
          {
            *JSON[i].value = JSON[i].init_value;              // Persistent storage is not enabled, force a known value
          }
          break;

        case IS_FLOAT:
          if ( JSON[i].non_vol != 0 )
          {
            nvs_get_i32(my_handle, JSON[i].non_vol, &x);       // Read in the value as an integer
            *JSON[i].d_value = (float)x / 1000.0;
          }
          else
          {
            *JSON[i].d_value = (double)JSON[i].init_value /1000.0;  
          }
          break;
      }
   }
   i++;
 }

/*
 * Go through and verify that the special cases are taken care of
 */
//  multifunction_switch();                                   // Look for an override on the target type
  
/*
 * All done, begin the program
 */
  return;
}


/*----------------------------------------------------------------
 * 
 * @function: factory_nonvol
 * 
 * @brief: Initialize the NONVOL back to factory settings
 * 
 * @return: None
 * 
 *---------------------------------------------------------------
 *
 * If the nonvol_init location is not set to INIT_DONE then
 * initilze the memory.
 * 
 * Copy the initial values from the JSON table to the NONVOL
 * memory.
 * 
 *------------------------------------------------------------*/
void factory_nonvol
  (
   bool new_serial_number
  )
{
  unsigned int serial_number;             // Board serial number
  char         ch, s[32];
  unsigned int x;                         // Temporary Value
  unsigned int i;                         // Iteration Counter
  
  DLT(DLT_CRITICAL, printf("factory_nonvol(%d)\r\n", new_serial_number); )

  serial_number = 0;
  x = 0;
  nvs_set_u32(my_handle, "NONVOL_V_SET", 0);
  if ( new_serial_number == false )
  {
    nvs_set_u32(my_handle, "NONVOL_V_SET", serial_number);
  }

/*
 * Use the JSON table to initialize the local variables
 */
  i=0;
  while ( JSON[i].token != 0 )
  {
    switch ( JSON[i].convert & IS_MASK )
    {
       case IS_VOID:                                      // Variable does not contain anything 
       case IS_FIXED:                                     // Variable cannot be overwritten                                    // MFS initialized from MFS entry
       break;
       
       case IS_TEXT:
       case IS_SECRET:
        if ( JSON[i].non_vol != 0 )
        {
          s[0] = 0;
          nvs_set_str(my_handle, JSON[i].non_vol, s);      // Zero out the text
        }
        break;
        
      case IS_MFS:
      case IS_INT32:
        x = JSON[i].init_value;                                               // Read in the value 
        if ( JSON[i].non_vol != 0 )
        {
          nvs_set_i32(my_handle, JSON[i].non_vol, x);                         // Read in the value
        }
        break;

      case IS_FLOAT:
        x = JSON[i].init_value;                                               // Read in the value 
        if ( JSON[i].non_vol != 0 )
        {
          nvs_set_i32(my_handle, JSON[i].non_vol, x);                         // Read in the value
        }
        break;
    }
   i++;
  }

#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
  strcpy(json_remote_url, REMOTE_URL);
  nvs_set_str(my_handle, NONVOL_REMOTE_URL, json_remote_url);
  nvs_set_i32(my_handle, NONVOL_REMOTE_ACTIVE, 0);
#endif 
/*    
 *     Test the board only if it is a factor init
 */
  if ( new_serial_number )
  {
    factory_test();
  }

/*
 * Ask for the serial number.  Exit when you get !
 */
  if ( new_serial_number )
  {
    ch = 0;
    serial_number = 0;
    serial_flush(ALL);
    
    printf("\r\nSerial Number? (ex 223! or x))");

    while (1)
    {
      if ( serial_available(CONSOLE) != 0 )
      {
        ch = serial_getch(CONSOLE);
        serial_putch(ch, CONSOLE);
        
        if ( ch == '!' )
        {  
          nvs_set_i32(my_handle, NONVOL_SERIAL_NO, serial_number);
          printf("\r\nSetting Serial Number to: %d", serial_number);
          break;
        }

        if ( (ch == 'x') || (ch == 'X') )
        {
          break;
        }

        if ( ch == 0x08 )          // Backspace
        {
          serial_number /= 10;
        }
        else
        {
          serial_number *= 10;
          serial_number += ch - '0';
        }
      }
    }
  }
  
/*
 * Initialization complete.  Mark the init done
 */
#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
  strcpy(json_remote_url, REMOTE_URL);
  nvs_set_str(my_handle, NONVOL_REMOTE_URL,    json_remote_url);
#endif
  nvs_set_i32(my_handle, NONVOL_PS_VERSION, PS_VERSION); // Write in the version number
  nvs_set_i32(my_handle, NONVOL_INIT, INIT_DONE);
  if ( nvs_commit(my_handle) )
  {
    DLT(DLT_CRITICAL, printf("Failed to write factory defaults to NONVOL");)
  }
    
/*
 * All done, return
 */    
  DLT(DLT_CRITICAL, printf("Factory Init complete\r\n");)
  return;
}

 
/*----------------------------------------------------------------
 * 
 * @function: nonvol_init
 * 
 * @brief: Initialize the NONVOL back to factory settings
 * 
 * @return: None
 *---------------------------------------------------------------
 *
 * {"INIT":1234} Reset but leave serial number alone
 * {"INIT":1235} Reset and prompt for new serial number
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
 
void nonvol_init
  (
    int verify                            // Verification code entered by user
  )
{
/*
 * Ensure that the user wants to init the unit
 */
  if ( (verify != INIT_ALLOWED) && (verify != INIT_SERIAL_NUMBER) )
  {
    printf("\r\nUse {\"INIT\":1234}\r\n");
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
 * @function: update_nonvol
 * 
 * @brief:  Update the nonvol to the current persistent storage version
 * 
 * @return: None
 *---------------------------------------------------------------
 *
 * Check the stored nonvol value against the current persistent
 * storage version and update if needed.
 * 
 *------------------------------------------------------------*/
void update_nonvol
  (
    unsigned int current_version  // Version present in persistent storage
  )
{
  unsigned int  i;                // Iteration counter
  long          ps_value;         // Value read from persistent storage  
  
  DLT(DLT_CRITICAL, printf("update_nonvol(%d)\r\n", current_version);)

/*
 * Check to see if this persistent storage has never had a version number
 */
  if ( PS_UNINIT(current_version) )
  {
    i=0;
    while ( JSON[i].token != 0 )
    { 
      switch ( JSON[i].convert & IS_MASK )
      {        
      case IS_INT32:
        nvs_get_i32(my_handle, JSON[i].non_vol, &ps_value);             // Pull up the value from memory
        if ( PS_UNINIT(ps_value) )                                      // Uninitilazed?
        {
          nvs_set_i32(my_handle, JSON[i].non_vol, JSON[i].init_value);  // Initalize it from the table
        }
        break;

      default:
        break;
      }
      i++;
   }
   current_version = PS_VERSION;                            // Initialized, force in the current version
   nvs_set_i32(my_handle, NONVOL_PS_VERSION, current_version);
   nvs_commit(my_handle);
  }

/* 
 * Version 0 -> 1 Split MFS apart and store separatly
 */
  if ( current_version == 0 )
  {  
    DLT(DLT_CRITICAL, printf("Updating PS0 to PS1");)

    json_mfs_hold_ab = HOLD_AB(json_multifunction);
    nvs_set_i32(my_handle, NONVOL_MFS_HOLD_AB,   json_mfs_hold_ab);

    json_mfs_tap_a = TAP_A(json_multifunction);
    nvs_set_i32(my_handle, NONVOL_MFS_TAP_A,     json_mfs_tap_a);

    json_mfs_tap_b = TAP_B(json_multifunction);
    nvs_set_i32(my_handle, NONVOL_MFS_TAP_B,     json_mfs_tap_b);

    json_mfs_hold_a = HOLD_A(json_multifunction);
    nvs_set_i32(my_handle, NONVOL_MFS_HOLD_A,     json_mfs_hold_a);

    json_mfs_hold_b = HOLD_B(json_multifunction);
    nvs_set_i32(my_handle, NONVOL_MFS_HOLD_B,     json_mfs_hold_b);

    json_mfs_hold_c = 0;
    nvs_set_i32(my_handle, NONVOL_MFS_HOLD_C,    json_mfs_hold_c);

    json_mfs_hold_d = 0;
    nvs_set_i32(my_handle, NONVOL_MFS_HOLD_D,    json_mfs_hold_d);

    json_mfs_select_cd = 0;
    nvs_set_i32(my_handle, NONVOL_MFS_SELECT_CD, json_mfs_select_cd);

#if ( BUIILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE)
    strcpy(json_remote_url, REMOTE_URL);
    nvs_set_str(my_handle, NONVOL_REMOTE_URL,    json_remote_url);
    
    strcpy(json_remote_url, REMOTE_URL);
    nvs_set_str(my_handle, NONVOL_REMOTE_URL, json_remote_url);
    
    nvs_set_i32(my_handle, NONVOL_REMOTE_ACTIVE, 0);
#endif
    current_version = 1;
  }

/*
 * Up to date, return
 */
  nvs_set_i32(my_handle, NONVOL_PS_VERSION, current_version);
  nvs_commit(my_handle);
  return;
}
