
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
#define TRIP_POINT    50.0      // Set the trip point 5 above the dc_bias
#define PWM_GAIN      0.04     // Control loop gain
#define DEAD_BAND_LO  (dc_bias + (TRIP_POINT - 10.0))   // Allow a +/- deadband
#define DEAD_BAND_HI  (dc_bias + (TRIP_POINT + 10.0))   // High Hysterysis
#define SHOT_TIME     300       // Wait 300 ms for the shot to end

#define CLOCK_RATE    (8.0)               // Clock rate in MHz
#define CLOCK_PERIOD  (1.0/CLOCK_RATE)    // Seconds per bit

/*
 *  Sensor Geometry.  Note, values can be scaled on output
 */
#define RADIUS   (81.6/2.0)  // Radius of target in mm

#define NX 0.0             // Location of North sensor
#define NY RADIUS

#define EX RADIUS          // Location of East sensor
#define EY 0.00

#define SX 0.0             // Location of South sensor
#define SY (-RADIUS)

#define WX (-RADIUS)       // Location of West sensor
#define WY 0.0

#define WIDTH 300           // Width of target in pixels
#define FWIDTH (300.0)      // floating point value

/*
 * Target Geometry
 */
#define RIFLE   (46.0 / 2.0)       // Rifle target, 46 mm
#define PISTOL  (75.0 /2.0 )       // Pistol target, 75 m

/*
 * Variables
 */
unsigned int  north, east, south, west;           // 16 bit counter registers
unsigned int  portA, portB, portC, portD, portG, portL;


double        hit_x, hit_y;     // Location of the shot
double        score;            // Score based on 10.9
double        click_x, click_y; // Adjustment Clicks
int           shot_count;       // Shot counter
double        dc_bias;          // Computed DC bias
double        pwm_control;      // Control value sent to PWM
double        pwm_reference;    // Analog input read back from pwm
unsigned long time_out;

unsigned int b_intercept[WIDTH+1], c_intercept[WIDTH+1];
double d[WIDTH+1];              // Convert index to distance
unsigned int left, right;       // Left and right side of active target
const char* which_one[4] = {"  <<NORTH>>  ", "  <<EAST>>  ", "  <<SOUTH>>  ", "  <<WEST>>  "};

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

/*
 *  Initialize the variables
 */
  shot_count = 1;
  time_out = micros();

  for (i=0; i != WIDTH+1; i++)
  {
    d[i] = RADIUS - (float)i * 2.0 * RADIUS/FWIDTH;
  }
  
  left = (FWIDTH/2.0) * (1.0 - RIFLE/RADIUS);
  right = FWIDTH - left;

/*
 * All done, begin the program
 */
 Serial.print("\n\rReady");
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
#define ARM           0         // State is ready to ARM
#define WAIT    (ARM+1)         // ARM the circuit
#define AQUIRE (WAIT+1)         // Aquire the shot
#define REDUCE (AQUIRE+1)       // Reduce the data
#define WASTE  (REDUCE+1)
void loop() 
{
  unsigned int state = ARM;
  unsigned long now;
  unsigned int shot = 0;
  double x_time, y_time;        // Location in time
  
 while (1)
 {
/*
 * Cycle through the state machine
 */
  switch (state)
  {
/*
 * Arm the circuit
 */
  default:
  case ARM:
    arm_counters();
    set_LED(LED_S, true);     // Show we are waiting
    set_LED(LED_X, false);    // No longer processing
    state = WAIT;
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    if ( is_running() )         // Shot detected
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
      state = REDUCE;
      stop_counters();
      }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:
    compute_hit(&x_time, &y_time);
    send_score(shot, x_time, y_time);
    state = ARM;
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
