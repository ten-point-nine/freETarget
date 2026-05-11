/*----------------------------------------------------------------
 *
 * spi.h
 *
 * Header file for SPI functions
 *
 *---------------------------------------------------------------*/
#ifndef _SPI_H_
#define _SPI_H_

/*
 * Global functions
 */
esp_err_t spi_init(unsigned int gpio_SCLK, unsigned int gpio_MISO, unsigned int gpio_MOSI);
esp_err_t spi_write(spi_device_handle_t handle, uint8_t address, uint8_t *data, size_t length); // Write to SPI device
esp_err_t spi_read(spi_device_handle_t handle, uint8_t address, uint8_t *tx_data, uint16_t tx_length, uint8_t *rx_data, uint16_t rx_length);

#endif                                                                                          // _SPI_H_
