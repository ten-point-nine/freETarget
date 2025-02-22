/*----------------------------------------------------------------
 *
 * pwm.j
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _I2C_H_
#define _I2C_H_

/*
 * Global functions
 */
esp_err_t i2c_init(int i2c_gpio_SDA, int i2c_gpio_SCL);            // GPIO I2C belongs to
esp_err_t i2c_write(uint8_t device, uint8_t *data, size_t length); // Write to the I2C device
esp_err_t i2c_read(uint8_t device, uint8_t *data, size_t length);  // Read from the I2C device

#endif
