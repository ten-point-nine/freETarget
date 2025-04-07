/*-------------------------------------------------------
 *
 * file: OTA.c
 *
 * Over the Air Update
 *
 *-------------------------------------------------------
 *
 * Over the air update
 *
 * See:https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html
 *
 * ----------------------------------------------------*/
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "errno.h"

#include "freETarget.h"
#include "helpers.h"
#include "diag_tools.h"
#include "ota.h"
#include "json.h"

/*
 *  Macros
 */
#define BUFFSIZE     1024
#define HASH_LEN     32 /* SHA-256 digest length */
#define OTA_URL_SIZE 256

/*
 *  Local functions
 */
static void http_cleanup(esp_http_client_handle_t client);
static void OTA_fatal_error(char *LED_status, char *error_message);
static bool OTA_check_header(char *ota_write_data, int data_read, const esp_partition_t *update_partition,
                             const esp_partition_t *running_partition, const esp_partition_t *boot_partition);

/*
 * Variables
 */
static char          ota_write_data[BUFFSIZE + 1] = {0};
static char          download_url[OTA_URL_SIZE];
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

/*----------------------------------------------------------------
 *
 * @function: OTA_load()
 *
 * @brief:    Over The Air Update
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * TEST:
 *   {"OTA_URL":"targetbin.app/OTA"}
 *
 *------------------------------------------------------------*/

void OTA_load(void)
{
  esp_err_t                err;
  esp_ota_handle_t         update_handle = 0;
  const esp_partition_t   *update_partition;
  const esp_partition_t   *boot_partition           = esp_ota_get_boot_partition();
  const esp_partition_t   *running_partition        = esp_ota_get_running_partition();
  bool                     image_header_was_checked = false;
  int                      data_read;
  int                      binary_file_length = 0;
  esp_app_desc_t           new_app_info;
  esp_app_desc_t           running_app_info;
  const esp_partition_t   *last_invalid_app;
  esp_app_desc_t           invalid_app_info;
  esp_http_client_handle_t client;
  int                      http_count = 0; // How many records have we read in

  esp_http_client_config_t config = {
      .cert_pem          = (char *)server_cert_pem_start,
      .timeout_ms        = OTA_TIMEOUT,
      .keep_alive_enable = true,
  };

  /*
   *  Start
   */
  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "OTA_load()");))
  set_status_LED(LED_OTA_DOWNLOAD);

  if ( boot_partition == NULL )
  {
    DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Boot partition not available");))
  }

  if ( running_partition == NULL )
  {
    DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Running partition not available");))
  }

#if ( 0 )
  if ( boot_partition != running_partition )
  {
    DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Configured OTA boot partition at offset 0x%08" PRIx32 ", but running from offset 0x%08" PRIx32,
                                   configured->address, running->address);))
    DLT(DLT_INFO,
        SEND(ALL, sprintf(_xs, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");))
  }
  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")", running->type, running->subtype,
                                 running->address);))
