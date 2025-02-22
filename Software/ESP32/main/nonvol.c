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
#include "nvs.h"
#include "nvs_flash.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "json.h"
#include "mfs.h"
#include "nonvol.h"
#include "serial_io.h"
#include "string.h"

/*
 *  Local variables
 */
nvs_handle_t my_handle; // Handle to NVS space

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
  long         nonvol_init;
  unsigned int i;      // Iteration Counter
  long         x;      // 32 bit number
  size_t       length; // Length of input string
  esp_err_t    err;    // ESP32 error type

  DLT(DLT_INFO, SEND(sprintf(_xs, "read_nonvol()");))

  /*
   * Initialize NVS
   */
  err = nvs_flash_init();
  if ( err != 0 )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "read_nonvol(): Failed to initialize NVM");))
    ESP_ERROR_CHECK(nvs_flash_erase()); // NVS partition was truncated and needs to be erased
    err = nvs_flash_init();
  }

  /*
   * Read the nonvol marker and if uninitialized then set up values
   */

  if ( nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "read_nonvol(): Failed to open NVM");))
  }

  nvs_get_i32(my_handle, "NONVOL_INIT", &nonvol_init);

  if ( nonvol_init != INIT_DONE )  // EEPROM never programmed
  {
    factory_nonvol(true);          // Force in good values
  }

  nvs_get_i32(my_handle, "NVM_SERIAL_NO", &nonvol_init);

  if ( nonvol_init == (-1) )       // Serial Number never programmed
  {
    factory_nonvol(true);          // Force in good values
  }

  nvs_get_i32(my_handle, NONVOL_PS_VERSION, &nonvol_init);
  if ( nonvol_init != PS_VERSION ) // persistent storage version
  {
    update_nonvol(nonvol_init);
    printf("back here");
  }

  /*
   * Use the JSON table to initialize the local variables
   */
  i = 0;
  while ( JSON[i].token != 0 )
  {
    if ( JSON[i].value != 0 ) // There is a value stored in memory
    {
      switch ( JSON[i].convert & IS_MASK )
      {
        case IS_VOID:
          break;

        case IS_TEXT:
        case IS_SECRET:
          if ( JSON[i].non_vol != 0 ) // Is persistent storage enabled?
          {
            length = JSON[i].convert & FLOAT_MASK;
            nvs_get_str(my_handle, JSON[i].non_vol, (char *)JSON[i].value, &length);
          }
          break;

        case IS_INT32:
        case IS_FIXED:
        case IS_MFS:
          if ( JSON[i].non_vol != 0 )                    // Is persistent storage enabled?
          {
            nvs_get_i32(my_handle, JSON[i].non_vol, &x); // Read in the value
            *JSON[i].value = x;
          }
          else
          {
            *JSON[i].value = JSON[i].init_value;         // Persistent storage is not enabled, force a known value
          }
          break;

        case IS_FLOAT:
          if ( JSON[i].non_vol != 0 )
          {
            nvs_get_i32(my_handle, JSON[i].non_vol, &x); // Read in the value as an integer
            *(double *)(JSON[i].value) = (float)x / 1000.0;
          }
          else
          {
            *(double *)(JSON[i].value) = (double)JSON[i].init_value / 1000.0;
          }
          break;
      }
    }
    i++;
  }

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
 * If the init_nonvol location is not set to INIT_DONE then
 * initilze the memory.
 *
 * Copy the initial values from the JSON table to the NONVOL
 * memory.
 *
 *------------------------------------------------------------*/
