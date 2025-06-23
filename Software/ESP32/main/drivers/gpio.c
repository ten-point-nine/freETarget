/*-------------------------------------------------------
 *
 * gpio.c
 *
 * General purpose GPIO driver
 *
 * ----------------------------------------------------*/
#include <string.h>
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "gpio_types.h"
#include "driver\gpio.h"
#include "esp_timer.h"
#include "led_strip_types.h"

#include "freETarget.h"
#include "compute_hit.h"
#include "diag_tools.h"
#include "gpio.h"
#include "timer.h"
#include "json.h"
#include "serial_io.h"
#include "timer.h"
#include "pcnt.h"
#include "gpio_define.h"
#include "mfs.h"
#include "dac.h"
#include "analog_io.h"
#include "pwm.h"

#include "../managed_components/espressif__led_strip/src/led_strip_rmt_encoder.h"

/*
 * function prototypes
 */

/*
 *  Typedefs
 */
typedef struct status_struct
{
  int blue;  // Bits to send to the LED
  int green;
  int red;
  int blink; // TRUE if blinking enabled
} status_struct_t;

/*
 * Variables
 */
status_struct_t status[3] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};
int                   paper_state; // Drive is ON or OFF
time_count_t          paper_time;  // How long the paper will be on for
volatile unsigned int step_count;  // How many step counts do we need?
volatile unsigned int step_time;   // Interval to next step

/*-----------------------------------------------------
 *
 * @function: is_running
 *
 * @brief: Determine if the clocks are running
 *
 * @return: TRUE if any of the counters are running
 *
 *-----------------------------------------------------
 *
 * Read in the running registers, and return a 1 for every
 * register that is running.
 *
 * It is up to the application to ignore unused RUN bits
 * (ex RUN_NORTH_HI) is PCNT Latency has been removed
 *
 *----------------------------------- ------------------*/
unsigned int is_running(void)
{
  unsigned int return_value;
  unsigned int i;

  return_value = 0; //

  /*
   * Read the running inputs
   */
  for ( i = N; i <= W; i++ )
  {
    if ( gpio_get_level(s[i].low_sense.sensor_GPIO) != 0 )
    {
      return_value |= s[i].low_sense.run_mask;
    }
    if ( gpio_get_level(s[i].high_sense.sensor_GPIO) != 0 )
    {
      return_value |= s[i].high_sense.run_mask;
    }
  }

  /*
   *  Return the run mask
   */
  return (return_value); // Return the running mask INCLUDING PCNT HI
}

/*-----------------------------------------------------
 *
 * @function: arm_timers
 *
 * @brief: Strobe the control lines to start a new cycle
 *
 * @return: NONE
 *
 *-----------------------------------------------------
 *
 * The counters are armed by
 *
 *   Stopping the current cycle
 *   Clearing the counters
 *   Enabling the counters to run again
 *
 *-----------------------------------------------------*/
void arm_timers(void)
{
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF);
  gpio_set_level(STOP_N, RUN_OFF);      // Reset the timer
  gpio_set_level(OSC_CONTROL, OSC_OFF); // Turn off the oscillator
  pcnt_clear();
  if ( json_pcnt_latency != 0 )
  {
    gpio_intr_enable(RUN_NORTH_HI);     // Turn on the interrupts
    gpio_intr_enable(RUN_EAST_HI);
    gpio_intr_enable(RUN_SOUTH_HI);
    gpio_intr_enable(RUN_WEST_HI);
  }
  gpio_set_level(OSC_CONTROL, OSC_ON);  // Turn on the oscillator

  gpio_set_level(STOP_N, RUN_GO);       // Then enable it
  return;
}

/*
 *  Stop the oscillator and RUN_xx controls
 */
void stop_timers(void)
{
  gpio_set_level(OSC_CONTROL, OSC_OFF); // Turn off the oscillator
  gpio_set_level(STOP_N, RUN_OFF);      // Clear the flip flop
  return;
}

/*
 *  Trigger the timers for a self test
 */