#endif

  /*
   *  Prepare to open the file
   */
  sprintf(download_url, "http://%s/freeETarget.bin", json_ota_url);
  config.url = download_url;

  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "OTA download URL: %s", download_url);))

  client = esp_http_client_init(&config);

  if ( client == NULL )
  {
    OTA_fatal_error(LED_OTA_FAILED_CONNECT, "Failed to initialise HTTP connection");
  }

  err = esp_http_client_open(client, 0); // Open the connection to the server

  if ( err != ESP_OK )
  {
    OTA_fatal_error(LED_OTA_FAILED_CONNECT, "Failed to open HTTP connection");
  }

  /*
   *  Successsfully got the file
   */
  esp_http_client_fetch_headers(client);
  update_partition = esp_ota_get_next_update_partition(NULL);
  assert(update_partition != NULL);
  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Writing to partition subtype %d at offset 0x%" PRIx32, update_partition->subtype,
                                 update_partition->address);))

  // esp_http_client_read(client, ota_write_data, BUFFSIZE); // Throw away the HTTP header
  /*
   *  Prepare for the update
   */
  err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

  if ( err != ESP_OK )
  {
    http_cleanup(client);
    esp_ota_abort(update_handle);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT, "esp_ota_begin failed");
  }

  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "esp_ota_begin succeeded");))

  /*
   * Loop here and bring in the file
   */
  http_count = 0;

  while ( 1 )
  {
    data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
    http_count++;
    if ( (http_count & 32) == 0 )
    {
      set_status_LED(LED_OTA_DOWNLOAD); // Slowly blink the LED
    }
    else
    {
      set_status_LED(LED_OTA_DOWNLOAD_T);
    }

    if ( data_read < 0 ) // Error getting data
    {
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      OTA_fatal_error(LED_OTA_FAILED_CONNECT, "Error: HTTP data read error");
    }

    if ( data_read > 0 )                       // Have a payload to process
    {
      if ( image_header_was_checked == false ) // First time through, check the header
      {
        OTA_check_header(ota_write_data, data_read, update_partition, running_partition, boot_partition);
        image_header_was_checked = true;
      }
      /*
       * flash the received data
       */
      err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
      if ( err != ESP_OK )
      {
        http_cleanup(client);
        esp_ota_abort(update_handle);
        OTA_fatal_error(LED_OTA_FAILED_CONNECT, "esp_ota_write() failed");
      }
      binary_file_length += data_read;
    }

    if ( data_read == 0 )
    {
      /*
       * As esp_http_client_read never returns negative error code, we rely on
       * `errno` to check for underlying transport connectivity closure if any
       */
      if ( esp_http_client_is_complete_data_received(client) == true )
      {
        DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Connection closed normally");))
      }
      else
      {
        DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Connection closed, errno = %d", errno);))
      }
      break; // No more data to read, exit the loop
    }
  }

  /*
   *  The download is complete, make sure we have the complete file
   */
  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Total Write binary data length: %d", binary_file_length);))

  if ( esp_http_client_is_complete_data_received(client) != true )
  {
    http_cleanup(client);
    esp_ota_abort(update_handle);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT, "Error in receiving complete file");
  }

  esp_http_client_cleanup(client);

  err = esp_ota_end(update_handle);
  if ( err != ESP_OK )
  {
    http_cleanup(client);
    OTA_fatal_error(LED_OTA_FATAL, "Failed to complete OTA update");
  }

  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "esp_ota_end succesful");))

  /*
   * Looks good, setup the registers
   */
  err = esp_ota_set_boot_partition(update_partition);
  if ( err != ESP_OK )
  {
    http_cleanup(client);
    OTA_fatal_error(LED_OTA_FATAL, "esp_ota_set_boot_partition failed");
  }

  /*
   *  All done, reboot
   */
  http_cleanup(client);
  OTA_fatal_error(LED_OTA_READY, "Cycle power to start new firmware"); // Reboot the system
  return;
}

/*----------------------------------------------------------------
 *
 * @function: OTA_check_header()
 *
 * @brief:    Check the header for the OTA image
 *
 * @return:   TRUE if the header is valid
 *            Halt if an error is detected
 *
 *---------------------------------------------------------------
 *
 * Check the header for the OTA image
 *
 * Then verify the version so that we don't load the same or
 * older version of the image.
 *
 * An error in this function is fatal, ie we cannot recover
 * and trying to load it again will result in the same error.
 *
 *------------------------------------------------------------*/
static bool OTA_check_header(char                  *ota_write_data,    // Data read from server
                             int                    data_read,         // Number of bytes in record
                             const esp_partition_t *update_partition,  // Update partition
                             const esp_partition_t *running_partition, // Running partition
                             const esp_partition_t *boot_partition     // Boot partition
)
{
  esp_app_desc_t         new_app_info;
  esp_app_desc_t         running_app_info;
  const esp_partition_t *last_invalid_app;
  esp_app_desc_t         invalid_app_info;

  /*
   *  Check that we read enough bytes
   */
  if ( data_read < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t) )
  {
    OTA_fatal_error(LED_OTA_FATAL, "Failed to read .bin header");
  }

  /*
   *  Extract the data and print it out
   */
  memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
  DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "New firmware version: %s", new_app_info.version);))

  if ( esp_ota_get_partition_description(running_partition, &running_app_info) == ESP_OK )
  {
    DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Running firmware version: %s", running_app_info.version);))
  }

  last_invalid_app = esp_ota_get_last_invalid_partition();
  if ( last_invalid_app != NULL )
  {
    if ( esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK )
    {
      DLT(DLT_OTA, SEND(ALL, sprintf(_xs, "Last invalid firmware version: %s", invalid_app_info.version);))
    }

    if ( memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0 )
    {
      OTA_fatal_error(LED_OTA_FATAL, "New version is the same as invalid version. Rolled back to previous version.");
    }
  }

  if ( memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0 )
  {
    OTA_fatal_error(LED_OTA_FATAL, "Current running version is the same as a new. We will not continue the update.");
  }

  /*
   * Everything looks good
   */
  return true;
}

