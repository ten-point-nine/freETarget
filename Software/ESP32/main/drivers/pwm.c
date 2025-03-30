/*************************************************************************
 *
 * file: pwm.c
 *
 * description:  PWM driver for LEDs and Voltag Reference
 *
 **************************************************************************
 *
 * This file sets up the timers and routing for the PWM control
 *
 * See:
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
 *
 ***************************************************************************/
#include "driver/ledc.h"

#include "freETarget.h"
#include "pwm.h"
#include "serial_io.h"

#define PWM_TIMER     LEDC_TIMER_0
#define PWM_MODE      LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL   LEDC_CHANNEL_0
#define PWM_DUTY_RES  LEDC_TIMER_13_BIT         // Set duty resolution to 13 bits
#define PWM_DUTY_100  ((1 << PWM_DUTY_RES) - 1) // 100% duty cycle
#define PWM_FREQUENCY (5000)                    // Frequency in Hertz. Set frequency at 5 kHz

static int pwm_ready = 0;                       // Set to 1 when the hardware is programmed

ledc_channel_config_t ledc_channel[4];

/*************************************************************************
 *
 * @function: pwm_init()
 *
 * description:  Set the PWM duty cycle
 *
 * @return:  None
 *
 **************************************************************************
 *
 * The PWM registers are updated for the new duty cycle
 *
 ***************************************************************************/

ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_13_BIT,   // resolution of PWM duty
    .freq_hz         = 5000,                // frequency of PWM signal
    .speed_mode      = LEDC_LOW_SPEED_MODE, // timer mode
    .timer_num       = LEDC_TIMER_0,        // timer index
    .clk_cfg         = LEDC_AUTO_CLK,       // Auto select the source clock
};

void pwm_init(unsigned int pwm_channel,     // PWM channel we are using
              unsigned int pwm_gpio         // What GPIO is it assigned to?
)
{
  /*
   * Configure the timer channel
   */
  if ( pwm_ready == 0 )
  {
    ledc_timer_config(&ledc_timer); // Setup the timer
    pwm_ready = 1;
  }
  SEND(ALL, sprintf(_xs, "channel: %d  gpio:%d", pwm_channel, pwm_gpio);)
  /*
   * Configure the output port
   */
  ledc_channel[pwm_channel].gpio_num            = pwm_gpio;
  ledc_channel[pwm_channel].channel             = pwm_channel;
  ledc_channel[pwm_channel].speed_mode          = LEDC_LOW_SPEED_MODE;
  ledc_channel[pwm_channel].timer_sel           = LEDC_TIMER_0;
  ledc_channel[pwm_channel].intr_type           = LEDC_INTR_DISABLE;
  ledc_channel[pwm_channel].duty                = 0; // Set duty to 0%
  ledc_channel[pwm_channel].hpoint              = 0;
  ledc_channel[pwm_channel].flags.output_invert = 0;
  ledc_channel_config(&ledc_channel[pwm_channel]);

  /*
   *  Initalize the output
   */
  ledc_set_duty(PWM_MODE, ledc_channel[pwm_channel].channel, ledc_channel[pwm_channel].duty);
  ledc_update_duty(PWM_MODE, ledc_channel[pwm_channel].channel);

  /*
   *  All done,
   */
  return;
}

/*************************************************************************
 *
 * @function: pwm_set()
 *
 * description:  Set the PWM duty cycle
 *
 * @return:  Nope
 *
 **************************************************************************
 *
 * The PWM registers are updated for the new duty cycle.
 *
 *
 *
 ***************************************************************************/
void pwm_set(unsigned int pwm_channel, // Channel being operated on
             unsigned int percent      // New duty cycle percentage
)
{
  unsigned int scaled;

  scaled = (1 << 13) * percent / 100;
  ledc_set_duty(PWM_MODE, ledc_channel[pwm_channel].channel, scaled);
  ledc_update_duty(PWM_MODE, ledc_channel[pwm_channel].channel);

  /*
   * All done, return
   */
  return;
}
