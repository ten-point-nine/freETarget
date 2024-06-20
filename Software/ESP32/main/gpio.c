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

#include "../managed_components/espressif__led_strip/src/led_strip_rmt_encoder.h"

/*
 * function prototypes
 */

/* 
 *  Typedefs
 */
typedef struct status_struct {
  int blue;                               // Bits to send to the LED
  int green;
  int red;   
  int blink;                              // TRUE if blinking enabled
  }  status_struct_t;

/* 
 * Variables
 */
status_struct_t status[3] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}};
int paper_state;                    // Drive is ON or OFF
volatile unsigned long paper_time;  // How long the paper will be on for
volatile unsigned long step_count;  // How many step counts do we need?

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
 *-----------------------------------------------------*/
static unsigned int clock[] = { RUN_NORTH_LO, RUN_EAST_LO, RUN_SOUTH_LO, RUN_WEST_LO, 
                                RUN_NORTH_HI, RUN_EAST_HI, RUN_SOUTH_HI, RUN_WEST_HI  };
static unsigned int run_mask[] = {BIT_NORTH_LO, BIT_EAST_LO, BIT_SOUTH_LO, BIT_WEST_LO,
                                  BIT_NORTH_HI, BIT_EAST_HI, BIT_SOUTH_HI, BIT_WEST_HI};

unsigned int is_running (void)
{
  unsigned int  return_value;
  unsigned int  i;

  return_value = 0;
/*
 * Read the running inputs
 */
  for (i=0; i != 8; i++)
  {
    if ( gpio_get_level(clock[i]) != 0 )
    {
      return_value |= run_mask[i];
    }
  }

/*
 *  Return the run mask
 */
  return return_value;                   // Return the running mask
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
  gpio_set_level(CLOCK_START, 0);
  gpio_set_level(STOP_N, 0);                  // Reset the timer
  gpio_set_level(OSC_CONTROL, OSC_OFF);       // Turn off the oscillator
  pcnt_clear();
  gpio_intr_enable(RUN_NORTH_HI);             // Turn on the interrupts
  gpio_intr_enable(RUN_EAST_HI);
  gpio_intr_enable(RUN_SOUTH_HI);
  gpio_intr_enable(RUN_WEST_HI);
  gpio_set_level(OSC_CONTROL, OSC_ON);        // Turn on the oscillator

  gpio_set_level(STOP_N, 1);                  // Then enable it
  return;
}


/*
 *  Stop the oscillator and RUN_xx controls
 */
void stop_timers(void)
{
  gpio_set_level(OSC_CONTROL, OSC_OFF);
  gpio_set_level(STOP_N, 0);      // Reset the timer
  return;
}

/*
 *  Trigger the timers for a self test
 */
