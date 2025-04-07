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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "errno.h"
#include "esp_wifi.h"

#include "freETarget.h"
#include "helpers.h"
#include "diag_tools.h"
#include "ota.h"
#include "json.h"

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

/*an ota data write buffer ready to write to the flash*/
static char          ota_write_data[BUFFSIZE + 1] = {0};
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

static void http_cleanup(esp_http_client_handle_t client);
static void OTA_fatal_error(char *LED_status);
static void print_sha256(const uint8_t *image_hash, const char *label);
static bool OTA_check_header(char                  *ota_write_data,    // Data read from server
                             int                    data_read,         // Number of bytes in record
                             const esp_partition_t *update_partition,  // Update partition
                             const esp_partition_t *running_partition, // Running partition
                             const esp_partition_t *boot_partition);   // Boot partition

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
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Start rollback to the previous version ...");))
  esp_ota_mark_app_invalid_rollback_and_reboot();
}

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
char download_url[OTA_URL_SIZE];

void OTA_load(void)
{
  esp_err_t err;
  /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
  esp_ota_handle_t         update_handle            = 0;
  const esp_partition_t   *update_partition         = esp_ota_get_next_update_partition(NULL);
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

  esp_http_client_config_t config = {
      .cert_pem          = (char *)server_cert_pem_start,
      .timeout_ms        = OTA_TIMEOUT,
      .keep_alive_enable = true,
  };

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "OTA_load()");))
  set_status_LED(LED_OTA_DOWNLOAD);

  if ( boot_partition == NULL )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Boot partition not available");))
  }

  if ( running_partition == NULL )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Running partition not available");))
  }

#if ( 0 )
  if ( boot_partition != running_partition )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Configured OTA boot partition at offset 0x%08" PRIx32 ", but running from offset 0x%08" PRIx32,
                                    configured->address, running->address);))
    DLT(DLT_INFO,
        SEND(ALL, sprintf(_xs, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");))
  }
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")", running->type, running->subtype,
                                  running->address);))
#endif

  /*
   *  Open the connection to the server
   */

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
  config.skip_cert_common_name_check = true;
#endif

  /*
   *  Prepare to open the file
   */
  sprintf(download_url, "http://%s/freeETarget.bin", json_ota_url);
  config.url = download_url;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "OTA download URL: %s", download_url);))

  client = esp_http_client_init(&config);

  if ( client == NULL )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Failed to initialise HTTP connection");))
    OTA_fatal_error(LED_OTA_FAILED_CONNECT);
  }
  err = esp_http_client_open(client, 0); // Open the connection to the server

  if ( err != ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Failed to open HTTP connection: %s", esp_err_to_name(err));))
    esp_http_client_cleanup(client);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT);
  }

  /*
   *  Successsfully got the file
   */
  esp_http_client_fetch_headers(client);
  assert(update_partition != NULL);
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Writing to partition subtype %d at offset 0x%" PRIx32, update_partition->subtype,
                                  update_partition->address);))

  // esp_http_client_read(client, ota_write_data, BUFFSIZE); // Throw away the HTTP header
  /*
   *  Prepare for the update
   */
  err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

  if ( err != ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "esp_ota_begin failed (%s)", esp_err_to_name(err));))
    http_cleanup(client);
    esp_ota_abort(update_handle);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT);
  }
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "esp_ota_begin succeeded");))

  /*
   * Loop here and bring in the file
   */
  while ( 1 )
  {
    data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);

    if ( data_read < 0 ) // Error getting data
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Error: SSL data read error");))
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      OTA_fatal_error(LED_OTA_FAILED_CONNECT);
    }

    else if ( data_read > 0 ) // Have a payload to process
    {
      if ( image_header_was_checked == false )
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
        OTA_fatal_error(LED_OTA_FAILED_CONNECT);
      }
      binary_file_length += data_read;
    }

    else if ( data_read == 0 )
    {
      /*
       * As esp_http_client_read never returns negative error code, we rely on
       * `errno` to check for underlying transport connectivity closure if any
       */
      if ( errno == ECONNRESET || errno == ENOTCONN )
      {
        DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Connection closed, errno = %d", errno);))
        break;
      }
      if ( esp_http_client_is_complete_data_received(client) == true )
      {
        DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Connection closed");))
        break;
      }
    }
  }

  /*
   *  The download is complete, make sure we have the complete file
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Total Write binary data length: %d", binary_file_length);))
  if ( esp_http_client_is_complete_data_received(client) != true )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Error in receiving complete file");))
    http_cleanup(client);
    esp_ota_abort(update_handle);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT);
  }

  err = esp_ota_end(update_handle);
  if ( err != ESP_OK )
  {
    if ( err == ESP_ERR_OTA_VALIDATE_FAILED )
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Image validation failed, image is corrupted");))
    }
    else
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "esp_ota_end failed (%s)!", esp_err_to_name(err));))
    }
    http_cleanup(client);
    OTA_fatal_error(LED_OTA_FATAL);
  }

/*
 * Looks good, setup the registers
 */
