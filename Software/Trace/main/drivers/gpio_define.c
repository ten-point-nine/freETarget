/*************************************************************************
 *
 * file: gpio_init.c
 *
 * description:  Initialize all of the GPIO pins
 *
 **************************************************************************
 *
 * This file sets up the configuration of ALL of the GPIOs in one place
 *
 * It contains definitions for all gpios even if they are not used in thie
 * particular implemenation.  The final table gpio_table contains a list of
 * the GPIOs and a link to the initialization table used for that particular
 * pin
 *
 ***************************************************************************/

#include "stdbool.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "gpio_types.h"

#include "trace.h"
#include "board_assembly.h"
#include "diag_tools.h"
#include "gpio_define.h"
#include "serial_io.h"
#include "i2c.h"

#define BOARD_REVISION 0 // Board revision via GPIO define entry #0
/*
 *  Digital IO definitions
 */
const DIO_struct_t dio00 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio01 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio02 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio03 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio04 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio05 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio06 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio07 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_OUTPUT, .initial_value = 0};  // Mode and Initial Value
const DIO_struct_t dio08 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio09 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio10 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio18 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio19 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value

const DIO_struct_t dio20 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio21 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 1}; // Mode and Initial Value

/*
 *  I2C COntrol.  GPIO explicitly filled in here
 */
I2C_struct_t i2c = {I2C_PORT, GPIO_NUM_0, GPIO_NUM_1};

/*
 *  GPIO Usage
 *
 *  This table contains the use of each of the individual pins
 *
 */

const gpio_struct_t gpio_table[] = {
    //   Name      Number       Assigned   Used by
    {"SDA",         GPIO_NUM_0,  (void *)&i2c,   COMMON}, // I2C SDA
    {"SCL",         GPIO_NUM_1,  NULL,           COMMON}, // I2C SCL

    {"BD_REV",      GPIO_NUM_2,  (void *)&dio02, COMMON}, // BD_REV
    {"TP1",         GPIO_NUM_3,  (void *)&dio03, COMMON}, // Spare test point
    {"TP2",         GPIO_NUM_4,  (void *)&dio04, COMMON}, // Spare test point
    {"INT",         GPIO_NUM_5,  (void *)&dio05, COMMON}, // Interrupt from Gyro/Accel
    {"LED",         GPIO_NUM_6,  (void *)&dio06, COMMON}, // Status LED, Active LOW
    {"PUSH_BUTTON", GPIO_NUM_7,  (void *)&dio07, COMMON}, // Setup button, Active LOW
    {"ROM_MESSAGE", GPIO_NUM_8,  NULL,           COMMON}, // Force ROM messages on the serial port, Active HIGH
    {"BOOT",        GPIO_NUM_9,  NULL,           COMMON}, // Stay in boot block
    {"NOT USED",    GPIO_NUM_10, NULL,           COMMON}, //
    {"NOT USED",    GPIO_NUM_19, NULL,           COMMON}, //
    {"TXD",         GPIO_NUM_21, NULL,           COMMON}, // UART TXD
    {"RXD",         GPIO_NUM_20, NULL,           COMMON}, // UART RXD
    {0,             0,           0,              0     }
};

/*
 *  Variables
 */
const static gpio_type_t gpio_order[] = // Order in which to program devices
    {
        DIGITAL_IO_OUT,                 // GPIO is used for Digital Output
        I2C_PORT,                       // GPIO is used as a i2c port
        DIGITAL_IO_IN,                  // GPIO is used for Digital Input
        0                               // End sentinel
};

/*************************************************************************
 *
 * function: gpio_init()
 *
 * description:  Initialize the GPIO states
 *
 * @return:  Nope
 *
 **************************************************************************
 *
 * The gpio_table is used to program the gpio hardware.
 *
 * The process is in two steps.
 *
 *  1 - Do the outputs and set the output to a known state
 *  2 - Do the inputs assuming that the outputs have put the board into a
 *      good state.
 *
 ***************************************************************************/
void gpio_init(void)
{
  int i;

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "gpio_init()");))

  gpio_install_isr_service(0); // Install the ISR service for later
  /*
   *  Loop and setup the GPIO outputs in a particular order
   */
  i = 0;
  while ( gpio_order[i] != 0 )       // Program the IO in order
  {
    gpio_init_single(gpio_order[i]); // Program the single GPIO matching this order
    i++;
  }

  /*
   *  All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "GPIO complete");))
  vTaskDelay(10);
  return;
}

/*
 * Carry out the steps to initialize a single GPIO line
 */
void gpio_init_single(unsigned int type)                                        // What type of GPIO are we programming?
{
  int i;                                                                        // Iteration counter
                                                                                /*
                                                                                 *  Loop throught the table looking for a match and program that
                                                                                 */

  i = 0;
  while ( gpio_table[i].gpio_name != 0 )                                        // Look for the end of the table
  {
    if ( (gpio_table[i].gpio_uses != NULL)                                      // Is the GPIO used?
         && (((const DIO_struct_t *)(gpio_table[i].gpio_uses))->type == type) ) //
    {
      switch ( ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->type )
      {
        default:
          DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "GPIO not found: %d", i);))
          break;

        case DIGITAL_IO_IN:
          DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Digital input: (%d) %s", gpio_table[i].gpio_number, gpio_table[i].gpio_name);))
          gpio_set_direction(gpio_table[i].gpio_number, GPIO_MODE_INPUT);
          gpio_set_pull_mode(gpio_table[i].gpio_number, GPIO_PULLUP_ONLY);

          if ( (gpio_isr_t)((const DIO_struct_t *)(gpio_table[i].gpio_uses))->callback != NULL )
          {
            gpio_intr_disable(gpio_table[i].gpio_number);
            gpio_set_intr_type(gpio_table[i].gpio_number, GPIO_INTR_POSEDGE); // Setup the interrupt handler
            gpio_isr_handler_add(gpio_table[i].gpio_number, (gpio_isr_t)((const DIO_struct_t *)(gpio_table[i].gpio_uses))->callback,
                                 NULL);                                       // Add in the ISR handler
          }
          break;

        case DIGITAL_IO_OUT:
          DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Digital output: (%d) %s = %d", gpio_table[i].gpio_number, gpio_table[i].gpio_name,
                                          ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->initial_value);))
          gpio_set_direction(gpio_table[i].gpio_number, ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->mode);
          gpio_set_pull_mode(gpio_table[i].gpio_number, GPIO_PULLUP_PULLDOWN);
          gpio_set_level(gpio_table[i].gpio_number, ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->initial_value);
          break;

        case I2C_PORT:
          DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "I2C: (%d) %s", gpio_table[i].gpio_number, gpio_table[i].gpio_name);))
          i2c_init(((I2C_struct_t *)(gpio_table[i].gpio_uses))->gpio_number_SDA,
                   ((I2C_struct_t *)(gpio_table[i].gpio_uses))->gpio_number_SCL);
          break;
      }
    }
    i++;
  }

  /*
   *  All done, return
   */
  return;
}