void trigger_timers(void)
{
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF); // Send out a 0-1-0 to
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_ON);  // the flip flops to
  gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF); // clock in a 1 and start the process

  return;
}
/*-----------------------------------------------------
 *
 * @function: read_DIP
 *
 * @brief: READ the jumper block setting
 *
 * @return: TRUE for every position with a jumper installed
 *
 *-----------------------------------------------------
 *
 * The DIP register is read and formed into a word.
 * The word is complimented to return a 1 for every
 * jumper that is installed.
 *
 *-----------------------------------------------------*/

unsigned int read_DIP(void)
{
  unsigned int return_value = 0;
  unsigned int dips[]       = {DIP_A, DIP_B, DIP_C, DIP_D};
  unsigned int bit_mask[]   = {0x08, 0x04, 0x02, 0x01};
  unsigned int i;

  for ( i = 0; i != 4; i++ )
  {
    if ( gpio_get_level(dips[i]) != 0 )
    {
      return_value |= bit_mask[i];
    }
  }

  return return_value;
}

/*-----------------------------------------------------
 *
 * @function: init_status_LED
 *
 * @brief:    Initialize the LED driver
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * The status LED driver makes use of the remote control
 * transmitter supported in the ESP32
 *
 *-----------------------------------------------------*/
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000  // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

rmt_channel_handle_t    led_channel    = NULL;
rmt_tx_channel_config_t tx_chan_config = {
    .clk_src           = RMT_CLK_SRC_DEFAULT, // select source clock
    .mem_block_symbols = 64,                  // increase the block size can make the LED less flickering
    .resolution_hz     = RMT_LED_STRIP_RESOLUTION_HZ,
    .trans_queue_depth = 1,                   // set the number of transactions that can be pending in the background
};

rmt_encoder_handle_t       led_encoder    = NULL;
led_strip_encoder_config_t encoder_config = {.resolution = RMT_LED_STRIP_RESOLUTION_HZ};

void status_LED_init(unsigned int led_gpio    // What GPIO is used for output
)
{
  tx_chan_config.gpio_num = led_gpio;
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_channel));
  ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
  ESP_ERROR_CHECK(rmt_enable(led_channel));
  return;
}

/*-----------------------------------------------------
 *
 * @function: set_status_LED
 *
 * @brief:    Set the state of all the status LEDs
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * The state of the LEDs can be turned on or off
 *
 * 'R' - Set the LED to Red
 * 'r' - Blink the LED in red
 * '-' - Leave the LED alone
 * '.' - Turn the LED off
 *
 *-----------------------------------------------------*/
#define LED_ON       0x3F                     // Max full scale is 0xff (too bright)
#define N_SERIAL_LED 3
#define CEE          3                        // LED C takes the fourth position
#define DEE          4                        // LED D takes the fifth position

rmt_transmit_config_t tx_config = {
    .loop_count = 0,                          // no transfer loop
};

static unsigned char led_strip_pixels[3 * 3]; // 3 LEDs + 3 Bytes per LED
static unsigned char LED_C = ' ';             // Rapid fire LED C
static unsigned char LED_D = ' ';             // Rapid fire LED D