#if ( 0 )
  err = esp_ota_set_boot_partition(update_partition);
  if ( err != ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));))
    http_cleanup(client);
    OTA_fatal_error(LED_OTA_FAILED_CONNECT);
  }
#endif
  /*
   *  All done, reboot
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Prepare to restart system!");))
  esp_restart();
}

/*----------------------------------------------------------------
 *
 * @function: OTA_check_header()
 *
 * @brief:    Check the header for the OTA image
 *
 * @return:   TRUE if the header is valid
 *---------------------------------------------------------------
 *
 * Check the header for the OTA image
 *
 * THen verify the version so that we don't load the same or
 * older version of the image.
 *
 * An error in this function is fatal, ie we cannot reco ver
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

  return true;

  /*
   *  Check that we read enough bytes
   */
  if ( data_read < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t) )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "received package is not fit len");))
    OTA_fatal_error(LED_OTA_FATAL);
  }

  /*
   *  Extract the data and print it out
   */
  memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "New firmware version: %s", new_app_info.version);))

  if ( esp_ota_get_partition_description(running_partition, &running_app_info) == ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Running firmware version: %s", running_app_info.version);))
  }

  last_invalid_app = esp_ota_get_last_invalid_partition();
  if ( last_invalid_app != NULL )
  {
    if ( esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK )
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Last invalid firmware version: %s", invalid_app_info.version);))
    }

    if ( memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0 )
    {
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "New version is the same as invalid version.");))
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Previously, there was an attempt to launch the firmware with %s version, but it failed.",
                                      invalid_app_info.version);))
      DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "The firmware has been rolled back to the previous version.");))
      OTA_fatal_error(LED_OTA_FATAL);
    }
  }

  if ( memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0 )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Current running version is the same as a new. We will not continue the update.");))
    OTA_fatal_error(LED_OTA_FATAL);
  }

  /*
   * Everything looks good
   */
  return true;
}

/*----------------------------------------------------------------
 *
 * @function: OTA_start()
 *
 * @brief:    Entry point for the OTA update
 *
 * @return: None
 *---------------------------------------------------------------
 *

 *
 *------------------------------------------------------------*/

void OTA_start(void)
{
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "OTA_start()");))

  uint8_t                sha_256[HASH_LEN] = {0};
  esp_partition_t        partition;
  const esp_partition_t *running = esp_ota_get_running_partition();
#if ( 0 )
  // get sha256 digest for the partition table
  partition.address = ESP_PARTITION_TABLE_OFFSET;
  partition.size    = ESP_PARTITION_TABLE_MAX_LEN;
  partition.type    = ESP_PARTITION_TYPE_DATA;
  esp_partition_get_sha256(&partition, sha_256);
  print_sha256(sha_256, "SHA-256 for the partition table: ");

  // get sha256 digest for bootloader
  partition.address = ESP_BOOTLOADER_OFFSET;
  partition.size    = ESP_PARTITION_TABLE_OFFSET;
  partition.type    = ESP_PARTITION_TYPE_APP;
  esp_partition_get_sha256(&partition, sha_256);
  print_sha256(sha_256, "SHA-256 for bootloader: ");

  // get sha256 digest for running partition
  if ( running != NULL )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Running partition not available");))
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
  }
