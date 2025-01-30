/******************************************************************************
 *
 * BlueTooth_spp.c
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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "spp_task.h"

static void spp_task_task_handler(void *arg);
static bool spp_task_send_msg(spp_task_msg_t *msg);
static void spp_task_work_dispatched(spp_task_msg_t *msg);

static QueueHandle_t spp_task_task_queue  = NULL;
static TaskHandle_t  spp_task_task_handle = NULL;

bool spp_task_work_dispatch(spp_task_cb_t p_cback, uint16_t event, void *p_params, int param_len, spp_task_copy_cb_t p_copy_cback)
{
  ESP_LOGD(SPP_TASK_TAG, "%s event 0x%x, param len %d", __func__, event, param_len);

  spp_task_msg_t msg;
  memset(&msg, 0, sizeof(spp_task_msg_t));

  msg.sig   = SPP_TASK_SIG_WORK_DISPATCH;
  msg.event = event;
  msg.cb    = p_cback;

  if ( param_len == 0 )
  {
    return spp_task_send_msg(&msg);
  }
  else if ( p_params && param_len > 0 )
  {
    if ( (msg.param = malloc(param_len)) != NULL )
    {
      memcpy(msg.param, p_params, param_len);
      /* check if caller has provided a copy callback to do the deep copy */
      if ( p_copy_cback )
      {
        p_copy_cback(&msg, msg.param, p_params);
      }
      return spp_task_send_msg(&msg);
    }
  }

  return false;
}

static bool spp_task_send_msg(spp_task_msg_t *msg)
{
  if ( msg == NULL )
  {
    return false;
  }

  if ( xQueueSend(spp_task_task_queue, msg, 10 / portTICK_PERIOD_MS) != pdTRUE )
  {
    ESP_LOGE(SPP_TASK_TAG, "%s xQueue send failed", __func__);
    return false;
  }
  return true;
}

static void spp_task_work_dispatched(spp_task_msg_t *msg)
{
  if ( msg->cb )
  {
    msg->cb(msg->event, msg->param);
  }
}

static void spp_task_task_handler(void *arg)
{
  spp_task_msg_t msg;
  for ( ;; )
  {
    if ( pdTRUE == xQueueReceive(spp_task_task_queue, &msg, (TickType_t)portMAX_DELAY) )
    {
      ESP_LOGD(SPP_TASK_TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig, msg.event);
      switch ( msg.sig )
      {
        case SPP_TASK_SIG_WORK_DISPATCH:
          spp_task_work_dispatched(&msg);
          break;
        default:
          ESP_LOGW(SPP_TASK_TAG, "%s, unhandled sig: %d", __func__, msg.sig);
          break;
      }

      if ( msg.param )
      {
        free(msg.param);
      }
    }
  }
}

void spp_task_task_start_up(void)
{
  spp_task_task_queue = xQueueCreate(10, sizeof(spp_task_msg_t));
  xTaskCreate(spp_task_task_handler, "SPPAppT", 2048, NULL, 10, &spp_task_task_handle);
  return;
}

void spp_task_task_shut_down(void)
{
  if ( spp_task_task_handle )
  {
    vTaskDelete(spp_task_task_handle);
    spp_task_task_handle = NULL;
  }
  if ( spp_task_task_queue )
  {
    vQueueDelete(spp_task_task_queue);
    spp_task_task_queue = NULL;
  }
}

void spp_wr_task_start_up(spp_wr_task_cb_t p_cback, int fd)
{
  xTaskCreate(p_cback, "write_read", 4096, (void *)fd, 5, NULL);
}
void spp_wr_task_shut_down(void)
{
  vTaskDelete(NULL);
}