/*----------------------------------------------------------------
 *
 * @function: ota_rollback()
 *
 * @brief:    Undo the current partition and go back to the last
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * Check the stored nonvol value against the current persistent
 * storage version and update if needed.
 *
 *------------------------------------------------------------*/
void OTA_rollback(void)
{
  if ( esp_ota_mark_app_invalid_rollback_and_reboot() != ESP_OK )
  {
    OTA_fatal_error(LED_OTA_FATAL, "Rollback failed.");                         // Reboot the system
  }

  OTA_fatal_error(LED_OTA_READY, "Start rollback to the previous version ..."); // Reboot the system
}

/*----------------------------------------------------------------
 *
 * @function: http_cleanup()
 *
 * @brief:  Detect to see if we should roll back a version
 *
 * @return: TRUE - Rollback is required
 *---------------------------------------------------------------
 *
 * Check the stored nonvol value against the current persistent
 * storage version and update if needed.
 *
 *------------------------------------------------------------*/

static void http_cleanup(esp_http_client_handle_t client)
{
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return;
}

/*----------------------------------------------------------------
 *
 * @function: ota_partitions
 *
 * @brief:  Show the partition data
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * Display the partition data for the ESP32
 *
 * After a download the partitions are:
 *
 * Boot Partition
 * Not available
 *
 * Running Partition
 * Type:ESP_PARTITION_TYPE_APP  Subtype:0
 * Address: 0X10000 Size :0x200000
 * Lable: factory
 *
 * Update Partition
 * Type:ESP_PARTITION_TYPE_APP  Subtype:16
 * Address: 0X210000  Size 0x200000
 * Lable: ota_0
 *
 *------------------------------------------------------------*/
static char *partition_type[] = {"ESP_PARTITION_TYPE_APP", "ESP_PARTITION_TYPE_DATA"};

void OTA_partitions(void)
{
  const esp_partition_t *boot_partition    = esp_ota_get_boot_partition();
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  const esp_partition_t *update_partition  = esp_ota_get_next_update_partition(NULL);

  SEND(ALL, sprintf(_xs, "\r\nBoot Partition");)
  if ( boot_partition != NULL )
  {
    SEND(ALL, sprintf(_xs, "\r\nType:%s  Subtype:%d", partition_type[boot_partition->type], boot_partition->subtype);)
    SEND(ALL, sprintf(_xs, "\r\nAddress: 0X%lX  Size 0x%lX", boot_partition->address, boot_partition->size);)
    SEND(ALL, sprintf(_xs, "\r\nLable: %s\r\n", boot_partition->label);)
  }
  else
  {
    SEND(ALL, sprintf(_xs, " Not available\r\n");)
  }

  SEND(ALL, sprintf(_xs, "\r\nRunning Partition");)
  if ( running_partition != NULL )
  {
    SEND(ALL, sprintf(_xs, "\r\nType:%s  Subtype:%d", partition_type[running_partition->type], running_partition->subtype);)
    SEND(ALL, sprintf(_xs, "\r\nAddress: 0X%lX Size :0x%lX", running_partition->address, running_partition->size);)
    SEND(ALL, sprintf(_xs, "\r\nLable: %s\r\n", running_partition->label);)
  }
  else
  {
    SEND(ALL, sprintf(_xs, " Not available"); SEND(ALL, sprintf(_xs, "\r\n");))
  }

  SEND(ALL, sprintf(_xs, "\r\nUpdate Partition");)
  if ( update_partition != NULL )
  {
    SEND(ALL, sprintf(_xs, "\r\nType:%s  Subtype:%d", partition_type[update_partition->type], update_partition->subtype);)
    SEND(ALL, sprintf(_xs, "\r\nAddress: 0X%lX  Size 0x%lX", update_partition->address, update_partition->size);)
    SEND(ALL, sprintf(_xs, "\r\nLable: %s\r\n", update_partition->label);)
  }
  else
  {
    SEND(ALL, sprintf(_xs, " Not available\r\n");)
  }

  return;
}

/*----------------------------------------------------------------
 *
 * @function: OTA_fatal_error()
 *
 * @brief:    Manage an error we can't recover from
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * Display the error LEDs and halt the system
 *
 *------------------------------------------------------------*/
static void OTA_fatal_error(char *LED_status,   // Indicator sent to LEDs
                            char *error_message // Why are we halting
)
{
  set_status_LED(LED_status);

  DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "%s", error_message);))

  while ( 1 )
  {
    vTaskDelay(ONE_SECOND);
  }
}
