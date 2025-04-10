/*----------------------------------------------------------------
 *
 * mfs.c
 *
 * Multifunction Switch support
 *
 *-------------------------------------------------------------*/
#include "driver/gpio.h"
#include "stdio.h"
#include "nvs.h"
#include "esp_random.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "nonvol.h"
#include "serial_io.h"
#include "timer.h"
#include "mfs.h"
#include "ota.h"

/*
 *  Definitions
 */
#define LONG_PRESS    (ONE_SECOND)
#define TAP_1_PENDING 0x01
#define TAP_2_PENDING 0x02
#define TAP_MASK_1    0x04
#define TAP_MASK_2    0x08
#define HOLD_MASK_1   0x10
#define HOLD_MASK_2   0x20
#define HOLD_MASK_12  (HOLD_MASK_1 | HOLD_MASK_2)
#define SWITCH_VALID  0x80

/*
 * Function Prototypes
 */

static void sw_state(unsigned int action); // Carry out the MFS function
static void mfs_on(void);                  // Functions to carry out mfs actions.
static void mfs_paper_feed(void);
static void mfs_paper_shot(void);
static void mfs_off(void);
static void mfs_led_adjust(void);
static void mfs_pc_test(void);

/*
 * Variables
 */
static unsigned int switch_state;                // What switches are pressed
const mfs_action_t  mfs_action[] = {
    {TARGET_ON,      mfs_on,         "TARGET_ON"     }, // Take the target out of sleep
    {PAPER_FEED,     mfs_paper_feed, "PAPER FEED"    }, // Feed paper until button released
    {LED_ADJUST,     mfs_led_adjust, "LED_ADJUST"    }, // Adjust LED brighness
    {PAPER_SHOT,     mfs_paper_shot, "PAPER SHOT"    }, // Advance paper the distance of one shot
    {PC_TEST,        mfs_pc_test,    "PC TEST"       }, // Send a test shot to the PC
    {TARGET_OFF,     mfs_off,        "TARGET OFF"    }, // Turn the target on or off
    {NO_ACTION,      NULL,           "NO ACTION"     }, // No action on C & D inputs
    {TARGET_TYPE,    NULL,           "TARGET TYPE"   }, // Put the target type into the send score
    {SHOOTER_LEVEL,  NULL,           "SHOOTER_LEVEL" }, // Shooter experiance level
    {RAPID_RED,      NULL,           "RAPID RED"     }, // The output is used to drive the RED rapid fire LED
    {RAPID_GREEN,    NULL,           "RAPID_GREEN"   }, // The output is used to drive the GREEN rapid fire LED
    {RAPID_LOW,      NULL,           "RAPID LOW"     }, // The output is active low
    {RAPID_HIGH,     NULL,           "RAPID HIGH"    }, // The output is active high
    {STEPPER_DRIVE,  NULL,           "STEPPER_DRIVE" }, // The output is used to drive stepper motor
    {STEPPER_ENABLE, NULL,           "STEPPER_ENABLE"}, // The output is used to drive stepper motor enable
    {0,              0,              0               }
};

/*
 * Test Vectors
  {"MFS_HOLD_12":2, "MFS_TAP_2": 0, "MFS_TAP_1":3 , "MFS_HOLD_2": 5, "MFS_HOLD_1":1, "MFS_HOLD_D":9, "MFS_HOLD_C":9, "MFS_SELECT_CD":9,
 "ECHO":0}
  {"MFS_HOLD_12":2}
  {"MFS_TAP_2":0}
  {"MFS_TAP_1":4}
  {"MFS_HOLD_2":5}
  {"MFS_HOLD_1":1}
  {"MFS_HOLD_D":9}
  {"MFS_HOLD_C":9}
  {"MFS_SELECT_CD":9}
  {"ECHO":0}

*/

/*-----------------------------------------------------
 *
 * @function: multifunction_init
 *
 * @brief:    Setup the MFS
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Program the GPIO outputs depending on the setup
 * entered into MFS2
 *
 * Then continue to look at DIP A and DIP B to perform
 * any initialization functions
 *
 *-----------------------------------------------------*/
