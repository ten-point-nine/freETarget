/*----------------------------------------------------------------
 * 
 * freETarget
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *--------------------------------------------------------------
 *--
 */
#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "EEPROM.h"
#include "nonvol.h"
#include "mechanical.h"

history_t history;
double        s_of_sound;        // Speed of sound
unsigned int shot = 0;

/*----------------------------------------------------------------
 * 
 * void setup()
 * 
 * Initialize the board and prepare to run
 * 
 *----------------------------------------------------------------
 */
void show_echo(void);
void setup() 
{
  int i;
  
/*
 *  Setup the serial port
 */
  Serial.begin(115200);
  Serial.print("\n\rfreETarget "); Serial.print(SOFTWARE_VERSION); Serial.print("\n\r");
  
/*
 * Initialize variables
 */
  read_nonvol();
  
/*
 *  Set up the port pins
 */
  init_gpio();
  init_sensors();
  init_analog_io();
  for (i=0; i !=4; i++)
  {
    digitalWrite(LED_S, ~(1 << i) & 1);
    digitalWrite(LED_X, ~(1 << i) & 2);
    digitalWrite(LED_Y, ~(1 << i) & 4);
    delay(250);
  }

/*
 * All done, begin the program
 */
 show_echo();
 return;
}

/*----------------------------------------------------------------
 * 
 * void loop()
 * 
 * Main control loop
 * 
 *----------------------------------------------------------------
 */
#define SET_MODE      0         // Set the operating mode
#define ARM    (SET_MODE+1)     // State is ready to ARM
#define WAIT    (ARM+1)         // ARM the circuit
#define AQUIRE (WAIT+1)         // Aquire the shot
#define REDUCE (AQUIRE+1)       // Reduce the data
#define WASTE  (REDUCE+1)       // Wait for the shot to end
#define SEND_ERROR (WASTE+1)    // Got a trigger, but was defective

unsigned int state = SET_MODE;
unsigned long now;
double x_time, y_time;          // Location in time
unsigned int running_mode;
unsigned int sensor_status;     // Record which sensors contain valid data
unsigned int location;          // Sensor location 
unsigned int i, j;              // Iteration Counter
void loop() 
{

/*
 * Take care of any commands coming through
 */
  read_JSON();
  
/*
 * Cycle through the state machine
 */
  switch (state)
  {

/*
 *  Check for special operating modes
 */
  default:
  case SET_MODE:
    if ( json_test == 0 )       // No self test started
    {
      state = ARM;              // Carry on to the target
    }
    else
    {
      self_test(json_test);     // Run the self test
    }
    break;
/*
 * Arm the circuit
 */
  case ARM:
    if ( read_DIP() & (VERBOSE_TRACE) )
    {
      Serial.print("\n\n\rWaiting...");
    }
    arm_counters();
    set_LED(LED_S, true);     // Show we are waiting
    set_LED(LED_X, false);    // No longer processing
    set_LED(LED_Y, false);   
    sensor_status = 0;
    state = WAIT;
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    if ( is_running() != 0 )              // Shot detected
      {
      state = AQUIRE;
      now = micros();                     // Remember the starting time
      }
     break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    sensor_status |= is_running();        // Remember all of the running timers
    if ( (micros() - now) > SHOT_TIME )   // Enough time already
    { 
      switch ( hamming(sensor_status) )   // Determine how many sensors were tripped
      {
        default:
        case 0:                           // 1, 2, No solution is possilble
        case 1:
        case 2:
          state = SEND_ERROR;
          break;

        case 3:
        case 4:
          state = REDUCE;                 // 3, 4 Have enough data to performe the calculations
          break;
      }
    }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:   
    stop_counters(); 
    if ( read_DIP() & (VERBOSE_TRACE) )
    {
      Serial.print("\n\rReducing...");
    }
    set_LED(LED_S, false);
    set_LED(LED_X, false);     // No longer processing
    set_LED(LED_Y, true);      // Reducing the shot
    location = compute_hit(sensor_status, shot, &history);
    send_score(&history, shot);
    state = WASTE;
    shot++;                   
    break;

/*
 *  Wait here to make sure the RUN lines are no longer set
 */
  case WASTE:
    delay(1000);                              // Hang out for a second
    if ( (json_paper_time * PAPER_STEP) > (PAPER_LIMIT) )
    {
      json_paper_time = 0;                    // Check for an infinit loop
    }
    if ( json_paper_time != 0 )
    {
      if ( read_DIP() & (VERBOSE_TRACE) )
      {
        Serial.print("\n\rAdvancing paper...");
      } 
      digitalWrite(PAPER, PAPER_ON);          // Advance the motor drive time
      for (i=0; i != json_paper_time; i++ )
      {
        j = 7 * (1.0 - ((float)i / float(json_paper_time)));
        set_LED(LED_S, j & 1);                // Show the paper advancing
        set_LED(LED_X, j & 2);                // 
        set_LED(LED_Y, j & 4);                // 
        delay(PAPER_STEP);                    // in 100ms increments
      }
      digitalWrite(PAPER, PAPER_OFF);
    }
    state = SET_MODE;
    break;

/*    
 * Show an error occured
 */
  case SEND_ERROR:     
    if ( read_DIP() & (VERBOSE_TRACE) )
    {
      Serial.print("\n\rBad read...\n\r");
    } 
    set_LED(LED_S, true);       // Showing an error
    set_LED(LED_X, true);       // 
    set_LED(LED_Y, true);       // 
    send_timer(sensor_status);
    state = WASTE;              // Advance the paper in case there is a hole
    break;
    }

/*
 * All done, exit for now
 */
  return;
}
