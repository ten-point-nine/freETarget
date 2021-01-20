/*----------------------------------------------------------------
 * 
 * freETarget
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *-------------------------------------------------------------*/
#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "EEPROM.h"
#include "nonvol.h"
#include "mechanical.h"
#include "diag_tools.h"

history_t history;  

double        s_of_sound;        // Speed of sound
unsigned int shot = 0;

char* names[] = { "ANON",    "BOSS",   "MINION",
                  "DOC",     "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",
                  "RUDOLF",  "DONNER", "BLITXEM", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",
                  "ODIN",    "WODEN",   "THOR",   "BALDAR",
                  0};
                  
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
  AUX_SERIAL.begin(115200); 
  MINION_SERIAL.begin(115200);
  
  Serial.print("\r\nfreETarget ");     Serial.print(SOFTWARE_VERSION);      Serial.print("\r\n");
  AUX_SERIAL.print("\r\nfreETarget "); AUX_SERIAL.print(SOFTWARE_VERSION);  AUX_SERIAL.print("\r\n");

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
 * Initialize variables
 */
  read_nonvol();

/*
 * Turn off the self test
 */ 
  switch (json_test)
  {
    case T_HELP:
    case T_PAPER:
    case T_PASS_THRU:
    case T_SET_TRIP:
    case T_XFR_LOOP:
      json_test = T_HELP;
      break;

    default:
      Serial.print("\r\nStarting Test: "); Serial.print(json_test);
      break;
  }

/*
 * Ready to go
 */
  show_echo();
  
  if ( read_DIP() & VERSION_2 )
  {
    Serial.print("\r\nVersion 2.2");
  }
  else
  {
    Serial.print("\r\nVersion 2.99");
  }
  
  if ( read_DIP() & BOSS )
  {
    Serial.print("\r\nBOSS\r\n");
  }
  else
  {
    Serial.print("\r\nminion\r\n");
  }
  
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
int ch;
unsigned int shot_number;

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
    if ( read_DIP() & CALIBRATE )
    {
      json_test = T_SET_TRIP;
    }
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
      Serial.print("\r\n\nWaiting...");
    }
    arm_counters();
    set_LED(LED_S, true);     // Show we are waiting
    set_LED(LED_X, false);    // No longer processing
    set_LED(LED_Y, false);   
    state = WAIT;             // Fall through to WAIT
    
/*
 * Wait for the shot
 */
  case WAIT:
    if ( (read_DIP() & BOSS) == 0 )       // Am I a minion?
      {
      ch = MINION_SERIAL.read();
        
      if ( (HI(ch) == '%') || (LO(ch) == '%'))  // me to go into 
        {
        self_test(T_XFR_LOOP);            // Transfer self test
        }
      }
    sensor_status = is_running();
    if ( sensor_status != 0 )             // Shot detected
    {
      now = micros();                     // Remember the starting time
      set_LED(LED_S, false);              // No longer waiting
      set_LED(LED_X, true);               // Aquiring
      if ( read_DIP() & (VERBOSE_TRACE) )
      {
        Serial.print("\r\nTriggered by:"); 
        if ( sensor_status & 0x01 ) Serial.print("N");
        else                        Serial.print("-");
        if ( sensor_status & 0x02 ) Serial.print("E");
        else                        Serial.print("-");
        if ( sensor_status & 0x04 ) Serial.print("S");
        else                        Serial.print("-");
        if ( sensor_status & 0x08 ) Serial.print("W");
        else                        Serial.print("-");
      } 
      state = AQUIRE;
     }
     break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    if ( (micros() - now) > SHOT_TIME )   // Enough time already
    { 
      stop_counters(); 
      sensor_status = is_running();       // Remember all of the running timers
      state = REDUCE;                     // 3, 4 Have enough data to performe the calculations
    }
    break;

 
/*
 *  Reduce the data to a score
 */
  case REDUCE:   
     if ( read_DIP() & (VERBOSE_TRACE) )
     {
        sensor_status = is_running();
        Serial.print("\r\nReceived by:"); 
        if ( sensor_status & 0x01 ) Serial.print("N");
        else                        Serial.print("-");
        if ( sensor_status & 0x02 ) Serial.print("E");
        else                        Serial.print("-");
        if ( sensor_status & 0x04 ) Serial.print("S");
        else                        Serial.print("-");
        if ( sensor_status & 0x08 ) Serial.print("W");
        else                        Serial.print("-");
     } 
      
    if ( read_DIP() & (VERBOSE_TRACE) )
    {
      Serial.print("\r\nReducing...");
    }
    set_LED(LED_X, false);              // No longer aquiring
    set_LED(LED_Y, true);               // Reducing the shot
    location = compute_hit(sensor_status, &history, false);
    if ( (read_DIP() & BOSS) == 0 )
    {
      state = ARM;
      break;
    }
    send_score(&history, shot_number);
    state = WASTE;
    shot_number++;                   
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
        Serial.print("\r\nAdvancing paper...");
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
      Serial.print("\r\nBad read...\r\n");
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
