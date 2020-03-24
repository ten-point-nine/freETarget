
/*----------------------------------------------------------------
 * 
 * freETarget
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"

history_t history[100];

/*----------------------------------------------------------------
 * 
 * void setup()
 * 
 * Initialize the board and prepare to run
 * 
 *----------------------------------------------------------------
 */
void setup() 
{
  int i;
/*
 *  Setup the serial port
 */
  Serial.begin(115200);
  Serial.println("\n\rfreETarget V2.0X");

/*
 *  Set up the port pins
 */
  init_gpio();
  init_sensors();
  init_analog_io();

/*
 * All done, begin the program
 */
 Serial.print("\n\rfreETarget Ready");
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
#define WASTE  (REDUCE+1)

void loop() 
{
  unsigned int state = SET_MODE;
  unsigned long now;
  unsigned int shot = 0;
  double x_time, y_time;        // Location in time
  unsigned int running_mode;
  unsigned int sensor_status;   // Record which sensors contain valid data
  unsigned int location;        // Sensor location 
  
 while (1)
 {


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
    running_mode = read_DIP();
    
    if ( running_mode & RUNNING_MODE_CALIBRATION )
    {
      cal_analog();
    }
    state = ARM;
    break;
/*
 * Arm the circuit
 */
  case ARM:
    arm_counters();
    set_LED(LED_S, true);     // Show we are waiting
    set_LED(LED_X, false);    // No longer processing
    set_LED(LED_Y, false);   
    state = WAIT;
    sensor_status = 0;
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    sensor_status |= is_running();
    if ( sensor_status != 0 )    // Shot detected
      {
      set_LED(LED_S, false);    // No longer waiting
      set_LED(LED_X, true);     // Starting processing
      state = AQUIRE;
      now = micros();           // Remember the starting time
      }
     break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    if ( (micros() - now) > SHOT_TIME )
      { 
      stop_counters();
      state = REDUCE;
      }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:
    set_LED(LED_S, false);     // 
    set_LED(LED_X, false);     // No longer processing
    set_LED(LED_Y, true);      // Reducing the shot
    location = compute_hit(shot, &history[shot]);
    rotate_shot(location, &history[shot]);  // Rotate the shot back onto the target
    send_score(&history[shot]);
    state = WASTE;
    shot++;                   
    break;

/*
 *  Wait here to make sure the RUN lines are no longer set
 */
  case WASTE:
    if ( is_running() == 0 )
    {
      state = SET_MODE;
    }
    break;
    }
  }
}

/*----------------------------------------------------------------
 * 
 * void debug_trace() 
 * 
 * Put debug information on the console
 * 
 *--------------------------------------------------------------*/
void debug_trace (void)
{
#if (TRACE_PWM == true )

#endif

  return;
}