void set_status_LED(char new_state[]          // New LED colours
)
{
  static char *old_state = "   ";
  int          i;

  if ( new_state == old_state )               // Dont't do anything if the state is the same
  {
    return;
  }
  old_state = new_state;

  /*
   * Decode the calling string into a list of pixels
   */
  for ( i = 0; i != N_SERIAL_LED; i++ )
  {
    if ( new_state[i] != '-' ) // - Leave the setting alone
    {
      status[i].blink = 0;     // Default to blink off
      status[i].red   = 0;
      status[i].green = 0;
      status[i].blue  = 0;

      switch ( new_state[i] )
      {
        case 'r':              // RED LED
          status[i].blink = 1; // Turn on Blinking
        case 'R':
          status[i].red = LED_ON;
          break;

        case 'y':              // YELLOW LED
          status[i].blink = 1; // Turn on Blinking
        case 'Y':
          status[i].red   = LED_ON / 2;
          status[i].green = LED_ON / 2;
          break;

        case 'g':              // GREEN LED
          status[i].blink = 1; // Turn on Blinking
        case 'G':
          status[i].green = LED_ON;
          break;

        case 'b':              // BLUE LED
          status[i].blink = 1;
        case 'B':
          status[i].blue = LED_ON;
          break;

        case 'w':
          status[i].blink = 1; // WHITE LED
        case 'W':
          status[i].red   = LED_ON / 3;
          status[i].green = LED_ON / 3;
          status[i].blue  = LED_ON / 3;
          break;

        case ' ': // LEDs are all off
          break;
      }
    }
  }

  /*
   *  Record the C/D LEDs
   */
  if ( new_state[CEE] != '-' )
  {
    LED_C = new_state[CEE];
  }
  if ( new_state[DEE] != '-' )
  {
    LED_D = new_state[DEE];
  }

  /*
   * Ready to output the LEDs
   */
  commit_status_LEDs(1);
  return;
}

/*-----------------------------------------------------
 *
 * @function: commit_status_LED
 *
 * @brief:    Write the status LED settings to the hardware
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * This function looks at the blink state and outputs
 * the bits to the hardware.
 *
 * blink = 1 -> put to hardware
 * blink = 0 -> turn off the LED
 *
 *-----------------------------------------------------*/
void toggle_status_LEDs(void)
{
  static unsigned int blink_state = 0;

  commit_status_LEDs(blink_state);
  blink_state ^= 1;

  return;
}

void commit_status_LEDs(unsigned int blink_state)
{
  unsigned int i;

  /*
   *  Send out the new settings to the serial LEDs
   */
  for ( i = 0; i < N_SERIAL_LED; i++ )
  {
    led_strip_pixels[i * 3 + 0] = 0;                 // Turn them all off
    led_strip_pixels[i * 3 + 2] = 0;
    led_strip_pixels[i * 3 + 1] = 0;
    if ( (status[i].blink == 0)                      // Blinking is off (ie, always on)
         || (blink_state == 1) )                     // Or, we are in a blink-on cycle
    {
      led_strip_pixels[i * 3 + 0] = status[i].green; // Set the RGB
      led_strip_pixels[i * 3 + 2] = status[i].blue;
      led_strip_pixels[i * 3 + 1] = status[i].red;
    }
  }

  rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);

  /*
   *  Send out the Rapid Fire LEDs
   */
  switch ( LED_C )
  {
    default:
    case ' ':
      rapid_green(0);
      break;
    case 'g':
      rapid_green(blink_state);
      break;
    case 'G':
      rapid_green(1);
      break;
    case 'r':
      rapid_red(blink_state);
      break;
    case 'R':
      rapid_red(1);
      break;
  }

  switch ( LED_D )
  {
    default:
    case ' ':
      rapid_red(0);
      break;
    case 'g':
      rapid_green(blink_state);
      break;
    case 'G':
      rapid_green(1);
      break;
    case 'r':
      rapid_red(blink_state);
      break;
    case 'R':
      rapid_red(1);
      break;
  }

  /*
   * All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: read_timers
 *
 * @brief:   Read the timer registers
 *
 * @return:  All four timer registers read and stored
 *
 *-----------------------------------------------------
 *
 * Force read each of the timers
 *
 * Compensation for rise time
 *
 *
 *
 *                   *
 *    vref_hi      * +
 *               *   + vref_hi - vref_lo
 *             *     +
 *    vref_lo *      +
 *           *++++++++
 *         *   pcnt_hi3
 *        *
 *       * origin
 *
 *                         vref_lo
 * origin = pcnt_lo - ----------------  * pcnt_hi
 *                    vref_hi - vref_lo
 *
 * IMPORTANT
 *
 * pcnt_hi is the time from the start of pcnt_lo starting
 * until vref_hi is triggered
 *
 *
 *-----------------------------------------------------*/
