/*----------------------------------------------------------------
 * 
 * freETarget        
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
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
#define TAP_A         0x04
#define TAP_B         0x08
#define HOLD_A        0x10
#define HOLD_B        0x20
#define HOLD_AB       (HOLD_A | HOLD_B)
#define SWITCH_VALID  0x80

/*
 * Function Prototypes 
 */
static void send_fake_score(void);
static void sw_state(unsigned int action);      // Carry out the MFS function

/*
 * Variables 
 */
static unsigned int dip_mask;                   // Output to the DIP port if selected
static unsigned int switch_state;               // What switches are pressed

/*-----------------------------------------------------
 * 
 * @function: multifunction_init
 * 
 * @brief:    Use the multifunction switches during starup
 * 
 * @return:   None
 * 
 *-----------------------------------------------------
 * 
 * Read the jumper header and modify the initialization
 * 
 *-----------------------------------------------------*/
#define HOLDC_GPIO GPIO_NUM_36
#define HOLDD_GPIO GPIO_NUM_35

 void multifunction_init(void)
 {
  unsigned int dip;

/*
 * Check to see if the DIP switch has been overwritten
 */
  if ( (HOLD3(json_multifunction2) == RAPID_RED) 
        || (HOLD3(json_multifunction2) == RAPID_GREEN))
  {
    gpio_set_direction(HOLDC_GPIO,  GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLDC_GPIO,  GPIO_PULLUP_PULLDOWN);
    gpio_set_level(HOLDC_GPIO, 0);
  }

  if ( (HOLD4(json_multifunction2) == RAPID_RED) 
        || (HOLD4(json_multifunction2) == RAPID_GREEN))
  {
    gpio_set_direction(HOLDD_GPIO,  GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(HOLDD_GPIO,  GPIO_PULLUP_PULLDOWN);
    gpio_set_level(HOLDD_GPIO, 0);
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
      switch_state |= HOLD_A | SWITCH_VALID;
      switch_A_count = LONG_PRESS;
      set_status_LED(LED_MFS_A);
    }
  }
  else  // Released the switch,  See if it was a tap
  {
    if ( switch_state & TAP_A_PENDING )
    {
      switch_state |= TAP_A | SWITCH_VALID;
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
      switch_state |= HOLD_B | SWITCH_VALID;
      switch_B_count = LONG_PRESS;
      set_status_LED(LED_MFS_B);
    }
  }
  else    // Released the switch, seeif it was a tap
  {
    if ( switch_state & TAP_B_PENDING )
    {
      switch_state |= SWITCH_VALID | TAP_B;
      switch_state &= ~TAP_B_PENDING;
    }
    switch_B_count = 0;
  }

/*
 *  Look for the special case where there is HOLD_12, but 
 *  there is some kind of bounce
 */
  if ( (switch_state & (HOLD_A | HOLD_B))
        && (switch_state & (TAP_A | TAP_B )) )
  {
      switch_state |= (HOLD_A | HOLD_B);
      switch_state &= ~(TAP_A | TAP_B);
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
 * MFS_TAP1\": \"%s\",\n\r\"MFS_TAP2\": \"%s\",\n\r\"MFS_HOLD1\": \"%s\",\n\r\"MFS_HOLD2\": \"%s\",\n\r\"MFS_HOLD12\": \"%s\",\n\r", 
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
  if ( switch_state & HOLD_A )
  {
    if ( HOLD1(json_multifunction) == TARGET_TYPE ) 
    {
      sw_state(HOLD1(json_multifunction));
      action = 0;
    }
  }

  if ( switch_state & HOLD_B )
  {
    if ( HOLD2(json_multifunction) == TARGET_TYPE ) 
    {
      sw_state(HOLD2(json_multifunction));
      action = 0;
    }
  }  
  
/*
 * Carry out the switch action
 */
  switch (action)
  {
    case TAP_A:
      sw_state(TAP1(json_multifunction));
      break;

    case TAP_B:
      sw_state(TAP2(json_multifunction));
      break;

    case HOLD_A:
      sw_state(HOLD1(json_multifunction));
      break;

    case HOLD_B:
      sw_state(HOLD2(json_multifunction));
      break;

    case HOLD_AB:
      sw_state(HOLD12(json_multifunction));
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

/*
 * Carry out an action based on the switch state
 */
static void sw_state 
    (
    unsigned int action
    )
{     
  unsigned int led_step;

  DLT(DLT_INFO, printf("Switch action: %d", action);)

/*
 *  Act on the MFS setting(s) 
 */
  switch (action)
  {
    case POWER_TAP:
      set_LED_PWM_now(json_LED_PWM);      // Yes, a quick press to turn the LED on
      vTaskDelay(ONE_SECOND/2),
      set_LED_PWM_now(0);                 // Blink
      vTaskDelay(ONE_SECOND/2);
      set_LED_PWM_now(json_LED_PWM);      // and leave it on
      power_save = (long)json_power_save * 60L * (long)ONE_SECOND; // and resets the power save time
      json_power_save += 30;
      break;
        
    case PAPER_FEED:                      // Turn on the paper untill the switch is pressed again 
      DLT(DLT_INFO, printf("\r\nAdvancing paper");)
      paper_on_off(true, 10 * ONE_SECOND);
      while ( DIP_SW_A || DIP_SW_B )
      {
        timer_delay(1);
      }
      paper_on_off(false, 0);
      set_status_LED(LED_MFS_OFF);
      DLT(DLT_INFO, printf("\r\nDone");)
      break;

    case PAPER_SHOT:                      // The switch acts as paper feed control
      drive_paper();                      // Turn on the paper drive
      break;
      
    case PC_TEST:                         // Send a fake score to the PC
      send_fake_score();
      break;
      
    case ON_OFF:                         // Turn the target off
      bye();                             // Stay in the Bye state until a wake up event comes along
      break;
      
    case LED_ADJUST:
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
      break;

    case TARGET_TYPE:                     // Over ride the target type if the switch is closed
      json_target_type = 0;
      if (HOLD1(json_multifunction) == TARGET_TYPE) // If the switch is set for a target type
      {
        if ( DIP_SW_A )
        {
          json_target_type = 1;           // 
        }
      }
      if (HOLD2(json_multifunction) == TARGET_TYPE) 
      {
        if ( DIP_SW_B )
        {
          json_target_type = 1;
        }
      }
      break;
      
    default:
      break;
  }

/*
 * All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * @function: multifunction_hold12()
 *            multifunction_hold2()
 *            multifunction_hold1()
 *            multifunction_tap2()
 *            multifunction_tap1()
 * 
 * @brief:    Modify the individual filelds
 * 
 * @return:   mfs updated with the new field
 * 
 *-----------------------------------------------------
 *
 * The MFS is encoded as a 3 digit packed BCD number
 * 
 * This function unpacks the numbers and displayes it as
 * text in a JSON message.
 * 
 *-----------------------------------------------------*/ 
unsigned int multifunction_common
(
  unsigned int newMFS,
  unsigned int place,
  unsigned int oldMFS
)
{
  unsigned int x;

  newMFS %= 10;
  x = json_multifunction;
  x -= oldMFS * place;
  x += newMFS * place;
  return x;

}

unsigned int multifunction_hold12
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_HOLD12, HOLD12(json_multifunction));
}

unsigned int multifunction_hold2
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_HOLD2, HOLD2(json_multifunction));
}