void trigger_timers(void)
{
  gpio_set_level(CLOCK_START, 0);
  gpio_set_level(CLOCK_START, 1);
  gpio_set_level(CLOCK_START, 0);

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
  unsigned int dips[] = {DIP_A, DIP_B, DIP_C, DIP_D};
  unsigned int bit_mask[] = {0x08, 0x04, 0x02, 0x01};
  unsigned int i;

  for (i=0; i != sizeof(dips)/sizeof(unsigned int); i++)
  {
    if (gpio_get_level(dips[i]) != 0) 
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
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

rmt_channel_handle_t led_channel = NULL;
rmt_tx_channel_config_t tx_chan_config = {
    .clk_src           = RMT_CLK_SRC_DEFAULT, // select source clock
    .mem_block_symbols = 64, // increase the block size can make the LED less flickering
    .resolution_hz     = RMT_LED_STRIP_RESOLUTION_HZ,
    .trans_queue_depth = 1, // set the number of transactions that can be pending in the background
};

rmt_encoder_handle_t led_encoder = NULL;
led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ
};

void status_LED_init
(
  unsigned int led_gpio   // What GPIO is used for output
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
#define   LED_ON 0x3F

rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

unsigned char led_strip_pixels[3 * 3];

void set_status_LED
  (
    char* new_state       // New LED colours
  )
{ 
  int i;

/*
 * Decode the calling string into a list of pixels
 */
  i=0;
  while (*new_state != 0)
  {
    if ( *new_state != '-' )      // - Leave the setting alone
    {
      status[i].blink = 0;        // Default to blink off
      status[i].red   = 0;
      status[i].green = 0;
      status[i].blue  = 0;    

      switch (*new_state)
      {
        case 'r':                 // RED LED
          status[i].blink = 1;    // Turn on Blinking
        case 'R':
          status[i].red   = LED_ON;
          break;

        case 'y':                 // YELLOW LED
          status[i].blink = 1;    // Turn on Blinking
        case 'Y':
          status[i].red   = LED_ON/2;
          status[i].green = LED_ON/2;
          break;

        case 'g':                 // GREEN LED
          status[i].blink = 1;    // Turn on Blinking
        case 'G':
          status[i].green = LED_ON;
          break;

        case 'b':                 // BLUE LED
          status[i].blink = 1;
        case 'B':
          status[i].blue  = LED_ON;
          break;

        case 'w':
          status[i].blink = 1;    // WHITE LED
        case 'W':
          status[i].red   = LED_ON/3;
          status[i].green = LED_ON/3;
          status[i].blue  = LED_ON/3;
          break;

        case ' ':                 // LEDs are all off
          break;
      }
    }
    i++;
    new_state++;
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
void commit_status_LEDs
  (
    unsigned int blink_state
  )
{
  unsigned int i;

/*
 *  Send out the new settings
 */
  for (i=0; i < 3; i++)
  {
    led_strip_pixels[i * 3 + 0] = 0;
    led_strip_pixels[i * 3 + 2] = 0;
    led_strip_pixels[i * 3 + 1] = 0;
    if ( (status[i].blink == 0) || (blink_state == 1) )
    {
      led_strip_pixels[i * 3 + 0] = status[i].green;
      led_strip_pixels[i * 3 + 2] = status[i].blue;
      led_strip_pixels[i * 3 + 1] = status[i].red;
    }
  }
    
  rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);

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
void read_timers
(
  int timer[]
)
{
  unsigned int i;
  double pcnt_hi;                               // Reading from high counter 

  for (i=0; i != 8; i++)
  {
    timer[i] = pcnt_read(i);
  }

  if ( (json_pcnt_latency != 0)                   // Latecy has a valid setting
          && ((json_vref_hi - json_vref_lo) > 0 ) ) // The voltage references are good
  {
    for (i=N; i <= W; i++)                        // Add the rise time to the signal to get a better estimate
    {
      pcnt_hi = timer[i+4] - json_pcnt_latency;   // PCNT HI   (reading - latentcy)
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
 * @function: drive_paper
 * 
 * @brief:    Turn on the witness paper motor for json_paper_time
 * 
 * @return:  None
 * 
 *-----------------------------------------------------
 *
 * The function turns on the motor for the specified
 * time.  The motor is cycled json_paper_step times
 * to drive a stepper motor using the same circuit.
 * 
 * Use an A4988 to drive te stepper in place of a DC
 * motor
 * 
 * There is a hardare change between Version 2.2 which
 * used a transistor and 3.0 that uses a FET.
 * The driving circuit is reversed in the two boards.
 * 
 * DC Motor
 * Step Count = 0
 * Step Time = 0
 * Paper Time = Motor ON time
 * 
 * Stepper Motor
 * Step Count = Number of pulses to send
 * Step Time =  Period on/off of each pulse (50% duty cycle)
 * Paper Time = 0
 * 
 * {"PAPER_TIME":500, "STEP_COUNT": 0, {"STEP_TIME":0}
 * {"PAPER_TIME":0, "STEP_COUNT": 100, {"STEP_TIME":20}
 * 
 *-----------------------------------------------------*/
void drive_paper(void)
{
  DLT(DLT_DIAG, printf("Advancing paper: %dms", json_paper_time);)

/*
 * See what kind of drive we are using
 */
  if ( json_paper_time != 0 )       // DC motor - Turn the output on once
  {
    paper_on_off(true, ONE_SECOND * json_paper_time / 1000);         // Motor OFF
  }
  else if ( json_step_count != 0 )  // Stepper motor - Toggle the output
  {
    step_count = json_step_count * 2;// Two states per cycle
    stepper_off_toggle(false, ONE_SECOND * json_step_time / 1000);  // Motor OFF
  }

 /*
  * All done, return
  */
  return;
 }
/*-----------------------------------------------------
 * 
 * @function: drive_paper_tick
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
void drive_paper_tick(void)
{

/*
 * Drive the DC motor
 */
  if ( json_paper_time != 0 )
  {
    if ( (paper_time == 0) && (is_paper_on() != 0) )
    {
      paper_on_off(false, 0);                      // Motor OFF
      DLT(DLT_DIAG, printf("Done");)
    }
  }
  
/*
 * Drive the stepper motor
 */
  if ( json_step_count != 0 )   // Stepper enabled
  {
    if ( step_count >= 1)       // In motion
    {
      stepper_off_toggle(true, ONE_SECOND * json_step_time / 1000); // Motor toggle
      step_count--;
    }
    else                        // Motion finished
    {
      stepper_off_toggle(false, 0); // Motor OFF
    }
  }

 /*
  * All done, return
  */
  return;
 }

/*-----------------------------------------------------
 * 
 * @function: paper_on_off
 * 
 * @brief:    Turn the withness paper motor on or off
 * 
 * @return:  None
 * 
 *-----------------------------------------------------
 *
 * The witness paper motor changed polarity between 2.2
 * and Version 3.0.
 * 
 * This function reads the board revision and controls 
 * the FET accordingly
 * 
 *-----------------------------------------------------*/
void paper_on_off                               // Function to turn the motor on and off
(
  bool on,                                      // on == true, turn on motor drive
  unsigned long duration                        // How long will it be on for? 
)
{
  paper_state = on;

  if ( on == true )
  {
    gpio_set_level(PAPER, PAPER_ON);            // Turn it on
  }
  else
  {
    gpio_set_level(PAPER, PAPER_OFF);            // Turn it off
  }

/*
 * No more, return
 */
  timer_new(&paper_time, duration);
  return;
}

int is_paper_on(void)
{
  return paper_state;
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
  face_strike++;      // Got a face strike

  DLT(DLT_CRITICAL, printf("\r\nface_ISR(): %d", face_strike);)

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
 *--------------------------------------------------------------*/
void aquire(void)
 {
/*
 * Pull in the data amd save it in the record array
 */
  read_timers(&record[this_shot].timer_count[0]);   // Record this count
  record[this_shot].shot_time = 0;                  // Capture the time into the shot
  record[this_shot].face_strike = face_strike;      // Record if it's a face strike
  record[this_shot].sensor_status = is_running();   // Record the sensor status
  record[this_shot].shot_number = shot_number++;    // Record the shot number and increment
  this_shot = (this_shot+1) % SHOT_STRING;          // Prepare for the next shot

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

void rapid_red
(
  unsigned int state          // New state for the RED light
) 
{
  if ( json_mfs_select_cd == RAPID_LOW )   // Inverted drive
  {
    state = !state;
  }
  if ( json_mfs_hold_c == RAPID_RED )
  {
      gpio_set_level(DIP_C, state);
  }
  if ( json_mfs_hold_d == RAPID_RED )
  {
      gpio_set_level(DIP_D, state);
  }

  return;
}

void rapid_green
(
  unsigned int state          // New state for the GREEN light
) 
{
  if ( json_mfs_select_cd == RAPID_LOW )   // Inverted drive
  {
    state = !state;
  }

  if ( json_mfs_hold_c == RAPID_GREEN )
  {
      gpio_set_level(DIP_C, state);
  }
  if ( json_mfs_hold_d == RAPID_GREEN )
  {
      gpio_set_level(DIP_D, state);
  }

  return;
}

/*----------------------------------------------------------------
 * 
 * @function: stepper_off_toggle()
 * 
 * @brief: Set the stepper motor state
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

void stepper_off_toggle
(
  unsigned int  action,         // action, off (0) or toggle (1)
  unsigned long duration        // Time to next state
) 
{
  static int current_state;     // Memory of the output

  if ( json_mfs_hold_c == STEPPER_DRIVE )
  {
      if ( action == false )
      {
        current_state = 0;
        gpio_set_level(DIP_C, 0);
        timer_new(paper_time, 0);
      }
      else
      {
        current_state = ! current_state;
        gpio_set_level(DIP_C, current_state);
        timer_new(paper_time, ONE_SECOND * json_step_time / 1000);
      }

  }

  if ( json_mfs_hold_d == STEPPER_DRIVE )
  {
      if ( action == false )
      {
        current_state = 0;
        gpio_set_level(DIP_D, 0);
        timer_new(paper_time, 0);
      }
      else
      {
        current_state = ! current_state;
        gpio_set_level(DIP_D, current_state);
        timer_new(paper_time, ONE_SECOND * json_step_time / 1000);
      }
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
  printf("\r\nDigital test");

/*
 * Read in the fixed digital inputs
 */
  printf("\r\nTime: %4.2fs", (float)(esp_timer_get_time()/1000000));
  printf("\r\nDIP: 0x%02X", read_DIP()); 
  printf("\r\nDone\r\n");

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
  printf("\r\nStatus LED test");
  timer_delay(2*ONE_SECOND);
  set_status_LED("RRR");
  timer_delay(ONE_SECOND);
  set_status_LED("GGG");
  timer_delay(ONE_SECOND);
  set_status_LED("BBB");
  timer_delay(ONE_SECOND);
  set_status_LED("WWW");
  timer_delay(ONE_SECOND);
  set_status_LED("RGB");
  timer_delay(ONE_SECOND);
  set_status_LED("rgb");
  timer_delay(5*ONE_SECOND);         // Blink for 5 seconds
  set_status_LED(LED_READY);
  printf("\r\nDone\r\n");
  return;
}

/*----------------------------------------------------------------
 * 
 * @function: rapid_LED_test()
 * 
 * @brief:    Cycle the status LEDs
 * 
 * @return:   Nothing
 * 
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/
void rapid_LED_test(void)
{
  printf("\r\nRapid LED test\r\n");
  gpio_set_direction(HOLD_C_GPIO,  GPIO_MODE_OUTPUT);
  gpio_set_pull_mode(HOLD_C_GPIO,  GPIO_PULLUP_PULLDOWN);
  gpio_set_direction(HOLD_D_GPIO,  GPIO_MODE_OUTPUT);
  gpio_set_pull_mode(HOLD_D_GPIO,  GPIO_PULLUP_PULLDOWN);

  json_mfs_hold_d = RAPID_RED;          // Hold D
  json_mfs_hold_c = RAPID_GREEN;        // Hold C
  json_mfs_select_cd = RAPID_LOW;       // Select C and D operation
  
  while (1)
  {
    rapid_red(0);
    rapid_green(0);
    timer_delay(ONE_SECOND);

    rapid_red(1);
    rapid_green(0);
    timer_delay(ONE_SECOND);

    rapid_red(0);
    rapid_green(1);
    timer_delay(ONE_SECOND);

    rapid_red(1);
    rapid_green(1);
    timer_delay(ONE_SECOND);
  }
  printf("\r\nDone\r\n");
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
 *  functions drive_paper() or drive_paper_tick().
 * 
 *--------------------------------------------------------------*/
void paper_test(void)
{
  volatile unsigned long time_delay;
  int i;

  printf("\r\nAdvancing paper 500 ms at a time");
  for (i=0; i != 10; i++)
  {
    printf("  %d+", (i+1));
    paper_on_off(true, ONE_SECOND/2);
    timer_delay(ONE_SECOND / 2);
    printf("-");
    paper_on_off(false, 0);
    timer_delay(ONE_SECOND / 2);
  }

  printf("\r\nDone\r\n");

  return;
}
