
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
  DIGITAL_IO_OUT = 1,                                    // GPIO is used for digital IO
  DIGITAL_IO_IN,                                         // GPIO is used for digital IO
  ANALOG_IO,                                             // GPIO is used for Analog IO
  I2C_PORT,                                              // GPIO is used as a i2c port
} gpio_type_t;

typedef struct DIO_struct
{
  gpio_type_t type;                                      // What type of structure am I
  int         mode;                                      // Mode used by the DIO
  int         initial_value;                             // Value set on initialization
  bool (*callback)(void);                                // Pointer to callback if needed
} DIO_struct_t;

typedef struct ADC_struct
{
  gpio_type_t type;                                      // What type of structure am I
  int         adc_channel;                               // What channel are we using?
  int         adc_attenuation;                           // What is the attenuation setting
} ADC_struct_t;

#define ADC(adc, channel) (((adc - 1) * 10) + (channel)) // Pack the channel and ADC into an integer
#define ADC_ADC(id)       (id / 10)                      // Unpack the ADC from the integer
#define ADC_CHANNEL(id)   (id % 10)                      // Unpack the channel from the interger

typedef struct serialIO_struct
{
  gpio_type_t type;                                      // What type of structure am I
  int         serial_config[4];                          // baud, parity, length, stop bits
} serialIO_struct_t;

typedef struct I2C_struct
{
  gpio_type_t type;                                      // What type of structure am I
  int         gpio_number_SDA;                           // Number associated with SDA
  int         gpio_number_SCL;                           // Number associated with SDA
} I2C_struct_t;

typedef struct gpio_struct
{
  char        *gpio_name;                                // GPIO name
  int          gpio_number;                              // Number associated with GPIO
  void        *gpio_uses;                                // Pointer to IO specific structure
  unsigned int board_mask;                               // Board mask associated with this GPIO
} gpio_struct_t;

extern const gpio_struct_t gpio_table[];                 // List of available devices
#endif