void multifunction_init(void)
{

  DLT(DLT_INFO, SEND(ALL, sprintf(_xs, "Multifunction_init()");))

  /*
   * Check to see if the DIP switch has been overwritten
   */
  if ( json_mfs_hold_c >= RAPID_RED )
  {
    gpio_set_direction(HOLD_C_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLD_C_GPIO, GPIO_PULLUP_PULLDOWN);
    switch ( json_mfs_hold_c )
    {
      case RAPID_RED:
        rapid_red(0);
        break;
      case RAPID_GREEN:
        rapid_green(0);
        break;
      case STEPPER_DRIVE:
        gpio_set_level(HOLD_C_GPIO, 0);
        break;
      case STEPPER_ENABLE:
        gpio_set_level(HOLD_C_GPIO, 0);
        break;
    }
  }

  if ( json_mfs_hold_d >= RAPID_RED )
  {
    gpio_set_direction(HOLD_D_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLD_D_GPIO, GPIO_PULLUP_PULLDOWN);
    switch ( json_mfs_hold_d )
    {
      case RAPID_RED:
        rapid_red(0);
        break;
      case RAPID_GREEN:
        rapid_green(0);
        break;
      case STEPPER_DRIVE:
        gpio_set_level(HOLD_D_GPIO, 0);
        break;
      case STEPPER_ENABLE:
        gpio_set_level(HOLD_D_GPIO, 0);
        break;
    }
  }

  /*
   * Continue to read the DIP switch
   */
  if ( DIP_SW_A && DIP_SW_B ) // Both switches closed?
  {
    factory_nonvol(false);    // Initalize the nonvol but do not calibrate
    esp_restart();
  }

  /*
   * The initialization override has been finished
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: multifunction_switch_tick
 *
 * @brief:    Determine the operation of the switched
 *
 * @return:   Switch state
 *
 *-----------------------------------------------------
 *
 * This is called every 100 ms to monitor the action of
 * the multifunction switches.
 *
 * Possible outcomes are:
 *  PENDING - The switch has been pressed, but the outcome is unknown
 *  TAP - The switch has been released within ONE_SECOND and is a tap
 *  HOLD - The switch has been held for more than ONE_SECOND
 *
 *-----------------------------------------------------*/
void multifunction_switch_tick(void)
{
  static unsigned int switch_A_count;
  static unsigned int switch_B_count; // How long has the switch been pressed

  IF_NOT(IN_OPERATION)
  return;

  if ( switch_state & SWITCH_VALID )
  {
    return; // Stop checking once we have a valid switch
  }

  /*
   * Figure out what switches are pressed
   */
  if ( DIP_SW_A )
  {
    switch_A_count++;                  // One more tick
    if ( switch_A_count < LONG_PRESS ) // Still not long enought for a hold
    {
      switch_state |= TAP_1_PENDING;   // It must be pending
    }
    else
    {
      switch_state &= ~TAP_1_PENDING;
      switch_state |= HOLD_MASK_1 | SWITCH_VALID;
      switch_A_count = LONG_PRESS;
    }
  }
  else // Released the switch,  See if it was a tap
  {
    if ( switch_state & TAP_1_PENDING )
    {
      switch_state &= ~TAP_1_PENDING;
      switch_state |= TAP_MASK_1 | SWITCH_VALID;
    }
    switch_A_count = 0;
  }

  if ( DIP_SW_B )
  {
    switch_B_count++;
    if ( switch_B_count < LONG_PRESS )
    {
      switch_state |= TAP_2_PENDING;
    }
    else
    {
      switch_state &= ~TAP_2_PENDING;
      switch_state |= HOLD_MASK_2 | SWITCH_VALID;
      switch_B_count = LONG_PRESS;
    }
  }
  else // Released the switch, seeif it was a tap
  {
    if ( switch_state & TAP_2_PENDING )
    {
      switch_state &= ~TAP_2_PENDING;
      switch_state |= TAP_MASK_2 | SWITCH_VALID;
    }
    switch_B_count = 0;
  }

  /*
   *  Look for the special case where there is HOLD_12, but
   *  there is some kind of bounce
   */
  if ( (switch_state & (HOLD_MASK_1 | HOLD_MASK_2)) && (switch_state & (TAP_MASK_1 | TAP_MASK_2)) )
  {
    switch_state |= (HOLD_MASK_1 | HOLD_MASK_2);
    switch_state &= ~(TAP_MASK_1 | TAP_MASK_2);
  }

  /*
   *  All done
   */
  return;
}
/*-----------------------------------------------------
 *
 * @function: multifunction_switch
 *
 * @brief:    Carry out the functions of the multifunction switch
 *
 * @return:   Switch state
 *
 *-----------------------------------------------------
 *
 * The actions of the DIP switch will change depending on the
 * mode that is programmed into it.
 *
 * For example  some of the DIP switches, tapping the switch
 * turns the LEDs on, and holding it will carry out
 * the alternate activity.
 *
 * The variable switch state is a bit mask of the current
 * switch state, for example SWITCH 1 has been tapped.
 *
 * Switch state is computed in
 *
 *-----------------------------------------------------*/

void multifunction_switch(void)
{
  unsigned int action;                      // Action to happen

  IF_NOT(IN_OPERATION)
  return;

  if ( (switch_state & SWITCH_VALID) == 0 ) // Invalid switch state
  {
    return;
  }

  /*
   * Figure out what switches are pressed
   */
  action = switch_state & (~SWITCH_VALID);

  /*
   * Carry out the switch action
   */
  switch ( action )
  {
    case TAP_MASK_1:
      sw_state(json_mfs_tap_1);
      break;

    case TAP_MASK_2:
      sw_state(json_mfs_tap_2);
      break;

    case HOLD_MASK_1:
      sw_state(json_mfs_hold_1);
      break;

    case HOLD_MASK_2:
      sw_state(json_mfs_hold_2);
      break;

    case HOLD_MASK_12:
      sw_state(json_mfs_hold_12);
      break;

    default:
      break;
  }

  /*
   * All done, return the GPIO state
   */
  switch_state = 0;
  return;
}

/*-----------------------------------------------------
 *
 * @function: sw_state
 *
 * @brief:    Switch to MFS function and execute it
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * The switch action is pressing or tapping a particular
 * switch,
 *
 *-----------------------------------------------------*/
static void sw_state(unsigned int action)
{
  mfs_action_t *mfs_ptr;

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "Switch action: %d", action);))

  mfs_ptr = mfs_find(action);
  if ( (mfs_ptr != NULL) && (mfs_ptr->fcn != NULL) )
  {
    (mfs_ptr->fcn)();
  }

  return;
}

