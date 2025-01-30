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

#include "esp_private/periph_ctrl.h" // for enabling UHCI module, remove it after UHCI driver is released
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/lldesc.h"
#include "esp_private/gdma.h"
#include "hal/uhci_ll.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_log.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"

/*
 * Operation functions for HCI UART Transport Layer
 */
void        uhci_uart_install(void); // Install the BlueTooth UART
static bool hci_uart_tl_init(void);
static void hci_uart_tl_deinit(void);
static void hci_uart_tl_recv_async(uint8_t *buf, uint32_t size, esp_bt_hci_tl_callback_t callback, void *arg);
static void hci_uart_tl_send_async(uint8_t *buf, uint32_t size, esp_bt_hci_tl_callback_t callback, void *arg);
static void hci_uart_tl_flow_on(void);
static bool hci_uart_tl_flow_off(void);
static void hci_uart_tl_finish_transfers(void);

/*
 * Variables
 */
static esp_bt_hci_tl_t s_hci_uart_tl_funcs = {
    ._magic            = ESP_BT_HCI_TL_MAGIC_VALUE,
    ._version          = ESP_BT_HCI_TL_VERSION,
    ._reserved         = 0,
    ._open             = (void *)hci_uart_tl_init,
    ._close            = (void *)hci_uart_tl_deinit,
    ._finish_transfers = (void *)hci_uart_tl_finish_transfers,
    ._recv             = (void *)hci_uart_tl_recv_async,
    ._send             = (void *)hci_uart_tl_send_async,
    ._flow_on          = (void *)hci_uart_tl_flow_on,
    ._flow_off         = (void *)hci_uart_tl_flow_off,
};

/*
 * Private Functions
 */

/*
 * Definitions
 */

#define UART_HCI_NUM (1)

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

static char *esp_status[] = {"ESP_BT_CONTROLLER_STATUS_IDLE", "ESP_BT_CONTROLLER_STATUS_INITED", "ESP_BT_CONTROLLER_STATUS_ENABLED",
                             "ESP_BT_CONTROLLER_STATUS_NUM"};

void BlueTooth_init(void)
{
  DLT(DLT_INFO, SEND(sprintf(_xs, "BlueTooth_init()");))

  esp_err_t                  ret;
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.hci_tl_funcs               = &s_hci_uart_tl_funcs;

  /*
   * Install the BlueTooth UART
   */
  ret = esp_bt_controller_init(&bt_cfg);
  if ( ret != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));))
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if ( ret != ESP_OK )
  {
    DLT(DLT_CRITICAL, SEND(sprintf(_xs, "Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));))
    return;
  }

  /*
   * Check it
   */
  DLT(DLT_INFO, SEND(sprintf(_xs, "BlueTooth status: %s", esp_status[esp_bt_controller_get_status()]);))

  uhci_uart_install();

  /*
   *  All done
   */
  return;
}
#if ( 0 )
struct uart_env_tag
{
  struct uart_txrxchannel tx;
  struct uart_txrxchannel rx;
};

struct uart_env_tag uart_env;
#endif
static volatile uhci_dev_t  *s_uhci_hw = &UHCI0;
static gdma_channel_handle_t s_rx_channel;
static gdma_channel_handle_t s_tx_channel;

/*****************************************************************************
 *
 * @function: BlueTooth Callbacks
 *
 * @brief:    Callbacks used by the BlueTooth Controller
 *
 * @return:   None
 *
 ******************************************************************************
 *

 *
 *******************************************************************************/

/*
 * ._open
 */
static bool hci_uart_tl_init(void)
{
  return true;
}

/*
 * ._close
 */
static void hci_uart_tl_deinit(void)
{
}

/*
 * ._recv
 */
static IRAM_ATTR void hci_uart_tl_recv_async(uint8_t *buf, uint32_t size, esp_bt_hci_tl_callback_t callback, void *arg)
{
}

/*
 * ._send
 */
static IRAM_ATTR void hci_uart_tl_send_async(uint8_t *buf, uint32_t size, esp_bt_hci_tl_callback_t callback, void *arg)
{
}

/*
 * ._flow_on
 */
static void hci_uart_tl_flow_on(void)
{
}

/*
 * ._flow_off
 */
static bool hci_uart_tl_flow_off(void)
{
  return true;
}
/*
 * ._finish_transfers
 */
static void hci_uart_tl_finish_transfers(void)
{
}

static IRAM_ATTR bool hci_uart_tl_rx_eof_callback(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data)
{

  return true;
}

static IRAM_ATTR bool hci_uart_tl_tx_eof_callback(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data)
{

  return true;
}

void uhci_uart_install(void)
{
  periph_module_enable(PERIPH_UHCI0_MODULE);
  periph_module_reset(PERIPH_UHCI0_MODULE);

  periph_module_enable(PERIPH_UART1_MODULE);
  periph_module_reset(PERIPH_UART1_MODULE);

  // configure UHCI
  uhci_ll_init(s_uhci_hw);
  uhci_ll_set_eof_mode(s_uhci_hw, UHCI_RX_LEN_EOF);
  // disable software flow control
  s_uhci_hw->escape_conf.val = 0;
  uhci_ll_attach_uart_port(s_uhci_hw, 1);
}
/*****************************************************************************
 *
 * @function: BLueTooth_send_test_message()
 *
 * @brief:    Send a message over BlueTooth
 *
 * @return:   None
 *
 ******************************************************************************
 *
 *
 *******************************************************************************/
#define BT_TEST_MESSAGE "\r\nHello World BlueTooth\r\n"

void BlueTooth_send_test_message(void)
{
  /*
   *  Test to see if we can send anything
   */
  if ( esp_vhci_host_check_send_available() != true )
  {
    printf("\r\nBlueTooth send available failed");
    return;
  }

  /*
   *  Yes, we can send, Hello World
   */
  esp_vhci_host_send_packet((uint8_t *)BT_TEST_MESSAGE, sizeof(BT_TEST_MESSAGE));

  /*
   * Send message complete
   */
  printf("\r\nBlueTooth send message complete");
  return;
}