void read_timers(int timer[])
{
  unsigned int i;
  double       pcnt_hi; // Reading from high counter

  for ( i = 0; i != 8; i++ )
  {
    timer[i] = pcnt_read(i);
  }

  if ( (json_pcnt_latency != 0)                   // Latecy has a valid setting
       && ((json_vref_hi - json_vref_lo) > 0) )   // The voltage references are good
  {
    for ( i = N; i <= W; i++ )                    // Add the rise time to the signal to get a better estimate
    {
      pcnt_hi = timer[i + 4] - json_pcnt_latency; // PCNT HI   (reading - latentcy)
      if ( pcnt_hi > PCNT_NOT_TRIGGERED )         // Check to make sure the high timer was triggered by a shot
      {                                           // and not dinged from the pellet trap
        pcnt_hi = 0;                              // Not triggered by a shot
      }
      if ( pcnt_hi > 0 )
      {
        timer[i] = timer[i] + pcnt_hi * (json_vref_lo / (json_vref_hi - json_vref_lo));
      }
    }
  }

  return;
}

/*-----------------------------------------------------
 *
 * @function: paper_start
 *
 * @brief:    Start advancing the paper
 *
 * @return:  None
 *
 *-----------------------------------------------------
 *
 * These are the starting activities to begin moving
 * the witness paper.
 *
 * In the case of the DC motor, turn on the FET
 *
 * Stepper motors require the stepper to be ramped up
 *
 * Once the motors have started, paper_drive_tick will
 * keep it moving
 *
 * Use an A4988 to drive te stepper in place of a DC
 * motor
 *
 * Test Settings
 *
 * DC Motor
 * Step Count = 0
 * Step Time = 0
 * Paper Time = Motor ON time
 * {"PAPER_TIME":500, "STEP_COUNT": 0, "STEP_TIME":0,  "MFS_HOLD_C":0, "MFS_HOLD_D":0}
 *
 * Stepper Motor
 * Step Count = Number of pulses to send
 * Step Start = Starting number of pulses
 * Step Ramp =  Time period to reduce each cycle while starting
 * Step Time =  Period on/off of each pulse (50% duty cycle)
 * MFS_HOLD_C = STEPPER_DRIVE  26 // The output drives a stepper motor
 * MFS_HOLD_D = STEPPER_ENABLE 28 // The output enables the stepper motor
 *
 * Paper Time = 0
 * {"MFS_HOLD_C":26, "MFS_HOLD_D":28, "STEP_START":200, "STEP_RAMP": 5, "STEP_TIME":30, "STEP_COUNT": 200, "PAPER_TIME":0}
 *
 *-----------------------------------------------------*/
void paper_start(void)
{

  /*
   *  DC Motor, turn on the FET to start the motor
   */
  if ( IS_DC_WITNESS ) // DC motor,
  {
    DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "DC motor start: %d ms", json_paper_time);))
    DCmotor_on_off(true, json_paper_time);
  }

  /*
   * Set up the stepper and trigger the first pulse
   */
  if ( IS_STEPPER_WITNESS )       // Stepper
  {
    if ( json_mfs_hold_d == STEPPER_ENABLE )
    {
      gpio_set_level(HOLD_D_GPIO, STEP_ENABLE);
    }
    step_count = json_step_count; // Set local variables
    step_time  = json_step_start; // Start off slowly
    paper_drive_tick();           // Send out the first tick
  }

  return;
}

/*-----------------------------------------------------
 *
 * @function: paper_drive_tick
 *
 * @brief:    Drive the DC or Stepper motor drive
 *
 * @return:  None
 *
 *-----------------------------------------------------
 *
 * The function is called every 10 ms to update the
 * controls to the DC or stepper motor depending on
 * what has been selected in the configuration
 *
 *-----------------------------------------------------*/