static void mfs_on(void)
{
  set_LED_PWM_now(json_LED_PWM);                               // Yes, a quick press to turn the LED on
  vTaskDelay(ONE_SECOND / 2);
  set_LED_PWM_now(0);                                          // Blink
  vTaskDelay(ONE_SECOND / 2);
  set_LED_PWM_now(json_LED_PWM);                               // and leave it on
  power_save = (long)json_power_save * 60L * (long)ONE_SECOND; // and resets the power save time
  json_power_save += 30;

  return;
}

static void mfs_paper_feed(void)                               // Feed paper so long as the switch is pressed
{
  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "mfs_paper_feed()");))

  /*
   *  Advance paper using the DC motor
   */
  if ( IS_DC_WITNESS )
  {
    paper_start();
    while ( DIP_SW_A || DIP_SW_B ) // and loop until the switches are
    {
      vTaskDelay(TICK_10ms);       // released
    }
    paper_stop();
  }

  /*
   *  Advance the paper using the stepper motor
   */
  if ( IS_STEPPER_WITNESS )
  {
    paper_start();
    while ( DIP_SW_A || DIP_SW_B ) // and loop until the switches are
    {
      paper_drive_tick();          // Fake a timer tick since mfs_paper_feed() is blocking
      vTaskDelay(TICK_10ms);       // released
      step_count = 1000;           // Keep the motor moving
    }
    paper_stop();
  }

  /*
   *  End of action
   */
  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, _DONE_);))

  return;
}

static void mfs_paper_shot(void)
{
  paper_start();
  while ( 1 )
  {
    vTaskDelay(10);
    if ( is_paper_on() == 0 )
    {
      break;
    }
  }
  SEND(ALL, sprintf(_xs, "\r\nDone\r\n");)
  return;
}

#define SCALE 1200
static void mfs_pc_test(void)
{
  static unsigned int test_shot = 0;
  int                 temp, sign;

  temp                = esp_random() % (SCALE);
  sign                = ((esp_random() & 1) == 0) ? 1 : -1;
  record[test_shot].x = (float)(sign * temp);
  temp                = esp_random() % (SCALE);
  sign                = ((esp_random() & 1) == 0) ? 1 : -1;
  record[test_shot].y = (float)(sign * temp);
  s_of_sound          = speed_of_sound(temperature_C(), humidity_RH());
  prepare_score(&record[test_shot], test_shot, NOT_MISSED_SHOT);
  test_shot++;

  return;
}

static void mfs_off(void)
{
  bye(true); // Stay in the Bye state until a wake up event comes along
  return;
}

static void mfs_led_adjust(void)
{
  unsigned int led_step;

  led_step = 5;

  json_LED_PWM += led_step; // Bump up the LED by 5%

  if ( json_LED_PWM > 100 )
  {
    json_LED_PWM = 0;
  }

  DLT(DLT_DEBUG, SEND(ALL, sprintf(_xs, "mfs_led_adjust: %d%%", json_LED_PWM);))
  set_LED_PWM_now(json_LED_PWM); // Set the brightness
  nvs_set_i32(my_handle, NONVOL_LED_PWM, json_LED_PWM);
  nvs_commit(my_handle);

  vTaskDelay(ONE_SECOND);        // Give it time for the LEDs to stabilize

  return;
}

/*-----------------------------------------------------
 *
 * @function: mfs_find
 *
 * @brief:    Find the structure corresponding to the index
 *
 * @return:   Pointer mfs_structure
 *
 *-----------------------------------------------------
 *
 * Returns the text string for the switch so that it can
 * be used.
 *
 *-----------------------------------------------------*/

mfs_action_t *mfs_find(unsigned int action // Switch to be displayed
)
{
  unsigned int i;

  i = 0;
  while ( mfs_action[i].text != NULL )
  {
    if ( mfs_action[i].index == action )
    {
      return &mfs_action[i];
    }
    i++;
  }

  return NULL;
}

/*-----------------------------------------------------
 *
 * @function: mfs_show
 *
 * @brief:    Show what the MFS settings are
 *
 * @return:   None
 *
 *-----------------------------------------------------
 *
 * Returns the text string for the switch so that it can
 * be dislayed.
 *
 *-----------------------------------------------------*/

void mfs_show(void)
{
  unsigned int i;

  i = 0;
  while ( mfs_action[i].text != NULL )
  {
    SEND(ALL, sprintf(_xs, "\r\n%d: %s", mfs_action[i].index, mfs_action[i].text);)
    i++;
  }

  SEND(ALL, sprintf(_xs, "\r\n");)
  return;
}
