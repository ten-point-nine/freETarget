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
#define SWITCH_A_TAP   1
#define SWITCH_A_HOLD  2
#define SWITCH_B_TAP   3
#define SWITCH_B_HOLD  4
#define SWITCH_AB_HOLD 5

/*
 * Function Prototypes 
 */
static void send_fake_score(void);
static void sw_state(unsigned int action);      // Carry out the MFS function

/*
 * Variables 
 */
static unsigned int dip_mask;                   // Output to the DIP port if selected
static unsigned int switch_A_count;
static unsigned int switch_B_count;             // How long has the switch been pressed

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
 void multifunction_init(void)
 {
  unsigned int dip;

/*
 * Check to see if the DIP switch has been overwritten
 */
  if ( (HOLD1(json_multifunction2) == RAPID_RED) 
        || (HOLD1(json_multifunction2) == RAPID_GREEN))
  {
      gpio_set_level(DIP_A, 1);
      dip_mask = RED_MASK;
  }

  if (  (HOLD2(json_multifunction2) == RAPID_RED)
      || (HOLD2(json_multifunction2) == RAPID_GREEN ) )
  {
      gpio_set_level(DIP_C, 1);
      dip_mask |= GREEN_MASK;
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
void multifunction_switch_tick(void)
{
  IF_NOT(IN_OPERATION) return;

/*
 * Figure out what switches are pressed
 */
   if ( DIP_SW_A )
   { 
     switch_A_count++;
     if ( switch_A_count < LONG_PRESS)
     {
      set_status_LED(LED_MFS_a);
     }
     else
     {
      set_status_LED(LED_MFS_A);
     }
   }

   if ( DIP_SW_B )
   { 
     switch_B_count++;
     if ( switch_B_count < LONG_PRESS)
     {
      set_status_LED(LED_MFS_b);
     }
     else
     {
      set_status_LED(LED_MFS_B);
     }
   }  
/*
 *  All done
 */   
  return;
 }


void multifunction_switch(void)
{
  unsigned int  action;               // Action to happen

  IF_NOT(IN_OPERATION) return;

  if ( DIP_SW_A || DIP_SW_B )           // Do nothing if the switches are pressed
  {
    return;
  }

/*
 * Figure out what switches are pressed
 */
  action = 0;
  if ( switch_A_count != 0 )
  {
    action = SWITCH_A_TAP;
    if ( switch_A_count >= LONG_PRESS )
    {
      action = SWITCH_A_HOLD;
      if ( HOLD1(json_multifunction) == TARGET_TYPE ) 
      {
        sw_state(HOLD1(json_multifunction));
        action = 0;
      }
    }
  }
  
  if ( switch_B_count != 0 )
  {
    action = SWITCH_B_TAP;
    if ( switch_B_count >= LONG_PRESS )
    {
      action = SWITCH_B_HOLD;
      if ( HOLD2(json_multifunction) == TARGET_TYPE ) 
      {
        sw_state(HOLD2(json_multifunction));
        action = 0;
      }
    }
  }

  if ( (switch_A_count >= LONG_PRESS) && (switch_B_count >= LONG_PRESS) )
  {
    action = SWITCH_AB_HOLD;
  }

  if ( action == 0 )                 // Nothing to do
  {
    return;
  }
   
/*
 * Carry out the switch action
 */
  switch (action)
  {
    case SWITCH_A_TAP:
      sw_state(TAP1(json_multifunction));
      break;

    case SWITCH_B_TAP:
      sw_state(TAP2(json_multifunction));
      break;

    case SWITCH_A_HOLD:
      sw_state(HOLD1(json_multifunction));
      break;

    case SWITCH_B_HOLD:
      sw_state(HOLD2(json_multifunction));
      break;

    case SWITCH_AB_HOLD:
      sw_state(HOLD12(json_multifunction));
      break;
  }
  
/*
 * All done, return the GPIO state
 */
  switch_A_count = 0;
  switch_B_count = 0;
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

  char s[128];                          // Holding string 
  
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
      sprintf(s, "\r\n{\"LED_PWM\": %d}\n\r", json_power_save);
      serial_to_all(s, ALL);  
      break;
        
    case PAPER_FEED:                      // Turn on the paper untill the switch is pressed again 
      DLT(DLT_INFO, printf("\r\nAdvancing paper");)
      paper_on_off(true);
      while ( (DIP_SW_A == 0) && (DIP_SW_B == 0))
      {
        timer_delay(1);
      }
      paper_on_off(false);
      switch_A_count = 0;                 // Reset the LEDs if they are on
      switch_B_count = 0;
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
      while ( (DIP_SW_A || DIP_SW_B) )    // Keep it on while the switches are pressed 
      {
        json_LED_PWM += led_step;         // Bump up the LED by 5%
        if ( json_LED_PWM > 100 )
        {
          json_LED_PWM = 100;
          led_step = -5;                 // Force to zero on wrap around
        }
        if ( json_LED_PWM <0 )
        {
          json_LED_PWM = 0;
          led_step = +5;                 // Force to zero on wrap around
        }
        set_LED_PWM_now(json_LED_PWM);   // Set the brightness
        sprintf(s, "\r\n{\"LED_BRIGHT\": %d}\n\r", json_LED_PWM);
        serial_to_all(s, ALL);
        vTaskDelay(ONE_SECOND/4);
      }
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
 * @function: multifunction_display
 * 
 * @brief:    Display the MFS settings as text
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

 //                              0           1            2             3            4             5            6    7    8          9
static char* mfs2_text[] = { "DEFAULT", "RAPID_RED", "RAPID_GREEN",    "3",         "4",          "5",   "      6", "7", "8",       "9"};

void multifunction_display(void)
{
  char s[256];                          // Holding string

  sprintf(s, "\"MFS_TAP1\": \"%s\",\n\r\"MFS_TAP2\": \"%s\",\n\r\"MFS_HOLD1\": \"%s\",\n\r\"MFS_HOLD2\": \"%s\",\n\r\"MFS_HOLD12\": \"%s\",\n\r", 
  mfs_text[TAP1(json_multifunction)], mfs_text[TAP2(json_multifunction)], mfs_text[HOLD1(json_multifunction)], mfs_text[HOLD2(json_multifunction)], mfs_text[HOLD12(json_multifunction)]);
  serial_to_all(s, ALL);  

  sprintf(s, "\"MFS_CONFIG\": \"%s\",\n\r\"MFS_DIAG\": \"%s\",\n\r", 
  mfs2_text[HOLD1(json_multifunction2)], mfs2_text[HOLD2(json_multifunction2)]);
  serial_to_all(s, ALL);  
  
/*
 * All done, return
 */
  return;
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
    
  shot.x = 0 ; // esp_random() % (json_sensor_dia/2.0);
  shot.y = 0;
  shot.shot_number++;
  send_score(&shot);

  return;
} 