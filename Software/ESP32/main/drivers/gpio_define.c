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
#include "adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "led_strip.h"
#include "led_strip_types.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "analog_io.h"
#include "gpio_define.h"
#include "serial_io.h"
#include "i2c.h"
#include "pwm.h"
#include "pcnt.h"

/*
 *  Digital IO definitions
 */
const DIO_struct_t dio00 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio01 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio02 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio03 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio04 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio05 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio06 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio07 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio08 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio09 = {
    .type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0, .callback = &east_hi_pcnt_isr_callback};  // Mode and Initial Value
const DIO_struct_t dio10 = {
    .type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0, .callback = &south_hi_pcnt_isr_callback}; // Mode and Initial Value
const DIO_struct_t dio11 = {
    .type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0, .callback = &west_hi_pcnt_isr_callback};  // Mode and Initial Value
const DIO_struct_t dio12 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 0};                // Mode and Initial Value
const DIO_struct_t dio13 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio14 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio15 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio16 = {
    .type = PCNT_HI, .mode = GPIO_MODE_INPUT, .initial_value = 0, .callback = &north_hi_pcnt_isr_callback};       // Mode and Initial Value
const DIO_struct_t dio17 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio18 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio19 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value

const DIO_struct_t dio20 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio21 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 1};                // Mode and Initial Value
const DIO_struct_t dio22 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio23 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio24 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio25 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio26 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio27 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio28 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio29 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value

const DIO_struct_t dio30 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio31 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio32 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio33 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio34 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Can only be input
const DIO_struct_t dio35 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Can only be input
const DIO_struct_t dio36 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Can only be input
const DIO_struct_t dio37 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio38 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};                  // Mode and Initial Value
const DIO_struct_t dio39 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Can only be input // AMB

const DIO_struct_t dio40 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio41 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 0}; // Mode and Initial Value
const DIO_struct_t dio42 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 1}; // Mode and Initial Value
const DIO_struct_t dio43 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 0}; // Mode and Initial Value
const DIO_struct_t dio44 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio45 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 0}; // Mode and Initial Value
const DIO_struct_t dio46 = {.type = DIGITAL_IO_IN, .mode = GPIO_MODE_INPUT, .initial_value = 0};   // Mode and Initial Value
const DIO_struct_t dio47 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 0}; // Mode and Initial Value
const DIO_struct_t dio48 = {.type = DIGITAL_IO_OUT, .mode = GPIO_MODE_OUTPUT, .initial_value = 1}; // Mode and Initial Value

/*
 *  Analog IO usage
 */
