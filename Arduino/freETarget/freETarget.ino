
/*----------------------------------------------------------------
 * 
 * freETarget
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"

/*
 * Compilation Flags
 */
 #define TRACE_PWM      true
 
 // Trace the PWM registers
#define TRACE_COUNTERS true  // Trace the COUNTER values
#define ONE_SHOT       false // Just capture one shot
#define CLOCK_TEST     true  // Just sample the clock inputs
#define SHOW_PLOT      false // Disable the 2D graph
 

#define ONE_SECOND    1000      // 1000 ms delay
#define SHOT_TIME     300       // Wait 300 ms for the shot to end

#define RUNNING_MODE_CALIBRATION 1
#define VERBOSE_TRACE            2

/*
 * Target Geometry
 */
#define RIFLE   (46.0 / 2.0)       // Rifle target, 46 mm
#define PISTOL  (75.0 /2.0 )       // Pistol target, 75 m

/*
 * Variables
 */
int           shot_count;       // Shot counter
unsigned long time_out;



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
  Serial.println("\n\rfreETarget");

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
  default"
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
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    if ( is_running() != 0 )    // Shot detected
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
  Serial.print("!");
    if ( (micros() - now) > SHOT_TIME )
      {
      state = REDUCE;
      stop_counters();
      }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:
//    compute_hit(&x_time, &y_time);
    send_score(shot, x_time, y_time);
    state = SET_MODE;
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
