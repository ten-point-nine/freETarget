/******************************************************************************
 *
 * BlueTooth_spp_init.c
 *
 * BlueTooth SPP (Serial Port Protocol) for FreeETarget
 *
 ******************************************************************************
 *
 * BlueTooth offers a wireless serial port for tablets:
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_bt_device.html
 * https://github.com/espressif/esp-idf/tree/v5.4/examples/bluetooth/bluedroid/classic_bt/bt_spp_vfs_acceptor
 *
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

#include "esp_private/periph_ctrl.h" // for enabling UHCI module, remove it after UHCI driver is released
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/lldesc.h"
#include "esp_private/gdma.h"
#include "hal/uhci_ll.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "C:\Users\allan\esp\v5.3.1\esp-idf\components\bt\host\bluedroid\api\include\api\esp_spp_api.h"
#include "C:\Users\allan\esp\v5.3.1\esp-idf\components\bt\host\bluedroid\api\include\api\esp_bt_main.h"
#include "C:\Users\allan\esp\v5.3.1\esp-idf\components\bt\host\bluedroid\api\include\api\esp_gap_bt_api.h"
#include "C:\Users\allan\esp\v5.3.1\esp-idf\components\bt\host\bluedroid\api\include\api\esp_bt_device.h"
#include "C:\Users\allan\esp\v5.3.1\esp-idf\components\bt\host\bluedroid\api\include\api\esp_gap_bt_api.h"

#include "BlueTooth_spp.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"
#include "helpers.h"

#include "esp_vfs.h"
#include "sys/unistd.h"

#define SPP_SERVER_NAME "SPP_SERVER"

static char                 local_device_name[SMALL_STRING];
static const esp_spp_sec_t  sec_mask   = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

#define SPP_DATA_LEN 100

/*
 *  Local Functions
 */
static char *bda2str(uint8_t *bda, char *str, size_t size); // Bluetooth Device Address to String
static void  esp_spp_stack_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
static void  spp_read_handle(void *param);
static void  esp_spp_cb(uint16_t e, void *p);
static void  esp_spp_stack_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void         esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/*****************************************************************************
 *
 * @function: BlueTooth_SPP_init()
 *
 * @brief:    Initialize the BlueTooth Interface
 *
 * @return:   None
 *
 ******************************************************************************
 *

 *
 *******************************************************************************/
void BlueTooth_SPP_init(void)
{
  char      bda_str[18] = {0};
  esp_err_t ret;
  target_name(local_device_name);

#if ( 0 )
  esp_err_t ret = nvs_flash_init();
  if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND )
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
#endif

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if ( esp_bt_controller_init(&bt_cfg) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to install BlueTooth controller");))
    return;
  }

  if ( esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to enable BlueTooth controller");))
    return;
  }

  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();

#if ( CONFIG_EXAMPLE_SSP_ENABLED == false )
  bluedroid_cfg.ssp_en = false;
#endif
  if ( (ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Initialize bluedroid failed: %s", esp_err_to_name(ret));))
    return;
  }

  if ( esp_bluedroid_enable() != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Failed to enable bluedroid");))
    return;
  }

  if ( esp_bt_gap_register_callback(esp_bt_gap_cb) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "gap register failed");))
    return;
  }

  if ( esp_spp_register_callback(esp_spp_stack_cb) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "SPP register failed");))
    return;
  }

  spp_task_task_start_up();

  esp_spp_cfg_t bt_spp_cfg = BT_SPP_DEFAULT_CONFIG();
  if ( esp_spp_enhanced_init(&bt_spp_cfg) != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "SPP init failed");))
    return;
  }

#if ( CONFIG_EXAMPLE_SSP_ENABLED == true )
  /* Set default parameters for Secure Simple Pairing */
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t   iocap      = ESP_BT_IO_CAP_IO;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

  /*
   * Set default parameters for Legacy Pairing
   * Use variable pin, input pin code when pairing
   */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
  esp_bt_pin_code_t pin_code;
  esp_bt_gap_set_pin(pin_type, 0, pin_code);
  DLT(DLT_INFO, SEND(sprintf(_xs, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));))
}

static char *bda2str(uint8_t *bda, char *str, size_t size)
{
  if ( bda == NULL || str == NULL || size < 18 )
  {
    return NULL;
  }

  uint8_t *p = bda;
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
  return str;
}

