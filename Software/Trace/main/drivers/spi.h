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

#endif // _SPI_H_