void paper_drive_tick(void)
{

  /*
   * Drive the DC motor
   */
  if ( IS_DC_WITNESS )
  {
    if ( paper_time == 0 )
    {
      paper_stop(); // Motor OFF
    }
  }

  /*
   * Drive the stepper motor
   */
  if ( IS_STEPPER_WITNESS )  // Stepper enabled
  {
    if ( step_count != 0 )   // In motion
    {
      if ( paper_time == 0 ) // Timer for next pulse?
      {
        stepper_pulse();     // Motor toggle
      }
    }
    else
    {
      paper_stop();
    }
  }

  /*
   * All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: paper_stop
 *
 * @brief:    Stop advancing the paper
 *
 * @return:  None
 *
 *-----------------------------------------------------
 *
 *  General purpose function to stop the paper advancing
 *
 *-----------------------------------------------------*/
void paper_stop(void)
{

  /*
   * See what kind of drive we are using
   */
  if ( IS_DC_WITNESS )        // DC motor - Turn the output on once
  {
    DCmotor_on_off(false, 0); // Motor OFF
    ft_timer_delete(&paper_time);
  }

  if ( IS_STEPPER_WITNESS )   // Stepper motor - Toggle the output
  {
    step_count = 0;
    if ( json_mfs_hold_d == STEPPER_ENABLE )
    {
      gpio_set_level(HOLD_D_GPIO, STEP_DISABLE);
    }
    ft_timer_delete(&paper_time);
  }

  /*
   * All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: DCmotor_on_off
 *
 * @brief:    Turn the withness paper motor on or off
 *
 * @return:  None
 *
 *-----------------------------------------------------
 *
 * DCmotor_on_off turns the DC motor drive on or off
 * for the specified duration.
 *
 * To stop the motor once it is running use
 *
 * DCmotor_on_off(false, 0)
 *
 *-----------------------------------------------------*/
void DCmotor_on_off(bool          on,      // on == true, turn on motor drive
                    unsigned long duration // How long will it be on for in ms?
)
{
  /*
   *  We have a supply, continue
   */
  paper_state = on;

  if ( on == true )
  {
    gpio_set_level(PAPER, PAPER_ON);  // Turn it on
    ft_timer_new(&paper_time, MS_TO_TICKS(duration));
  }
  else
  {
    gpio_set_level(PAPER, PAPER_OFF); // Turn it off
    ft_timer_delete(&paper_time);
  }

  /*
   * No more, return
   */
  return;
}

int is_paper_on(void) // Return true if there is still time
{
  return (paper_time != 0);
}

/*----------------------------------------------------------------
 *
 * @function: stepper_pulse()
 *
 * @brief: Send a single pulse to the stepper motor driver
 *
 * @return: Nothing
 *
 *----------------------------------------------------------------
 *
 * This is the companion to DCmotor_on_off for the stepper motor
 *
 * The function also calculates a slew rate from stopped to full
 * speed
 *
 *--------------------------------------------------------------*/

void stepper_pulse(void)
{
  gpio_set_level(HOLD_C_GPIO, STEP_ON); // Pulse the stepper drive
  gpio_set_level(HOLD_C_GPIO, STEP_OFF);

  step_time = step_time - json_step_ramp;

  if ( step_time < json_step_time )
  {
    step_time = json_step_time;
  }

  DLT(DLT_DIAG, SEND(ALL, sprintf(_xs, "step_time %d   step_count: %d", step_time, step_count);))
  ft_timer_new(&paper_time, MS_TO_TICKS(step_time));

  if ( step_count != 0 )
  {
    step_count--;
  }

  /*
   *  All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: face_ISR
 *
 * @brief:    Face Strike Interrupt Service Routint
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Sensor #5 is attached to the digital input #19 and
 * is used to generate an interrrupt whenever a face
 * strike has been detected.
 *
 * The ISR simply counts the number of cycles.  Anything
 * above 0 is an indication that sound was picked up
 * on the front face.
 *
 *-----------------------------------------------------*/
void face_ISR(void)
{
  face_strike++; // Got a face strike

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "\r\nface_ISR(): %d", face_strike);))

  return;
}

