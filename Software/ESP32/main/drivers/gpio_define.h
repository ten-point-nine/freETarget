/*************************************************************************
 *
 * file: gpio_define.h
 *
 * description:  Public interface for gpio definition
 *
 **************************************************************************
 *
 *
 ***************************************************************************/

#ifndef __GPIO_DEFINE_H__
#define __GPIO_DEFINE_H__

#if ( 0 )
#define GPIO_NUM_1 1
#define GPIO_NUM_2 2
#define GPIO_NUM_3 3
#define GPIO_NUM_4 4
#define GPIO_NUM_5 5

#define GPIO_NUM_6  6
#define GPIO_NUM_7  7
#define GPIO_NUM_8  8
#define GPIO_NUM_9  9
#define GPIO_NUM_10 10

#define GPIO_NUM_11 11
#define GPIO_NUM_12 12
#define GPIO_NUM_13 13
#define GPIO_NUM_14 14
#define GPIO_NUM_15 15

#define GPIO_NUM_16 16
#define GPIO_NUM_17 17
#define GPIO_NUM_18 18
#define GPIO_NUM_19 19
#define GPIO_NUM_20 20

#define GPIO_NUM_21 21
#define GPIO_NUM_22 22
#define GPIO_NUM_23 23
#define GPIO_NUM_24 24
#define GPIO_NUM_25 25

#define GPIO_NUM_26 26
#define GPIO_NUM_27 27
#define GPIO_NUM_28 28
#define GPIO_NUM_29 29
#define GPIO_NUM_30 30

#define GPIO_NUM_31 31
#define GPIO_NUM_32 32
#define GPIO_NUM_33 33
#define GPIO_NUM_34 34
#define GPIO_NUM_35 35

#define GPIO_NUM_36 36
#define GPIO_NUM_37 37
#define GPIO_NUM_38 38
#define GPIO_NUM_39 39
#define GPIO_NUM_40 40
#endif

/*
 * function Prototypes
 */
void gpio_init(void);
void gpio_init_single(unsigned int type); // What type of GPIO are we programming?

/*
 * Type defs
 */
typedef enum gpio_type
{
  DIGITAL_IO_OUT = 1,                                // GPIO is used for digital IO
  DIGITAL_IO_IN,                                     // GPIO is used for digital IO
  ANALOG_IO,                                         // GPIO is used for Analog IO
  SERIAL_AUX,                                        // GPIO is used as Serial auxilary port
  PWM_OUT,                                           // GPIO is used as a PWM port
  I2C_PORT,                                          // GPIO is used as a i2c port
  PCNT,                                              // GPIO is used as a Pulse Counter
  PCNT_HI,                                           // GPIO associated with HI PCNT
  LED_STRIP                                          // GPIO is used to drives a LED strip (status LEDs)
} gpio_type_t;

typedef struct DIO_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         mode;                                  // Mode used by the DIO
  int         initial_value;                         // Value set on initialization
  bool (*callback)(void);                            // Pointer to callback if needed
} DIO_struct_t;

typedef struct ADC_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         adc_channel;                           // What channel are we using?
  int         adc_attenuation;                       // What is the attenuation setting
} ADC_struct_t;

#define ADC(adc, channel) (((adc) * 10) + (channel)) // Pack the channel and ADC into an integer
#define ADC_ADC(id)       (id / 10)                  // Unpack the ADC from the integer
#define ADC_CHANNEL(id)   (id % 10)                  // Unpack the channel from the interger

typedef struct serialIO_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         serial_config[4];                      // baud, parity, length, stop bits
} serialIO_struct_t;

typedef struct I2C_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         gpio_number_SDA;                       // Number associated with SDA
  int         gpio_number_SCL;                       // Number associated with SDA
} I2C_struct_t;

typedef struct PWM_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         pwm_channel;                           // PWM chanel assigned to this PWM
  int         gpio_number;                           // Number associated with PWM
} PWM_struct_t;

typedef struct PCNT_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         pcnt_unit;                             // What unit to use
  int         pcnt_control;                          // GPIO associated with PCNT control
  int         pcnt_signal;                           // GPIO associated with PCNT signal
  bool (*callback)(void *);                          // PCNT interrrupt handler
} PCNT_struct_t;

typedef struct LED_strip_struct
{
  gpio_type_t type;                                  // What type of structure am I
  int         gpio_number;                           // What unit to use
  int         led_qty;                               // How many LEDs are in the strip
} LED_strip_struct_t;

typedef struct gpio_struct
{
  char *gpio_name;                                   // GPIO name
  int   gpio_number;                                 // Number associated with GPIO
  void *gpio_uses;                                   // Pointer to IO specific structure
} gpio_struct_t;

extern const gpio_struct_t gpio_table[];             // List of available devices
#endif
