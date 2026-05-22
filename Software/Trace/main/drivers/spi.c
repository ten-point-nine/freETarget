/*************************************************************************
 *
 * file: spi.c
 *
 * description:  Simple SPI driver for trace
 *
 **************************************************************************
 *
 * This file manges the SPI driver
 *
 * See:
 *
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi.html
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html
 *
 ***************************************************************************/
#include <stdio.h>
#include "string.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "trace.h"
#include "diag_tools.h"
#include "spi.h"

/*
 * Definitions
 */

/*
 * Variables
 */

/*********************************************************************
 *
 * @function: spi_init
 *
 * @brief:    Initialize the control
 *
 * @return:   Success if it worked
 *
 *********************************************************************
 *
 * Setup the registers for the spi bus
 *
 ********************************************************************/
esp_err_t spi_init(unsigned int gpio_SCLK, unsigned int gpio_MISO, unsigned int gpio_MOSI)
{
  spi_bus_config_t spi_config;
  esp_err_t        ret;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "spi_init()");))

  memset(&spi_config, 0, sizeof(spi_config)); // Clear the configuration structure
  spi_config.mosi_io_num     = gpio_MOSI;
  spi_config.miso_io_num     = gpio_MISO;
  spi_config.sclk_io_num     = gpio_SCLK,
  spi_config.quadwp_io_num   = -1;            // QSPI not used
  spi_config.quadhd_io_num   = -1;            // QSPI not used
  spi_config.max_transfer_sz = WATERMARK + 32;          // Default is 4094, but can be set to a larger value if needed
  spi_config.flags           = 0;             // No special flags

  ret = spi_bus_initialize(SPI2_HOST, &spi_config, SPI_DMA_CH_AUTO);

  if ( ret == ESP_OK )
  {
    DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "spi initialized successfully");))
  }
  else
  {
    DLT(DLT_CRITICAL, SEND(ALL, sprintf(_xs, "Failed to initialize spi: %s", esp_err_to_name(ret));))
  }

  return ret;
}
