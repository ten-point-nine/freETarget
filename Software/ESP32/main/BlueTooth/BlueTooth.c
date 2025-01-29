/******************************************************************************
 *
 * BlueTooth.c
 *
 * WBlueTooth Driver for FreeETarget
 *
 ******************************************************************************
 *
 * BlueTooth offers a wireless serial port for tablets:
.
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/bluetooth/index.html#
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/bluetooth/controller_vhci.html#application-examples
 *
 * *****************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "freETarget.h"
#include "http_client.h"
#include "WiFi.h"
#include "compute_hit.h"
#include "diag_tools.h"
#include "http_client.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"

/*
 * Macros
 */

/*
 * Variables
 */

/*
 * Private Functions
 */

/*
 * Definitions
 */

/*****************************************************************************
 *
 * @function: BLueTooth_init()
 *
 * @brief:    Initialize the BlueTooth Interface
 *
 * @return:   None
 *
 ******************************************************************************
 *

 *
 *******************************************************************************/
void BlueTooth_init(void)
{
  DLT(DLT_INFO, SEND(sprintf(_xs, "BlueTooth_init()");))

  /*
   *  All done
   */
  return;
}
