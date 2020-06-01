/*-------------------------------------------------------
 * 
 * analog_io.ino
 * 
 * General purpose Analog driver
 * 
 * ----------------------------------------------------*/
#include "analog_io.h"
#include "gpio.h"
#include "Wire.h"

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
  Wire.begin();
  
/*
 * All done, begin the program
 */
  return;
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
 * The analog inputs are sampled, and the LED display is used to
 * show trip voltage in 250 mv increments.
 * 
 *--------------------------------------------------------------*/
#define THRESHOLD 10            // Accept an input +/- 20 steps

void cal_analog(void)
{
  uint16_t reference;
  uint16_t steps;
  
  show_analog();
  reference = analogRead(V_REFERENCE);

  steps = 1000.0 * TO_VOLTS(reference - max_analog()) / 250.0;
  
  Serial.print(steps); Serial.print("  ");

  if ( steps > 7 )
    {
     steps = 0;
    }
  digitalWrite(LED_S, (steps & 1) == 0);
  digitalWrite(LED_X, (steps & 2) == 0);
  digitalWrite(LED_Y, (steps & 4) == 0);

  
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
  Serial.print("Ref: ");     Serial.print(TO_VOLTS(analogRead(V_REFERENCE)));
  Serial.print("  North: "); Serial.print(TO_VOLTS(analogRead(NORTH_ANA)));
  Serial.print("  East: ");  Serial.print(TO_VOLTS(analogRead(EAST_ANA)));
  Serial.print("  South: "); Serial.print(TO_VOLTS(analogRead(SOUTH_ANA)));
  Serial.print("  West: ");  Serial.print(TO_VOLTS(analogRead(WEST_ANA)));
  Serial.print("  Max: ");   Serial.print(TO_VOLTS(max_analog()));

  Serial.print(" RN: ");     Serial.print(read_in(RUN_NORTH));
  Serial.print(" RE: ");     Serial.print(read_in(RUN_EAST));
  Serial.print(" RS: ");     Serial.print(read_in(RUN_SOUTH));
  Serial.print(" RW: ");     Serial.print(read_in(RUN_WEST));

  if ( read_in(RUN_NORTH) && read_in(RUN_EAST) && read_in(RUN_SOUTH) && read_in(RUN_WEST))
    {
    Serial.print(" **** ");
    }

  Serial.println();

  digitalWrite(STOP_N,  0);   // Clear the flip flops
  digitalWrite(STOP_N,  1);   // Let them go again
  
  return;

}


/*----------------------------------------------------------------
 * 
 * double temperature_C()
 * 
 * Read the temperature sensor and return temperature in degrees C
 * 
 *----------------------------------------------------------------
 *
 * See TI Documentation for LM75
 *
 *--------------------------------------------------------------*/
 #define RTD_SCALE      (0.5)   // 1/2C / LSB

double temperature_C(void)
{
  double return_value;
  unsigned int raw;
  unsigned int i;

/*
 *  Point to the temperature register
 */
  Wire.beginTransmission(TEMP_IC);
  Wire.write(0),
  Wire.endTransmission();

/*
 * Read in the temperature register
 */
  Wire.requestFrom(TEMP_IC, 2);
  raw = Wire.read();
  raw <<= 8;
  raw += Wire.read();
  raw >>= 7;
  
  if ( raw & 0x0100 )
    {
    raw |= 0xFF00;      // Sign extend
    }

/*
 *  Return the temperature in C
 */
  return_value =  (double)(raw) * RTD_SCALE ;

  if ( (read_DIP() & (VERBOSE_TRACE | RUNNING_MODE_CALIBRATION)) || (SAMPLE_CALCULATIONS != 0))
    {
    Serial.print("\n\rTemperature (RAW): 0x"); Serial.print(raw, HEX); Serial.print("   (C):"); Serial.print(return_value);
    }
    
  return return_value;

}
