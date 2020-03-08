/*-------------------------------------------------------
 * 
 * analog_io.ino
 * 
 * General purpose Analog driver
 * 
 * ----------------------------------------------------*/
#include "analog_io.h"

/*----------------------------------------------------------------
 * 
 * void init_analog()
 * 
 * Initialize the analog I/O
 * 
 *----------------------------------------------------------------
 */
void init_analog_io(void)
{
    
  pinMode(REF_OUT, OUTPUT);
  analogWrite(REF_OUT, 0);  // Prime the PWM

/*
 * All done, begin the program
 */
 Serial.print("\n\rReady");
}


/*----------------------------------------------------------------
 * 
 * bool set_reference(void)
 * 
 * Set the referenced voltage.  Return true if adjusted
 * 
 *--------------------------------------------------------------*/
bool last_return = false;
unsigned int pwm_reference, pwm_control;

bool set_reference(void)
{
/*
 * Check the pwm_reference every 1/5 second
 */
  if ( time_out <= micros() )
  {    
    time_out = micros() + 500000;
    pwm_reference = (float)analogRead(REF_IN);
  }
/*
 * Out of tolerence, adjust
 */
    else
    {
      debug_trace();
  
      analogWrite(REF_OUT, (int)pwm_control);
      last_return = true;
    }

/*
 * All done, return
 */
  return last_return;
}