void factory_nonvol(bool new_serial_number // TRUE if prompting for a new S/N
)
{
  unsigned int serial_number;              // Board serial number
  char         ch, s[32];
  unsigned int x;                          // Temporary Value
  unsigned int i;                          // Iteration Counter

  DLT(DLT_INFO, SEND(sprintf(_xs, "factory_nonvol(%d)\r\n", new_serial_number);))

  serial_number = 0;
  x             = 0;
  nvs_set_u32(my_handle, "NONVOL_V_SET", 0);
  if ( new_serial_number == false )
  {
    nvs_set_u32(my_handle, "NONVOL_V_SET", serial_number);
  }

  /*
   * Use the JSON table to initialize the local variables
   */
  i = 0;
  while ( JSON[i].token != 0 )
  {
    switch ( JSON[i].convert & IS_MASK )
    {
      case IS_VOID:  // Variable does not contain anything
      case IS_FIXED: // Variable cannot be overwritten                                    // MFS initialized from MFS entry
        break;

      case IS_TEXT:
      case IS_SECRET:
        if ( JSON[i].non_vol != 0 )
        {
          s[0] = 0;
          nvs_set_str(my_handle, JSON[i].non_vol, s); // Zero out the text
        }
        break;

      case IS_MFS:
      case IS_INT32:
        x = JSON[i].init_value;                       // Read in the value
        if ( JSON[i].non_vol != 0 )
        {
          nvs_set_i32(my_handle, JSON[i].non_vol, x); // Read in the value
        }
        break;

      case IS_FLOAT:
        x = JSON[i].init_value;                       // Read in the value
        if ( JSON[i].non_vol != 0 )
        {
          nvs_set_i32(my_handle, JSON[i].non_vol, x); // Read in the value
        }
        break;
    }
    i++;
  }

  /*
   *     Test the board only if it is a factor init
   */
  if ( new_serial_number )
  {
    if ( factory_test() == false )
    {
      SEND(sprintf(_xs, "\r\nFactory test did not pass.");)
      SEND(sprintf(_xs, "\r\nFactory Test will not be recorded");)
    }

    /*
     * Ask for the serial number.  Exit when you get !
     */
    ch            = 0;
    serial_number = 0;
    serial_flush(ALL);

    SEND(sprintf(_xs, "\r\nSerial Number? (ex 223! or X to cancel))");)

    while ( 1 )
    {
      if ( serial_available(CONSOLE) != 0 )
      {
        ch = serial_getch(CONSOLE);
        serial_putch(ch, CONSOLE);

        switch ( ch )
        {
          case '!':
            nvs_set_i32(my_handle, NONVOL_SERIAL_NO, serial_number);
            SEND(sprintf(_xs, "\r\nSetting Serial Number to: %d", serial_number);)
            break;

          case 0x08: // Backspace
            serial_number /= 10;
            break;

          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            serial_number *= 10;
            serial_number += ch - '0';
            break;
        }
      }
      vTaskDelay(10);
      if ( (ch == 'x') || (ch == 'X') || (ch == '!') )
      {
        break;
      }
    }
  }

  /*
   * Initialization complete.  Mark the init done
   */
  nvs_set_i32(my_handle, NONVOL_PS_VERSION, PS_VERSION); // Write in the version number
  nvs_set_i32(my_handle, NONVOL_INIT, INIT_DONE);
  if ( nvs_commit(my_handle) )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to write factory defaults to NONVOL");))
  }

  /*
   * All done, return
   */
  read_nonvol(); // Put the NONVOL into the working JSON variables
  DLT(DLT_INFO, SEND(sprintf(_xs, "Factory Init complete\r\n");))
  return;
}

/*----------------------------------------------------------------
 *
 * @function: init_nonvol
 *
 * @brief: Initialize the NONVOL back to factory settings
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * init_nonvol is called from the command line by {"INIT":1234}
 * It will reset the NONVOL as a factory nonvol.
 *
 *------------------------------------------------------------*/
#define INIT_ALLOWED 1234   // Number user must enter to allow initialization

void init_nonvol(int verify // Verification code entered by user
)
{
  /*
   * Ensure that the user wants to init the unit
   */
  if ( prompt_for_confirm() == false )
  {
    SEND(sprintf(_xs, "\r\nInitialization cancelled\r\n");)
    return;
  }

  factory_nonvol(true); // Reset to facgtory defaults and prompt for serial number

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

void update_nonvol(unsigned int current_version // Version present in persistent storage
)
{
  unsigned int i, version;                      // Iteration counter
  long         ps_value;                        // Value read from persistent storage

  DLT(DLT_INFO, SEND(sprintf(_xs, "update_nonvol(%d)\r\n", current_version);))

  /*
   * Check to see if this persistent storage has never had a version number
   */
  if ( PS_UNINIT(current_version) )
  {
    i = 0;
    while ( JSON[i].token != 0 )
    {
      switch ( JSON[i].convert & IS_MASK )
      {
        case IS_INT32:
          nvs_get_i32(my_handle, JSON[i].non_vol, &ps_value);            // Pull up the value from memory
          if ( PS_UNINIT(ps_value) )                                     // Uninitilazed?
          {
            nvs_set_i32(my_handle, JSON[i].non_vol, JSON[i].init_value); // Initalize it from the table
          }
          break;

        default:
          break;
      }
      i++;
    }
    current_version = PS_VERSION; // Initialized, force in the current version
    nvs_set_i32(my_handle, NONVOL_PS_VERSION, current_version);
    nvs_commit(my_handle);
  }

                                  /*
                                   *  Loop and update each of the configurations
                                   */
  for ( version = current_version; version <= PS_VERSION; version++ ) // All possible version numbers
  {
    i = 0;
    while ( JSON[i].token != 0 )
    {
      if ( JSON[i].ps_version == version )
      {
        DLT(DLT_INFO, SEND(sprintf(_xs, "Updating PS%d", version);))
        switch ( JSON[i].convert & IS_MASK )
        {
          case IS_VOID:  // Variable does not contain anything
          case IS_FIXED: // Variable cannot be overwritten                                    // MFS initialized from MFS entry
            break;

          case IS_TEXT:
          case IS_SECRET:
            if ( JSON[i].non_vol != 0 )
            {
              _xs[0] = 0;
              nvs_set_str(my_handle, JSON[i].non_vol, _xs); // Zero out the text
            }
            break;

          case IS_MFS:
          case IS_INT32:
            if ( JSON[i].non_vol != 0 )
            {
              nvs_set_i32(my_handle, JSON[i].non_vol, JSON[i].init_value); // Read in the value
            }
            break;

          case IS_FLOAT:
            if ( JSON[i].non_vol != 0 )
            {
              nvs_set_i32(my_handle, JSON[i].non_vol, JSON[i].init_value); // Read in the value
            }
            break;
        }
      }
      i++;
    }
  }

  /*
   * Up to date, return
   */
  nvs_set_i32(my_handle, NONVOL_PS_VERSION, PS_VERSION);
  nvs_commit(my_handle);
  printf("bye bye");
  return;
}
