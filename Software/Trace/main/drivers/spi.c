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
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_flash/index.html
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

  spi_config.mosi_io_num     = gpio_MOSI;
  spi_config.miso_io_num     = gpio_MISO;
  spi_config.sclk_io_num     = gpio_SCLK,
  spi_config.quadwp_io_num   = -1;  // QSPI not used
  spi_config.quadhd_io_num   = -1;  // QSPI not used
  spi_config.max_transfer_sz = 256; // Default is 4094, but can be set to a larger value if needed

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

/*********************************************************************
 *
 * @function: spi_read
 *
 * @brief:    Read a sequence of bytes from an spi registers
 *
 * @return:   Read bytes from spi and save to memory
 *
 *********************************************************************
 *
 * The spi transfer is set up and executed
 *
 ********************************************************************/

esp_err_t spi_read(spi_device_handle_t handle,     // Pointer to device handle
                   uint8_t             address,    // Register address to read from
                   uint8_t            *tx_data,    // Buffer to be send
                   uint16_t            tx_length,  // Length of tx_data
                   uint8_t            *rx_data,    // Buffer to be read
                   uint16_t            rx_length   // Number of bytes to be read
)
{
  esp_err_t         ret;
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof(transaction));    // Clear the transaction structure
  transaction.addr      = address;                 // Register address to read from
  transaction.tx_buffer = tx_data;                 // Buffer to send the command and any data
  transaction.length    = 0;                       // Transmit length in bits
  transaction.rx_buffer = rx_data;                 // Buffer to receive the data
  transaction.rxlength  = rx_length * 8;           // Receive length in bits
  transaction.flags = SPI_TRANS_USE_RXDATA;        // Indicate that this is a read operation

  ret = spi_device_transmit(handle, &transaction); // Transmit the transaction

  return ret;
}

/*********************************************************************
 *
 * @function: spi_write
 *
 * @brief:    Write a sequence of bytes from an spi registers
 *
 * @return:   Read bytes from spi and save to memory
 *
 *********************************************************************
 *
 * The spi transfer is set up and executed
 *
 ********************************************************************/
esp_err_t spi_write(spi_device_handle_t handle,  // Pointer to device handle
                    uint8_t             address, // Register address to write to
                    uint8_t            *data,    // Buffer to be write
                    size_t              length   // Number of bytes to write
)
{
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));  // Clear the transaction structure
  transaction.addr      = address;               // Register address to write to
  transaction.length    = length * 8;            // Length in bits
  transaction.tx_buffer = data;                  // Buffer to send the data
  transaction.flags     = SPI_TRANS_USE_TXDATA;  // Indicate that this is a write operation

  spi_device_transmit(handle, &transaction);     // Transmit the transaction

  return ESP_OK;
}
