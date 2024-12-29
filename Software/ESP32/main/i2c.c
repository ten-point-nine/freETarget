/*************************************************************************
 *
 * file: i2c.c
 *
 * description:  Simple I2C driver for freeETarget
 *
 **************************************************************************
 *
 * This file manges the I2C driver
 *
 * See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
 *
 ***************************************************************************/
#include <stdio.h>
#include "driver/i2c.h"

/*
 * Definitions
 */
#define I2C_MASTER_NUM            0 /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ        400000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0      /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0      /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS     1000

/*
 * Variables
 */
i2c_config_t i2c_configuration = {
    // Do not make const
    .mode             = I2C_MODE_MASTER,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_MASTER_FREQ_HZ,
};

/*********************************************************************
 *
 * @function: i2c_init
 *
 * @brief:    Initialize the control
 *
 * @return:   Success if it worked
 *
 *********************************************************************
 *
 * Setup the registers for the I2C bus
 *
 ********************************************************************/
esp_err_t i2c_init(int i2c_gpio_SDA, // GPIO SPI belongs to
                   int i2c_gpio_SCL  // GPIO SPI belongs to
)
{
  int i2c_master_port = I2C_MASTER_NUM;

  i2c_configuration.sda_io_num = i2c_gpio_SDA;
  i2c_configuration.scl_io_num = i2c_gpio_SCL;
  i2c_configuration.mode       = I2C_MODE_MASTER, /*!< I2C master mode */
      i2c_param_config(i2c_master_port, &i2c_configuration);

  return i2c_driver_install(i2c_master_port, i2c_configuration.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/*********************************************************************
 *
 * @function: i2c_read
 *
 * @brief:    Read a sequence of bytes from an I2C registers
 *
 * @return:   Read bytes from I2C and save to memory
 *
 *********************************************************************
 *
 * The I2C transfer is set up and executed
 *
 ********************************************************************/

esp_err_t i2c_read(uint8_t  device_addr, // I2C Device Address
                   uint8_t *data,        // Buffer to be read
                   size_t   length       // Number of bytes to be read
)
{
  return i2c_master_read_from_device(I2C_MASTER_NUM, device_addr, data, length, 10 * I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/*********************************************************************
 *
 * @function: i2c_write
 *
 * @brief:    Write a sequence of bytes from an I2C registers
 *
 * @return:   Read bytes from I2C and save to memory
 *
 *********************************************************************
 *
 * The I2C transfer is set up and executed
 *
 ********************************************************************/
esp_err_t i2c_write(uint8_t  device_addr, // Device Address
                    uint8_t *data,        // Data to write
                    size_t   length       // Number of bytes to write
)
{
  return i2c_master_write_to_device(I2C_MASTER_NUM, device_addr, data, length, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}