/*----------------------------------------------------------------
 *
 * @function: aquire()
 *
 * @brief: Aquire the data from the counter registers
 *
 * @return: Nothing
 *
 *----------------------------------------------------------------
 *
 *  This function reads the values from the counters and saves
 *  saves them into the record structure to be reduced later
 *  on.
 *
 *  The conditional IF_IN(IN_SHOT) is used to discard shots that
 *  are present while the target is not available for use.  For
 *  example IN_SHOT will be invalid while in rapid fire if the
 *  shot falls outside of the shot time
 *
 *--------------------------------------------------------------*/
void aquire(void)
{
  /*
   * Pull in the data amd save it in the record array
   */
  read_timers(&record[shot_in].timer_count[0]);                 // Record this count
  IF_IN(IN_SHOT)                                                // Only record the shot if we are actually expecting a shot
  {
    record[shot_in].shot_time     = run_time_ms() - shot_start; // Capture the time into the shot
    record[shot_in].face_strike   = face_strike;                // Record if it's a face strike
    record[shot_in].sensor_status = is_running();               // Record the sensor status
    shot_in                       = (shot_in + 1) % SHOT_SPACE; // Prepare for the next shot
  }

  /*
   * All done for now
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: rapid_red()
 *           rapid_green()
 *
 * @brief: Set the RED and GREEN lights
 *
 * @return: Nothing
 *
 *----------------------------------------------------------------
 *
 *  If MFS2 has enabled the rapid fire lights then allow the
 *  value to be set
 *
 *  IMPORTANT
 *
 *  LEDs are driven ON by an active low signal
 *
 *--------------------------------------------------------------*/

void rapid_red(unsigned int state        // New state for the RED light
)
{
  if ( json_mfs_select_cd == RAPID_LOW ) // Inverted drive
  {
    state = !state;
  }
  if ( IS_HOLD_C(RAPID_RED) )
  {
    gpio_set_level(DIP_C, state);
  }
  if ( IS_HOLD_D(RAPID_RED) )
  {
    gpio_set_level(DIP_D, state);
  }

  return;
}

void rapid_green(unsigned int state      // New state for the GREEN light
)
{
  if ( json_mfs_select_cd == RAPID_LOW ) // Inverted drive
  {
    state = !state;
  }

  if ( IS_HOLD_C(RAPID_GREEN) )
  {
    gpio_set_level(DIP_C, state);
  }
  if ( IS_HOLD_D(RAPID_GREEN) )
  {
    gpio_set_level(DIP_D, state);
  }

  return;
}

/*-----------------------------------------------------
 *
 * @function: digital_test()
 *
 * @brief:    Exercise the GPIO digital ports
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Read in all of the digial ports and report the
 * results
 *
 *-----------------------------------------------------*/
void digital_test(void)
{
  /*
   * Read in the fixed digital inputs
   */
  SEND(ALL, sprintf(_xs, "\r\nTime: %lds", run_time_seconds());)
  SEND(ALL, sprintf(_xs, "\r\nDIP: 0x%02X", read_DIP());)
  SEND(ALL, sprintf(_xs, _DONE_);)

  return;
}

/*----------------------------------------------------------------
 *
 * @function: status_LED_test()
 *
 * @brief:    Cycle the status LEDs
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void status_LED_test(void)
{
  if ( ((IS_HOLD_C(RAPID_RED)) && (IS_HOLD_C(RAPID_GREEN))) || ((IS_HOLD_D(RAPID_RED)) && (IS_HOLD_D(RAPID_GREEN))) )
  {
    SEND(ALL, sprintf(_xs, "\r\nMFS_C or MFS_D not configured for output\r\n");)
  }

  timer_delay(2 * ONE_SECOND);
  set_status_LED("RRRRR");
  timer_delay(2 * ONE_SECOND);
  set_status_LED("GGGGG");
  timer_delay(ONE_SECOND);
  set_status_LED("BBBRG");
  timer_delay(ONE_SECOND);
  set_status_LED("WWWGR");
  timer_delay(ONE_SECOND);
  set_status_LED("RGBRG");
  timer_delay(ONE_SECOND);
  set_status_LED("rgbrg");
  timer_delay(5 * ONE_SECOND); // Blink for 5 seconds
  set_status_LED(LED_READY);
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: paper_test
 *
 * @brief:    Drive the motor
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 *  Drive the motor in 500 ms increments.
 *
 *  This function drives the motor directly and does not use the
 *  functions paper_drive() or paper_drive_tick().
 *
 *--------------------------------------------------------------*/
void paper_test(void)
{
  int i;

  /*
   *  See if we have power attached
   */
  if ( check_12V() == false )
  {
    SEND(ALL, sprintf(_xs, "\r\nTest failed, no 12V supply");)
    return;
  }

  /*
   * Try running the test
   */
  SEND(ALL, sprintf(_xs, "\r\nAdvancing paper: 500 ms 10x");)
  for ( i = 0; i != 10; i++ )
  {
    SEND(ALL, sprintf(_xs, "  %d+", (i + 1));)
    DCmotor_on_off(true, ONE_SECOND / 2);
    timer_delay(ONE_SECOND / 2);
    SEND(ALL, sprintf(_xs, "-");)
    DCmotor_on_off(false, 0);
    timer_delay(ONE_SECOND / 2);
  }

  SEND(ALL, sprintf(_xs, _DONE_);)

  return;
}

/*----------------------------------------------------------------
 *
 * @function: LED_test
 *
 * @brief:    Cycle the LED brightness
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 *  Drive the LEDs in 5% increments evart 100ms.
 *
 *--------------------------------------------------------------*/
void LED_test(void)
{
  int i;

  SEND(ALL, sprintf(_xs, "\r\nCycling the LED");)

  for ( i = 0; i <= 100; i += 5 )
  {
    SEND(ALL, sprintf(_xs, "%d   ", i);)
    pwm_set(LED_PWM, i);
    vTaskDelay(ONE_SECOND / 10);
  }

  for ( i = 100; i >= 0; i -= 5 )
  {
    SEND(ALL, sprintf(_xs, "%d   ", i);)
    pwm_set(LED_PWM, i);
    vTaskDelay(ONE_SECOND / 10);
  }

  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

/*----------------------------------------------------------------
 *
 * @function: timer_control_tests
 *
 * @brief:    A collection of tests to exercise the timers
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 *  timer_run_all - Trigger the clock start and observe the output
 *                  on an oscilloscope.
 *
 *  timer_cycle_oscillator - Turn the 10MHz clock off and on
 *
 *--------------------------------------------------------------*/
void timer_run_all(void)
{
  SEND(ALL, sprintf(_xs, "\r\nCycle RUN lines at 2:1 duty cycle");)
  SEND(ALL, sprintf(_xs, "\r\nPress any key to stop");)
  while ( serial_available(ALL) == 0 )
  {
    gpio_set_level(STOP_N, RUN_GO);                 // Let the clock go
    gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF);
    gpio_set_level(CLOCK_START, CLOCK_TRIGGER_ON);
    gpio_set_level(CLOCK_START, CLOCK_TRIGGER_OFF); // Strobe the RUN linwes
    vTaskDelay(ONE_SECOND / 2);                     // The RUN lines should be on for 1/2 second
    gpio_set_level(STOP_N, RUN_OFF);                // Stop the clock
    vTaskDelay(ONE_SECOND / 4);                     // THe RUN lines shold be off for 1/4 second
  }

  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}

void timer_cycle_oscillator(void)
{
  SEND(ALL, sprintf(_xs, "\r\nCycle 10MHz Osc 2:1 duty cycle\r\n");)
  SEND(ALL, sprintf(_xs, "\r\nPress any key to stop.");)
  while ( serial_available(ALL) == 0 )
  {
    gpio_set_level(OSC_CONTROL, OSC_ON);  // Turn off the oscillator
    vTaskDelay(ONE_SECOND / 2);           // The oscillator should be on for 1/2 second
    gpio_set_level(OSC_CONTROL, OSC_OFF); // Turn off the oscillator
    vTaskDelay(ONE_SECOND / 4);           // The oscillator shold be off for 1/4 seocnd
  }
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
