
/*----------------------------------------------------------------
 * 
 * Electronic_Target
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
unsigned int read_port (int port);

/*
 * Compilation Flags
 */
 #define TRACE_PWM      true
 
 // Trace the PWM registers
 #define TRACE_COUNTERS true  // Trace the COUNTER values
 #define ONE_SHOT       false // Just capture one shot
 #define CLOCK_TEST     true  // Just sample the clock inputs
 #define SHOW_PLOT      false // Disable the 2D graph
 
/*
 * Hardware Allocation
 */

#define RESET_COUNT  2        // Counter Reset mapped to GPIO2
#define RESET_IDLE   LOW
#define RESET_ACTIVE HIGH

#define ARM_GPIO     3        // Board armed and running on GPIO 3
#define ARM_ARMED    HIGH
#define ARM_DISARMED LOW

#define OSCILLATOR   17       // Turn the oscillator on and off
#define OSC_ACTIVE    0     

#define OSC_IDLE      1   

#define REF_OUT      7          // Reference Output
#define REF_IN       0          // Reference Input

#define PORT_READ(PORT, BIT_MASK) (((PORT) & (BIT_MASK)) != 0? 1:0)

#define NORTH        22         // 12 O'Clock sensor
#define NORTH_CARRY  (PORT_READ(PINA, 0x01))    // Carry out from NORTH counter
#define NORTH_RUN    4          // NORTH Counter active state
#define NORTH_GO    (digitalRead(NORTH_RUN) == 0)
#define NORTH_ANA    1          // North Analog Input

#define EAST         (NORTH+8)  //  3 O'Clock sensor
#define EAST_CARRY   (PORT_READ(PINC, 0x80))    // Carry out from EAST counter
#define EAST_ANA     2          // East Analog Input

#define SOUTH        (EAST+8)   // 6 O'Clock sensor
#define SOUTH_CARRY  (PORT_READ(PIND, 0x80))    // Carry out from SOUTH counter
#define SOUTH_ANA    3          // South Analog Input

#define WEST         (SOUTH+8)  // 9 O'Clock sensor
#define WEST_CARRY   (PORT_READ(PINL, 0x08))    // Carry out from WEST counter
#define WEST_ANA     4          // West Analog Input

#define RED_LED      15         // GPIO for RED LED
#define GREEN_LED    16         // GPIP for GREEN LED
#define LED_OFF       1         // Turn LED OFF
#define LED_ON        0         // Turn LED ON

#define CARRY_IN     0x0002     // CARRY from the 74HC393
#define CARRY_OUT    0x0100     // Carry held in counter regiser, ex north

#define LED           13        // LED on Pin 13
#define ONE_SECOND    1000      // 1000 ms delay
#define TRIP_POINT    50.0      // Set the trip point 5 above the dc_bias
#define PWM_GAIN      0.04     // Control loop gain
#define DEAD_BAND_LO  (dc_bias + (TRIP_POINT - 10.0))   // Allow a +/- deadband
#define DEAD_BAND_HI  (dc_bias + (TRIP_POINT + 10.0))   // High Hysterysis

#define CLOCK_RATE    (4.0)               // Clock rate in MHz
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
  int pwm_feedback;
  int i;
/*
 *  Setup the serial port
 */
  Serial.begin(115200);
  Serial.println("\n\rElectronic Target");

/*
 *  Set up the port pins
 */
  pinMode(ARM_GPIO, OUTPUT);
  digitalWrite(ARM_GPIO, ARM_DISARMED);

  pinMode(RESET_COUNT, OUTPUT);
  digitalWrite(RESET_COUNT, RESET_ACTIVE);
  digitalWrite(RESET_COUNT, RESET_IDLE);

  pinMode(REF_OUT, OUTPUT);

  pinMode(OSCILLATOR, OUTPUT);
  digitalWrite(OSCILLATOR, OSC_IDLE);
  
  set_port(NORTH, INPUT_PULLUP);
  pinMode(NORTH_RUN, INPUT_PULLUP);
  set_port(EAST, INPUT_PULLUP);
  set_port(SOUTH, INPUT_PULLUP);
  set_port(WEST, INPUT_PULLUP);
  
  pinMode(LED,          OUTPUT);
  pinMode(GREEN_LED,    OUTPUT);
  digitalWrite(GREEN_LED, LED_OFF);
  
  pinMode(RED_LED,      OUTPUT);
  digitalWrite(RED_LED,   LED_OFF);

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
 * Determine the DC bias of the microphones
 */
 dc_bias = (analogRead(NORTH_ANA) + analogRead(EAST_ANA) + analogRead(SOUTH_ANA) + analogRead(WEST_ANA))/4.0;
 pwm_control = dc_bias / 4.0;              // Start the PWM at the dc_bias value (analog in -> 10 bits, pwm out -> 8 bits)
 analogWrite(REF_OUT, (int)pwm_control);  // Prime the PWM

