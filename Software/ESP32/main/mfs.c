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

/*
 *  Definitions
 */
#define LONG_PRESS (ONE_SECOND)
#define TAP_A_PENDING 0x01
#define TAP_B_PENDING 0x02
#define TAP_MASK_A    0x04
#define TAP_MASK_B    0x08
#define HOLD_MASK_A   0x10
#define HOLD_MASK_B   0x20
#define HOLD_MASK_AB  (HOLD_MASK_A | HOLD_MASK_B)
#define SWITCH_VALID  0x80

/*
 * Function Prototypes 
 */

static void sw_state(unsigned int action);      // Carry out the MFS function
static void mfs_power_tap (void);                               // Functions to carry out mfs actions.
static void mfs_paper_feed(void);
static void mfs_paper_shot(void);
static void mfs_on_off(void);
static void mfs_led_adjust(void);
static void mfs_pc_test(void);
static void mfs_on_off(void);

/*
 * Variables 
 */
static unsigned int switch_state;               // What switches are pressed
mfs_action_t mfs_action[] = {
  { POWER_TAP,      mfs_power_tap,  "WAKE UP"        },// Take the target out of sleep
  { PAPER_FEED,     mfs_paper_feed, "PAPER FEED"     },// Feed paper until button released
  { LED_ADJUST,     mfs_led_adjust, "LED_ADJUST"     },// Adjust LED brighness
  { PAPER_SHOT,     mfs_paper_shot, "PAPER SHOT"     },// Advance paper the distance of one shot 
  { PC_TEST,        mfs_pc_test,    "PC TEST"        },// Send a test shot to the PC
  { ON_OFF,         mfs_on_off,     "TARGET ON OFF"  },// Turn the target on or off
  { NO_ACTION,      NULL,           "NO ACTION"      },// No action on C & D inputs
  { TARGET_TYPE,    NULL,           "TARGET TYPE"    },// Put the target type into the send score
  { RAPID_RED,      NULL,           "RAPID RED"      },// The output is used to drive the RED rapid fire LED
  { RAPID_GREEN,    NULL,           "RAPID_GREEN"    },// The output is used to drive the GREEN rapid fire LED
  { RAPID_LOW,      NULL,           "RAPID LOW"      },// The output is active low
  { RAPID_HIGH,     NULL,           "RAPID HIGH"     },// The output is active high
  { STEPPER_DRIVE,  NULL,           "STEPPER_DRIVE"  },// The output is used to drive stepper motor
  { 0, 0, 0 }
};

/*
 * Test Vectors
 *
  {"MFS_HOLD_AB":2, "MFS_TAP_B": 0, "MFS_TAP_A":3 , "MFS_HOLD_B": 5, "MFS_HOLD_A":1, "MFS_HOLD_D":5, "MFS_HOLD_C":6, "MFS_SELECT_CD":1, "ECHO":0}
  {"MFS_HOLD_AB":2}
  {"MFS_TAP_B":0}
  {"MFS_TAP_A":3}
  {"MFS_HOLD_B":5}
  {"MFS_HOLD_A":1}
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
  unsigned int dip;

/*
 * Check to see if the DIP switch has been overwritten
 */
  if ( HOLD_C(json_multifunction2) > MFS2_DIP ) 
  {
    gpio_set_direction(HOLD_C_GPIO,  GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLD_C_GPIO,  GPIO_PULLUP_PULLDOWN);
    gpio_set_level(HOLD_C_GPIO, 0);
  }

  if ( HOLD_D(json_multifunction2) > MFS2_DIP) 
  {
    gpio_set_direction(HOLD_D_GPIO,  GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLD_D_GPIO,  GPIO_PULLUP_PULLDOWN);
    gpio_set_level(HOLD_D_GPIO, 0);
  }