#endif

  esp_ota_img_states_t ota_state;
  if ( esp_ota_get_state_partition(running, &ota_state) == ESP_OK )
  {
    if ( ota_state == ESP_OTA_IMG_PENDING_VERIFY )
    {
      bool diagnostic_is_ok = false; // Keep it happy until we find a problem
      if ( diagnostic_is_ok )
      {
        DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Diagnostics completed successfully! Continuing execution ...");))
        esp_ota_mark_app_valid_cancel_rollback();
      }
      else
      {
        DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Diagnostics failed! Start rollback to the previous version ...");))
        esp_ota_mark_app_invalid_rollback_and_reboot();
      }
    }
  }

#if ( 0 )
  // Initialize NVS.
  esp_err_t err = nvs_flash_init();
  if ( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND )
  {
    // OTA app partition table has a smaller NVS partition size than the non-OTA
    // partition table. This size mismatch may cause NVS initialization to fail.
    // If this happens, we erase NVS partition and initialize NVS again.
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
   * Read "Establishing Wi-Fi or Ethernet Connection" section in
   * examples/protocols/README.md for more information about this function.
   */
  ESP_ERROR_CHECK(example_connect());

#if CONFIG_EXAMPLE_CONNECT_WIFI
  /* Ensure to disable any WiFi power save mode, this allows best throughput
   * and hence timings for overall OTA operation.
   */
  esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#endif

  OTA_load();
  return;
}

/*----------------------------------------------------------------
 *
 * @function: rollback()
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
}

/*----------------------------------------------------------------
 *
 * @function: print_sha256()
 *
 * @brief:  Display the 256 bit sha for the memory
 *
 * @return: None
 *---------------------------------------------------------------
 *
 * Convert the hash into a text string and print the resuls
 *
 *------------------------------------------------------------*/
static void print_sha256(const uint8_t *image_hash, // Pointer to the hash
                         const char    *lable)         // Lable to describe the hash
{
  int  i;
  char hash_print[HASH_LEN * 2 + 1];
  /*
   *  Loop and convert the has to text
   */
  for ( i = 0; i < HASH_LEN; ++i )
  {
    sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
  }
  hash_print[HASH_LEN * 2] = 0; // Null terminate

  /*
   * Print the results and return
   */
  DLT(DLT_HTTP, SEND(ALL, sprintf("SHA256 %s:%s", hash_print, lable);))

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

  printf("\r\nBoot Partition");
  if ( boot_partition != NULL )
  {
    printf("\r\nType:%s  Subtype:%d", partition_type[boot_partition->type], boot_partition->subtype);
    printf("\r\nAddress: 0X%lX  Size 0x%lX", boot_partition->address, boot_partition->size);
    printf("\r\nLable: %s", boot_partition->label);
    printf("\r\n");
  }
  else
  {
    printf("\r\nNot available");
    printf("\r\n");
  }

  printf("\r\nRunning Partition");
  if ( running_partition != NULL )
  {
    printf("\r\nType:%s  Subtype:%d", partition_type[running_partition->type], running_partition->subtype);
    printf("\r\nAddress: 0X%lX Size :0x%lX", running_partition->address, running_partition->size);
    printf("\r\nLable: %s", running_partition->label);
    printf("\r\n");
  }
  else
  {
    printf("\r\nNot available");
    printf("\r\n");
  }

  printf("\r\nUpdate Partition");
  if ( update_partition != NULL )
  {
    printf("\r\nType:%s  Subtype:%d", partition_type[update_partition->type], update_partition->subtype);
    printf("\r\nAddress: 0X%lX  Size 0x%lX", update_partition->address, update_partition->size);
    printf("\r\nLable: %s", update_partition->label);
    printf("\r\n");
  }
  else
  {
    printf("\r\nNot available");
    printf("\r\n");
  }

  printf(_DONE_);
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
static void OTA_fatal_error(char *LED_status)
{
  set_status_LED(LED_OTA_FAILED_LOAD);

  DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Exiting OTA task due to fatal error...");))

  while ( 1 )
    continue;
}
