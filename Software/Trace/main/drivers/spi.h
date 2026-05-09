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
esp_err_t spi_write(uint8_t gpio_spi_cs, uint8_t *data, size_t length); // Write to SPI device
esp_err_t spi_read(uint8_t gpio_spi_cs, uint8_t *data, size_t length);  // Read from SPI device

#endif                                                                  // _SPI_H_
