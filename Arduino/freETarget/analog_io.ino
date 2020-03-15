/*-------------------------------------------------------
 * 
 * analog_io.ino
 * 
 * General purpose Analog driver
 * 
 * ----------------------------------------------------*/
#include "analog_io.h"
#include "gpio.h"

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
 Serial.print("\n\rAnalog I/O Ready");
}

/*----------------------------------------------------------------
 * 
 * unsigned int read_feedback(void)
 * 
 * return the reference voltage
 * 
 *--------------------------------------------------------------*/
unsigned int read_reference(void)
{
  return analogRead(V_REFERENCE);
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
    pwm_reference = (float)analogRead(V_REFERENCE);
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

/*----------------------------------------------------------------
 * 
 * void max_analog
 * 
 * Return the value of the largest analog input
 * 
 *--------------------------------------------------------------*/
uint16_t max_analog(void)
{
  uint16_t return_value;

  return_value = analogRead(NORTH_ANA);

  if ( analogRead(EAST_ANA) > return_value ) 
    return_value = analogRead(EAST_ANA);

  if ( analogRead(SOUTH_ANA) > return_value ) 
    return_value = analogRead(SOUTH_ANA);

  if ( analogRead(WEST_ANA) > return_value ) 
    return_value = analogRead(WEST_ANA);

  return return_value;
  
}
/*----------------------------------------------------------------
 * 
 * void cal_analog
 * 
 * Use the Pots to calibrate the analog input threshold
 * 
 *---------------------------------------------------------------
 *
 * The analog inputs are sampled, and the LEDs display
 * Low, or High, or middle to set the trip reference voltage
 * 
 *--------------------------------------------------------------*/
#define THRESHOLD 10            // Accept an input +/- 20 steps

void cal_analog(void)
{
  uint16_t max_input;
  uint16_t reference;

  show_analog();
  max_input = max_analog();
  reference = analogRead(V_REFERENCE);

  
  if ( reference < (max_input + THRESHOLD) )
  {
    set_LED(LED_S, 1);
    set_LED(LED_X, 0);
    set_LED(LED_Y, 0);
  }
  else if ( reference > (max_input + (2*THRESHOLD)) )
  {
    set_LED(LED_S, 0);
    set_LED(LED_X, 0);
    set_LED(LED_Y, 1);
  }
  else
  {
    set_LED(LED_S, 0);
    set_LED(LED_X, 1);
    set_LED(LED_Y, 0);
  }

/*
 * All done, return
 */
  return;

}

/*----------------------------------------------------------------
 * 
 * void show_analog()
 * 
 * Read and print the analog vlaue
 * 
 *--------------------------------------------------------------*/
void show_analog(void)
{
  Serial.println();
  Serial.print("Ref: ");     Serial.print(analogRead(V_REFERENCE));
  Serial.print("  North: "); Serial.print(analogRead(NORTH_ANA));
  Serial.print("  East: ");  Serial.print(analogRead(EAST_ANA));
  Serial.print("  South: "); Serial.print(analogRead(SOUTH_ANA));
  Serial.print("  West: ");  Serial.print(analogRead(WEST_ANA));
  Serial.print("  Max: ");   Serial.print(max_analog());

  Serial.print(" RN: ");     Serial.print(read_in(RUN_NORTH));
  Serial.print(" RE: ");     Serial.print(read_in(RUN_EAST));
  Serial.print(" RS: ");     Serial.print(read_in(RUN_SOUTH));
  Serial.print(" RW: ");     Serial.print(read_in(RUN_WEST));
    
  Serial.println();

  digitalWrite(STOP_N,  0);   // Clear the flip flops
  digitalWrite(STOP_N,  1);   // Let them go again
  
  return;

}
