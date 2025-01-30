/*******************************************************************************
 *
 * file: helpers.c
 *
 * FreeETarget helper files
 *
 *******************************************************************************
 *
 * Helper files that have no pre-defined home
 *
 ******************************************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_event.h"

#include "freETarget.h"
#include "token.h"
#include "json.h"
#include "timer.h"
#include "serial_io.h"
#include "wifi.h"
#include "diag_tools.h"
#include "http_client.h"
#include "http_server.h"
#include "BLueTooth.h"

/*-----------------------------------------------------
 *
 * @function: target_name
 *
 * @brief: Determine the target name and return
 *
 * @return: Target name returned via pointer
 *
 *-----------------------------------------------------
 *
 * Depending on the settings, determine the target name
 * and return it to the caller
 *
 *-----------------------------------------------------*/
void target_name(char *name_space)
{

  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF) )
  {
    if ( json_name_id != JSON_NAME_TEXT )
    {
      sprintf(name_space, "FET-%s", names[json_name_id]);
    }
    else
    {
      if ( json_name_text[0] != 0 )
      {
        sprintf(name_space, "FET-%s", json_name_text);
      }
      else
      {
        sprintf(name_space, "Undefined name");
      }
    }
  }
  else
  {
    sprintf(name_space, "FET-%d", my_ring);
  }

  /*
   * All done, return
   */
  return;
}