const ADC_struct_t adc1_ch0 = {.type = ANALOG_IO, .adc_channel = ADC(1, 0), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 0
const ADC_struct_t adc1_ch1 = {.type = ANALOG_IO, .adc_channel = ADC(1, 1), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 1
const ADC_struct_t adc1_ch2 = {.type = ANALOG_IO, .adc_channel = ADC(1, 2), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 2
const ADC_struct_t adc1_ch3 = {.type = ANALOG_IO, .adc_channel = ADC(1, 3), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 3
const ADC_struct_t adc1_ch4 = {.type = ANALOG_IO, .adc_channel = ADC(1, 4), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 4
const ADC_struct_t adc1_ch5 = {.type = ANALOG_IO, .adc_channel = ADC(1, 5), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 5
const ADC_struct_t adc1_ch6 = {.type = ANALOG_IO, .adc_channel = ADC(1, 6), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 6
const ADC_struct_t adc1_ch7 = {.type = ANALOG_IO, .adc_channel = ADC(1, 7), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 7
const ADC_struct_t adc1_ch8 = {.type = ANALOG_IO, .adc_channel = ADC(1, 8), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 9
const ADC_struct_t adc1_ch9 = {.type = ANALOG_IO, .adc_channel = ADC(1, 9), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 1, ADC 10

const ADC_struct_t adc2_ch0 = {.type = ANALOG_IO, .adc_channel = ADC(2, 0), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 0
const ADC_struct_t adc2_ch1 = {.type = ANALOG_IO, .adc_channel = ADC(2, 1), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 1
const ADC_struct_t adc2_ch2 = {.type = ANALOG_IO, .adc_channel = ADC(2, 2), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 2
const ADC_struct_t adc2_ch3 = {.type = ANALOG_IO, .adc_channel = ADC(2, 3), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 3
const ADC_struct_t adc2_ch4 = {.type = ANALOG_IO, .adc_channel = ADC(2, 4), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 4
const ADC_struct_t adc2_ch5 = {.type = ANALOG_IO, .adc_channel = ADC(2, 5), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 5
const ADC_struct_t adc2_ch6 = {.type = ANALOG_IO, .adc_channel = ADC(2, 6), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 6
const ADC_struct_t adc2_ch7 = {.type = ANALOG_IO, .adc_channel = ADC(2, 7), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 7
const ADC_struct_t adc2_ch8 = {.type = ANALOG_IO, .adc_channel = ADC(2, 8), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 8
const ADC_struct_t adc2_ch9 = {.type = ANALOG_IO, .adc_channel = ADC(2, 9), .adc_attenuation = ADC_ATTEN_DB_11}; // CHANNEL 2, ADC 9

/*
 *  PWM Control, GPIO filled in by GPIO definition above
 */
const PWM_struct_t pwm0 = {.pwm_channel = 0, .type = PWM_OUT};
const PWM_struct_t pwm1 = {.pwm_channel = 1, .type = PWM_OUT};
const PWM_struct_t pwm2 = {.pwm_channel = 2, .type = PWM_OUT};
const PWM_struct_t pwm3 = {.pwm_channel = 3, .type = PWM_OUT};

/*
 *  I2C COntrol.  GPIO explicitly filled in here
 */
I2C_struct_t i2c = {I2C_PORT, GPIO_NUM_14, GPIO_NUM_13};

/*
 *  PCNT.  8 for ESP32, 4 for ESP32-S3
 */
const PCNT_struct_t pcnt0 = {.type = PCNT, .pcnt_unit = 0, .pcnt_signal = GPIO_NUM_8, .pcnt_control = GPIO_NUM_5};
const PCNT_struct_t pcnt1 = {.type = PCNT, .pcnt_unit = 1, .pcnt_signal = GPIO_NUM_8, .pcnt_control = GPIO_NUM_6};
const PCNT_struct_t pcnt2 = {.type = PCNT, .pcnt_unit = 2, .pcnt_signal = GPIO_NUM_8, .pcnt_control = GPIO_NUM_7};
const PCNT_struct_t pcnt3 = {.type = PCNT, .pcnt_unit = 3, .pcnt_signal = GPIO_NUM_8, .pcnt_control = GPIO_NUM_15};
#if ( SOC_PCNT_UNITS_PER_GROUP != 4 )
const PCNT_struct_t pcnt4 = {.type = PCNT, .pcnt_unit = 4, .pcnt_signal = GPIO_NUM_8};
const PCNT_struct_t pcnt5 = {.type = PCNT, .pcnt_unit = 5, .pcnt_signal = GPIO_NUM_8};
const PCNT_struct_t pcnt6 = {.type = PCNT, .pcnt_unit = 6, .pcnt_signal = GPIO_NUM_8};
const PCNT_struct_t pcnt7 = {.type = PCNT, .pcnt_unit = 7, .pcnt_signal = GPIO_NUM_8};
#endif

/*
 *  LED Strip
 */
const static LED_strip_struct_t led_strip_config = {.type = LED_STRIP}; // 3 LEDs on the board

/*
 *  GPIO Usage
 *
 *  This table contains the use of each of the individual pins
 *
 */

const gpio_struct_t gpio_table[] = {
    //   Name      Number       Assigned
    {"BD_REV",       GPIO_NUM_4,  (void *)&adc1_ch3        }, // BD_REV
    {"ATX",          GPIO_NUM_17, (void *)&dio17           }, // ATX Initailize as input
    {"ARX",          GPIO_NUM_18, (void *)&dio18           }, // ARX and override later
    {"REF_CLK",      GPIO_NUM_8,  NULL                     }, // Re rence CLock Used by pcnt
    {"USB_D-",       GPIO_NUM_19, NULL                     }, // JTAG USB D-
    {"USB_D+",       GPIO_NUM_20, NULL                     }, // JTAG USB D+
    {"USB_JTAG",     GPIO_NUM_3,  NULL                     }, // JTAG Strap to 3V3
    {"ROM_MSG",      GPIO_NUM_46, NULL                     }, // Enable ROM messages

    {"RUN_EAST_HI",  GPIO_NUM_9,  (void *)&dio09           }, // RUN_EAST_HI Direct to GPIO
    {"RUN_SOUTH_HI", GPIO_NUM_10, (void *)&dio10           }, // RUN_SOUTH_HI
    {"RUN_WEST_HI",  GPIO_NUM_11, (void *)&dio11           }, // RUN_WEST_HI
    {"PAPER",        GPIO_NUM_12, (void *)&dio12           }, // PAPER  Drive
    {"V12_REF",      GPIO_NUM_1,  (void *)&adc1_ch0        }, // LED Feedback (Measure 12VDC)
    {"LED_PWM",      GPIO_NUM_2,  (void *)&pwm0            }, // LED_PWM
    {"TXD",          GPIO_NUM_43, NULL                     }, // UART Transmit   Initialized in serial_io_init
    {"RXD",          GPIO_NUM_44, NULL                     }, // UART Receive
    {"LDAC*",        GPIO_NUM_42, (void *)&dio42           }, // Load DAC*
    {"SPARE0",       GPIO_NUM_41, (void *)&dio41           }, //
    {"FACE_HALF",    GPIO_NUM_40, (void *)&dio40           }, // FACE Interrupt
    {"SPARE1",       GPIO_NUM_39, (void *)&dio39           }, //
    {"A",            GPIO_NUM_38, (void *)&dio38           }, // Auxilary Input A
    {"B",            GPIO_NUM_37, (void *)&dio37           }, // Auxilary Input B
    {"C",            GPIO_NUM_36, (void *)&dio36           }, // Auxilary Input C
    {"D",            GPIO_NUM_35, (void *)&dio35           }, // Auxilary Input D
    {"STATUS",       GPIO_NUM_45, (void *)&led_strip_config}, // Status LEDs
    {"OSC_CONTROL",  GPIO_NUM_48, (void *)&dio48           }, // Start / stop the 10MHz oscillator
    {"CLK_START*",   GPIO_NUM_47, (void *)&dio47           }, // Trigger the clocks for diagnostics
    {"STOP*",        GPIO_NUM_21, (void *)&dio21           }, // Stop the RUN signals
    {"SDA",          GPIO_NUM_14, (void *)&i2c             }, // SDA
    {"SCL",          GPIO_NUM_13, NULL                     }, // SCL
    {"RUN_NORTH_HI", GPIO_NUM_16, (void *)&dio16           }, // RUN_NORTH_HI Direct to GPIO
    {"RUN_NORTH_LO", GPIO_NUM_5,  (void *)&pcnt0           }, // RUN_NORTH_LO
    {"RUN_EAST_LO",  GPIO_NUM_6,  (void *)&pcnt1           }, // RUN_EAST_LO
    {"RUN_SOUTH_LO", GPIO_NUM_7,  (void *)&pcnt2           }, // RUN_SOUTH_LO
    {"RUN_WEST_LO",  GPIO_NUM_15, (void *)&pcnt3           }, // RUN_WEST_LO
    {0,              0,           0                        }
};

/*
 *  Variables
 */
const static gpio_type_t gpio_order[] = // Order in which to program devices
    {
        DIGITAL_IO_OUT,                 // GPIO is used for Digital Output
        ANALOG_IO,                      // GPIO is used for Analog IO
        SERIAL_AUX,                     // GPIO is used as Serial auxilary port
        PWM_OUT,                        // GPIO is used as a PWM port
        I2C_PORT,                       // GPIO is used as a i2c port
        LED_STRIP,                      // GPIO is used to drives a LED strip (status LEDs)
        PCNT,                           // GPIO is used as a Pulse Counter
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
   *  Loop and setup the GPIO outputs
   */
  i = 0;
  while ( gpio_order[i] != 0 )       // Program the IO in order
  {
    gpio_init_single(gpio_order[i]); //
    i++;
  }

  /*
   *  All done, return
   */
  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "GPIO complete");))
  vTaskDelay(10);
  return;
}

void gpio_init_single(unsigned int type) // What type of GPIO are we programming?
{
  unsigned int i;

                                         /*
                                          *  Loop throught the table looking for a match and program that
                                          */
  i = 0;
  while ( gpio_table[i].gpio_name != 0 )   // Look for the end of the table
  {
    if ( gpio_table[i].gpio_uses != NULL ) // Is the uses defined?
    {
      if ( ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->type == type )
      {
        switch ( ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->type )
        {
          default:
            printf("gpio not found %d", i);
            break;

          case DIGITAL_IO_IN:
          case PCNT_HI:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Digital input: %s", gpio_table[i].gpio_name);))
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
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Digital output: %s", gpio_table[i].gpio_name);))
            gpio_set_direction(gpio_table[i].gpio_number, ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->mode);
            gpio_set_pull_mode(gpio_table[i].gpio_number, GPIO_PULLUP_PULLDOWN);
            gpio_set_level(gpio_table[i].gpio_number, ((const DIO_struct_t *)(gpio_table[i].gpio_uses))->initial_value);
            break;

          case PWM_OUT:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "PWM output: %s", gpio_table[i].gpio_name);))

            pwm_init(((const PWM_struct_t *)(gpio_table[i].gpio_uses))->pwm_channel, gpio_table[i].gpio_number);
            break;

          case I2C_PORT:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "I2C: %s", gpio_table[i].gpio_name);))
            i2c_init(((I2C_struct_t *)(gpio_table[i].gpio_uses))->gpio_number_SDA,
                     ((I2C_struct_t *)(gpio_table[i].gpio_uses))->gpio_number_SCL);
            break;

          case LED_STRIP:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "LED driver: %s", gpio_table[i].gpio_name);))
            status_LED_init(gpio_table[i].gpio_number);
            break;

          case PCNT:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "pcnt: %s", gpio_table[i].gpio_name);))
            pcnt_init_FT(((const PCNT_struct_t *)(gpio_table[i].gpio_uses))->pcnt_unit,
                         ((const PCNT_struct_t *)(gpio_table[i].gpio_uses))->pcnt_control,
                         ((const PCNT_struct_t *)(gpio_table[i].gpio_uses))->pcnt_signal);
            break;

          case ANALOG_IO:
            DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Analog Input: %s", gpio_table[i].gpio_name);))
            adc_init(((const ADC_struct_t *)(gpio_table[i].gpio_uses))->adc_channel,
                     ((const ADC_struct_t *)(gpio_table[i].gpio_uses))->adc_attenuation);
            break;
        }
      }
    }
    i++;
  }

  /*
   *  All done, return
   */
  return;
}