static void spp_read_handle(void *param)
{
  int      size     = 0;
  int      fd       = (int)param;
  uint8_t *spp_data = NULL;

  spp_data = malloc(SPP_DATA_LEN);
  if ( !spp_data )
  {
    DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "malloc spp_data failed, fd:%d", fd);))
    goto done;
  }

  do
  {
    /* The frequency of calling this function also limits the speed at which the peer device can send data. */
    size = read(fd, spp_data, SPP_DATA_LEN);
    if ( size < 0 )
    {
      break;
    }
    else if ( size == 0 )
    {
      /* There is no data, retry after 500 ms */
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    else
    {
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "fd = %d data_len = %d", fd, size);))
      /* To avoid task watchdog */
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  } while ( 1 );
done:
  if ( spp_data )
  {
    free(spp_data);
  }
  spp_wr_task_shut_down();
}

static void esp_spp_cb(uint16_t e, void *p)
{
  esp_spp_cb_event_t  event       = e;
  esp_spp_cb_param_t *param       = p;
  char                bda_str[18] = {0};

  switch ( event )
  {
    case ESP_SPP_INIT_EVT:
      if ( param->init.status == ESP_SPP_SUCCESS )
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_INIT_EVT");))
        /* Enable SPP VFS mode */
        esp_spp_vfs_register();
      }
      else
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_INIT_EVT status:%d", param->init.status);))
      }
      break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_DISCOVERY_COMP_EVT");))
      break;
    case ESP_SPP_OPEN_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_OPEN_EVT");))
      break;
    case ESP_SPP_CLOSE_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_CLOSE_EVT status:%d handle:%" PRIu32 " close_by_remote:%d", param->close.status,
                                          param->close.handle, param->close.async);))

      break;
    case ESP_SPP_START_EVT:
      if ( param->start.status == ESP_SPP_SUCCESS )
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_START_EVT handle:%" PRIu32 " sec_id:%d scn:%d", param->start.handle,
                                            param->start.sec_id, param->start.scn);))
        esp_bt_gap_set_device_name(local_device_name);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
      }
      else
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_START_EVT status:%d", param->start.status);))
      }
      break;
    case ESP_SPP_CL_INIT_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_CL_INIT_EVT");))
      break;
    case ESP_SPP_SRV_OPEN_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%" PRIu32 ", rem_bda:[%s]", param->srv_open.status,
                                          param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));))
      if ( param->srv_open.status == ESP_SPP_SUCCESS )
      {
        spp_wr_task_start_up(spp_read_handle, param->srv_open.fd);
      }
      break;
    case ESP_SPP_VFS_REGISTER_EVT:
      if ( param->vfs_register.status == ESP_SPP_SUCCESS )
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_VFS_REGISTER_EVT");))
        esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
      }
      else
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_SPP_VFS_REGISTER_EVT status:%d", param->vfs_register.status);))
      }
      break;
    default:
      break;
  }
}

static void esp_spp_stack_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  /* To avoid stucking Bluetooth stack, we dispatch the SPP callback event to the other lower priority task */
  spp_task_work_dispatch(esp_spp_cb, event, param, sizeof(esp_spp_cb_param_t), NULL);
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  switch ( event )
  {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
      if ( param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS )
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "authentication success: %s", param->auth_cmpl.device_name);))
      }
      else
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "authentication failed, status:%d", param->auth_cmpl.stat);))
      }
      break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:
    {
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);))
      if ( param->pin_req.min_16_digit )
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "Input pin code: 0000 0000 0000 0000");))
        esp_bt_pin_code_t pin_code = {0};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
      }
      else
      {
        DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "Input pin code: 1234");))
        esp_bt_pin_code_t pin_code;
        pin_code[0] = '1';
        pin_code[1] = '2';
        pin_code[2] = '3';
        pin_code[3] = '4';
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
      }
      break;
    }

#if ( CONFIG_EXAMPLE_SSP_ENABLED == true )
    case ESP_BT_GAP_CFM_REQ_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %" PRIu32, param->cfm_req.num_val);
                                  esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);))
      break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%" PRIu32, param->key_notif.passkey);))
      break;
    case ESP_BT_GAP_KEY_REQ_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");))
      break;
#endif

    case ESP_BT_GAP_MODE_CHG_EVT:
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);))
      break;

    default:
    {
      DLT(DLT_COMMUNICATION, SEND(sprintf(_xs, "event: %d", event);))
      break;
    }
  }
  return;
}