unsigned int multifunction_hold1
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_HOLD1, HOLD1(json_multifunction));
}

unsigned int multifunction_tap2
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_TAP2, TAP2(json_multifunction));
}

unsigned int multifunction_tap1
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_TAP1,TAP1(json_multifunction));
}

unsigned int multifunction_hold3
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_HOLD12, HOLD3(json_multifunction2));
}

unsigned int multifunction_hold4
(
  unsigned int newMFS         // New field value
)
{
  return multifunction_common(newMFS, SHIFT_HOLD12, HOLD4(json_multifunction2));
}

/*-----------------------------------------------------
 * 
 * @function: multifunction_show()
 * 
 * @brief:    Display the MFS values
 * 
 * @return:   None
 * 
 *-----------------------------------------------------
 *
 * The MFS is encoded as a 3 digit packed BCD number
 * 
 * This function unpacks the numbers and displayes it as
 * text in a JSON message.
 * 
 *-----------------------------------------------------*/ 
 //                             0            1            2             3            4             5            6    7    8          9
static char* mfs_text[] = { "WAKE_UP", "PAPER_FEED", "ADJUST_LED", "PAPER_SHOT", "PC_TEST",  "POWER_ON_OFF",   "6", "7", "8", "TARGET_TYPE"};

 //                              0           1               2             3            4             5            6    7    8          9
static char* mfs2_text[] = { "UNUSED", "TARGET SELECT", "RAPID_RED", "RAPID_GREEN",    "3",         "4",          "5",   "      6", "7", "8",       "9"};

void multifunction_show(unsigned int x)
{
  int i;

  SEND(sprintf(_xs, "\n\r");) 
  for (i=0; i <= 9; i++)
  {
    SEND(sprintf(_xs, "\"%d:%s\",\n\r", i, mfs_text[i]);) 
  }


/*
 * All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * @function: multifunction_str
 * 
 * @brief:    Return the text string for this function
 * 
 * @return:   Pointer to text
 * 
 *-----------------------------------------------------
 *
 * Returns the text string for the switch so that it can
 * be dislayed.
 * 
 *-----------------------------------------------------*/

char *multifunction_str
(
  unsigned int mfs_function     // Switch to be displayed
)
{
  return mfs_text[mfs_function]; // Return a pointer to the string
}

char *multifunction_str_2
(
  unsigned int mfs_function     // Switch to be displayed
)
{
  return mfs2_text[mfs_function]; // Return a pointer to the string
}
/*----------------------------------------------------------------
 * 
 * @function: send_fake_score
 * 
 * @brief: Send a fake score to the PC for testing
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
static void send_fake_score(void) 
{ 
  static   shot_record_t shot;
    
  shot.x = esp_random() % (5000);
  shot.y = esp_random() % (5000);
  s_of_sound = speed_of_sound(temperature_C(), humidity_RH());
  shot.shot_number++;
  send_score(&shot);
  return;
} 