/*
 * Set up the reference voltage
 */
  while( set_reference() )
  {
    continue;
  }
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
#define WASTE  (AQUIRE+1)
void loop() 
{
  unsigned int state = ARM;
  unsigned long waste_time;

  
/*
 * Determine the DC bias of the microphones
 */
 dc_bias = (analogRead(NORTH_ANA) + analogRead(EAST_ANA) + analogRead(SOUTH_ANA) + analogRead(WEST_ANA))/4.0;

 while (1)
 {

/*
 * Adjust the reference voltage
 */
  set_reference();

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
    arm_clock();
    state = AQUIRE;
    break;
/*
 * Wait for the shot
 */
  case AQUIRE:
    if ( NORTH_GO )
    {
      if ( aquire_shot() )
      {
        digitalWrite(RED_LED, LED_ON);
        compute_hit();
        send_score();
        state = WASTE;
        waste_time = micros() + 1000000;
        debug_trace();
        shot_count++;
#if (ONE_SHOT == true )
        while(1) continue;
#endif
        
      }
    }
    break;

  case WASTE:
      if ( waste_time < micros() )
    {
      dc_bias = (analogRead(NORTH_ANA) + analogRead(EAST_ANA) + analogRead(SOUTH_ANA) + analogRead(WEST_ANA))/4.0;
      digitalWrite(RED_LED, LED_OFF);
      state = ARM;
    }
  }
 }
}

/*----------------------------------------------------------------
 * 
 * void set_port(port, sense)
 * 
 * Set all bits in an 8 bit port
 * 
 *--------------------------------------------------------------*/
void set_port (
  unsigned int port,                  // Port to manage
  unsigned int sense                  // Port sense
  )
{
  int i;

  for (i=0; i != 8; i++)
    pinMode( port+i, sense);
}

/*----------------------------------------------------------------
 * 
 * byte read_port(port)
 * 
 * Read all 8 bits of a port
 * 
 *--------------------------------------------------------------*/
unsigned int read_port (
  int port                   // Port to manage
  )
{
  unsigned int input;
  unsigned int i;

  input = 0;
  for (i=0; i != 8; i++)
  {
    input = (input << 1) + digitalRead(port+i); 
  }

  return input;
  
  switch (port)
  {
    case NORTH: input = (PORT_READ(portA, 0x01) << 7) +  (PORT_READ(portA, 0x02) << 6) + (PORT_READ(portA, 0x04) << 5) + (PORT_READ(portA, 0x08) << 4) + (PORT_READ(portA, 0x10) << 3) + (PORT_READ(portA, 0x20) << 2) + (PORT_READ(portA, 0x40) << 1) + (PORT_READ(portA, 0x80) << 0); break;
    case EAST:  input = (PORT_READ(portC, 0x80) << 7) +  (PORT_READ(portC, 0x40) << 6) + (PORT_READ(portC, 0x20) << 5) + (PORT_READ(portC, 0x10) << 4) + (PORT_READ(portC, 0x08) << 3) + (PORT_READ(portC, 0x04) << 2) + (PORT_READ(portC, 0x02) << 1) + (PORT_READ(portC, 0x01) << 0); break;
    case SOUTH: input = (PORT_READ(portD, 0x80) << 7) +  (PORT_READ(portG, 0x04) << 6) + (PORT_READ(portG, 0x02) << 5) + (PORT_READ(portG, 0x01) << 4) + (PORT_READ(portL, 0x80) << 3) + (PORT_READ(portL, 0x40) << 2) + (PORT_READ(portL, 0x20) << 1) + (PORT_READ(portL, 0x10) << 0); break;
    case WEST:  input = (PORT_READ(portL, 0x08) << 7) +  (PORT_READ(portL, 0x04) << 6) + (PORT_READ(portL, 0x02) << 5) + (PORT_READ(portL, 0x01) << 4) + (PORT_READ(portB, 0x08) << 3) + (PORT_READ(portB, 0x04) << 2) + (PORT_READ(portB, 0x02) << 1) + (PORT_READ(portB, 0x01) << 0); break;

  } 

  return input;
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
  Serial.print("\n\rPWM: "); Serial.print(pwm_control);

  Serial.print("  Bias: "); Serial.print(dc_bias); Serial.print(" = (");
  Serial.print(analogRead(NORTH_ANA)); Serial.print(" + "); Serial.print(analogRead(EAST_ANA)); Serial.print(" + "); Serial.print(analogRead(SOUTH_ANA)); Serial.print(" + "); Serial.print(analogRead(WEST_ANA)); Serial.print(")/4");
  Serial.print("  LO: "); Serial.print(DEAD_BAND_LO);
  Serial.print("  In: "); Serial.print(pwm_reference);
  Serial.print("  HI: "); Serial.print(DEAD_BAND_HI);
#endif

  return;
}

/*----------------------------------------------------------------
 * 
 * bool set_reference(void)
 * 
 * Set the referenced voltage.  Return true if adjusted
 * 
 *--------------------------------------------------------------*/
bool last_return = false;
bool set_reference(void)
{
/*
 * Check the pwm_reference every 1/5 second
 */
  if ( time_out <= micros() )
  {    
    time_out = micros() + 500000;
    pwm_reference = (float)analogRead(REF_IN);
    if ( (pwm_reference >= DEAD_BAND_LO)  && (pwm_reference <= DEAD_BAND_HI) )
    {
      digitalWrite(GREEN_LED, LED_ON);
      last_return = false;
    }
/*
 * Out of tolerence, adjust
 */
    else
    {
      debug_trace();
      digitalWrite(GREEN_LED, LED_OFF);
      pwm_control = pwm_control - (float)(pwm_reference - (dc_bias + TRIP_POINT)) * PWM_GAIN;
  
      if ( pwm_control < 0.0 ) 
        pwm_control = 0.0;
      if ( pwm_control > 255.0 ) 
        pwm_control = 255.0;
  
      analogWrite(REF_OUT, (int)pwm_control);
      last_return = true;
    }
  }
/*
 * All done, return
 */
  return last_return;
}