/*
 * Continue to read the DIP switch
 */
  dip = read_DIP();                     // Read the jumper header

  if ( dip == 0 )                       // No jumpers in place
  { 
    return;                             // Carry On
  }

  if ( DIP_SW_A && DIP_SW_B )           // Both switches closed?
  {
    factory_nonvol(false);              // Initalize the nonvol but do not calibrate
  }

  else
  {
    if ( DIP_SW_A )                     // Switch A pressed
    {
      is_trace = 10;                    // Turn on tracing
    }
  
    if ( DIP_SW_B )                     // Switch B pressed
    {

    }
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
 * The function polls the switches and keeps track of
 * how long the switch has been closed for.
 * 
 * Turn on the coloured LEDs based on how long the switch
 * has been closed for.
 * 
 *-----------------------------------------------------*/
void multifunction_switch_tick(void)
{
  static unsigned int switch_A_count;
  static unsigned int switch_B_count;             // How long has the switch been pressed

  IF_NOT(IN_OPERATION) return;

  if ( switch_state & SWITCH_VALID )
  {
    return;       // Stop checking once we have a valid switch
  }

/*
 * Figure out what switches are pressed
 */
   if ( DIP_SW_A )
   { 
    switch_A_count++;
    if ( switch_A_count < LONG_PRESS)
    {
      switch_state |= TAP_A_PENDING;
      set_status_LED(LED_MFS_a);
    }
    else
    {
      switch_state &= ~TAP_A_PENDING;
      switch_state |= HOLD_MASK_A | SWITCH_VALID;
      switch_A_count = LONG_PRESS;
      set_status_LED(LED_MFS_A);
    }
  }
  else  // Released the switch,  See if it was a tap
  {
    if ( switch_state & TAP_A_PENDING )
    {
      switch_state |= TAP_MASK_A | SWITCH_VALID;
      switch_state &= ~TAP_A_PENDING;
    }
    switch_A_count = 0;
  }

  if ( DIP_SW_B )
  { 
    switch_B_count++;
    if ( switch_B_count < LONG_PRESS)
    {
      switch_state |= TAP_B_PENDING;
      set_status_LED(LED_MFS_b);
    }
    else
    {
      switch_state &= ~TAP_B_PENDING;
      switch_state |= HOLD_MASK_B | SWITCH_VALID;
      switch_B_count = LONG_PRESS;
      set_status_LED(LED_MFS_B);
    }
  }
  else    // Released the switch, seeif it was a tap
  {
    if ( switch_state & TAP_B_PENDING )
    {
      switch_state |= TAP_MASK_B | SWITCH_VALID;
      switch_state &= ~TAP_B_PENDING;
    }
    switch_B_count = 0;
  }

/*
 *  Look for the special case where there is HOLD_12, but 
 *  there is some kind of bounce
 */
  if ( (switch_state & (HOLD_MASK_A | HOLD_MASK_B))
        && (switch_state & (TAP_MASK_A | TAP_MASK_B )) )
  {
      switch_state |= (HOLD_MASK_A | HOLD_MASK_B);
      switch_state &= ~(TAP_MASK_A | TAP_MASK_B);
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
 * For some of the DIP switches, tapping the switch
 * turns the LEDs on, and holding it will carry out 
 * the alternate activity.
 * 
 * MFS_TAP_A\": \"%s\",\n\r\"MFS_TAP_B\": \"%s\",\n\r\"MFS_HOLD_A\": \"%s\",\n\r\"MFS_HOLD_B\": \"%s\",\n\r\"MFS_HOLD_AB\": \"%s\",\n\r", 
 * Special Cases
 * 
 * Both switches pressed, Toggle the Tabata State
 * Either switch set for target type switch
 *-----------------------------------------------------*/

void multifunction_switch(void)
{
  unsigned int  action;               // Action to happen

  IF_NOT(IN_OPERATION) return;

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
  switch (action)
  {
    case TAP_MASK_A:
      sw_state(json_mfs_tap_a);
      break;

    case TAP_MASK_B:
      sw_state(json_mfs_tap_b);
      break;

    case HOLD_MASK_A:
      sw_state(json_mfs_hold_a);
      break;

    case HOLD_MASK_B:
      sw_state(json_mfs_hold_b);
      break;

    case HOLD_MASK_AB:
      sw_state(json_mfs_hold_ab);
      break;

    default:
      break;
  }
  
/*
 * All done, return the GPIO state
 */
  switch_state = 0;
  set_status_LED(LED_MFS_OFF);
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
 * This function executes the function defined by the 
 * MFS switch
 * 
 *-----------------------------------------------------*/
static void sw_state 
    (
    unsigned int action
    )
{     
  mfs_action_t* mfs_ptr;

  DLT(DLT_INFO, printf("Switch action: %d", action);)

  mfs_ptr = mfs_find(action);
  if ( (mfs_ptr != NULL) && (mfs_ptr->fcn != NULL) )
  {
    (mfs_ptr->fcn)();
  }

  return;
}

static void mfs_power_tap (void)
{
  set_LED_PWM_now(json_LED_PWM);      // Yes, a quick press to turn the LED on
  vTaskDelay(ONE_SECOND/2),
  set_LED_PWM_now(0);                 // Blink
  vTaskDelay(ONE_SECOND/2);
  set_LED_PWM_now(json_LED_PWM);      // and leave it on
  power_save = (long)json_power_save * 60L * (long)ONE_SECOND; // and resets the power save time
  json_power_save += 30;
  
  return;
}

static void mfs_paper_feed(void)
{
  DLT(DLT_INFO, printf("\r\nAdvancing paper");)
  paper_on_off(true, 10 * ONE_SECOND);
  while ( DIP_SW_A || DIP_SW_B )
  {
    timer_delay(1);
  }
  paper_on_off(false, 0);
  set_status_LED(LED_MFS_OFF);
  
  DLT(DLT_INFO, printf("\r\nDone");)
  
  return;
}

static void mfs_paper_shot(void)
{
  drive_paper();                      // Turn on the paper drive
  return;
}

static void mfs_pc_test(void)
{
  static   shot_record_t shot;
    
  shot.x = esp_random() % (5000);
  shot.y = esp_random() % (5000);
  s_of_sound = speed_of_sound(temperature_C(), humidity_RH());
  shot.shot_number++;
  send_score(&shot);
  return;
} 

static void mfs_on_off(void)
{
  bye();                             // Stay in the Bye state until a wake up event comes along
  return;
}

static void mfs_led_adjust(void)
{
  unsigned int led_step;

  led_step = 5;
  
  json_LED_PWM += led_step;         // Bump up the LED by 5%
  if ( json_LED_PWM > 100 )
  {
    json_LED_PWM = 0;
  }
  set_LED_PWM_now(json_LED_PWM);   // Set the brightness
  vTaskDelay(ONE_SECOND/4);
      
  nvs_set_i32(my_handle, NONVOL_LED_PWM, json_LED_PWM);
  nvs_commit(my_handle);
  
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
 * be dislayed.
 * 
 *-----------------------------------------------------*/

mfs_action_t* mfs_find
(
  unsigned int action     // Switch to be displayed
)
{
  unsigned int i;

  i = 0;
  while (mfs_action[i].text != NULL )
  {
    if (mfs_action[i].index == action )
    {
      return &mfs_action[i];
    }
    i++;
  }

  return NULL;
}

