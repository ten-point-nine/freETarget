/*-------------------------------------------------------
 * 
 * timer_ISR.ino
 * 
 * Timer interrupt file
 * 
 * ----------------------------------------------------*/

/*-----------------------------------------------------
 * 
 * function: init_timer
 * 
 * brief: Initalize the timer channels
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * Timer 0 used to by Arduino for the millis() functions
 * Timer 1 is used by freETarget to sample the sensor inputs
 * Timer 2 is unassigned
 *-----------------------------------------------------*/
#define FREQUENCY 1000ul                // 1000 Hz

void init_timer(void)
{
  if ( DLT(INIT_TRACE) )
  {
    Serial.print(T("init_timer()"));
  }
  
/*
 * Timer 1
 */
  TCCR1A = 0;                           // set entire TCCR0A register to 0
  TCCR1B = 0;                           // same for TCCR0B
  TCNT1  = 0;                           // initialize counter value to 0

  OCR1A = 16000000 / 64 / FREQUENCY;    // 16MHz CPU Clock / 64 Prescale / 1KHz timer interrupt
  TCCR1A |= B00000010;                  // Enable CTC Mode
  TCCR1B |= B00000011;                  // Prescale 64
  TIMSK1 &= ~(B0000010);                // Make sure the interrupt is disabled
  
/*
 * All done, return
 */  
  return;
}

/*-----------------------------------------------------
 * 
 * function: enable_timer_interrupt()
 * function: disable_timer_interrupt()
 * 
 * brief:    Turn on the interrupt enable bits
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * Set the CTC interrupt bit on or off
 * 
 *-----------------------------------------------------*/

void enable_timer_interrupt(void)
{
  TIMSK1 |= (B0000010);                       // Enable timer compare interrupt
  
/*
 * All done, return
 */  
  return;

}



void disable_timer_interrupt(void)
{
  TIMSK1 &= ~(B0000010);                      // disable timer compare interrupt
  
/*
 * All done, return
 */  
  return;
}


/*-----------------------------------------------------
 * 
 * function: ISR
 * 
 * brief:    Timer 1 Interrupt
 * 
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * Timer 1 samples the inputs and when all of the 
 * sendor inputs are present, the counters are
 * read and made available to the software
 * 
 * There are three data aquisition states
 * 
 * IDLE - No inputs are present
 * WAIT - Inputs are present, but we have to wait
 *        for all of the inputs to be present or
 *        timed out
 * DONE - We have read the counters but need to
 *        wait for the ringing to stop
 *        
 * There are three motor control states
 * 
 * IDLE    - Do nothing
 * RUNNING - The motor is turned on for a duration 
 * CYCLE   - Count the number of stepper motor cycles
 * 
 *-----------------------------------------------------*/
#define PORT_STATE_IDLE 0                       // There are no sensor inputs
#define PORT_STATE_WAIT 1                       // Some sensor inputs are present, but not all
#define PORT_STATE_DONE 2                       // All of the inmputs are present

#define MOTOR_STATE_IDLE     0                  // The motor is not running
#define MOTOR_STATE_RUNNING  1                  // The motor is running
#define MOTOR_STATE_CYCLE    2                  // Cycle the stepper motor

#define MAX_WAIT_TIME   10                      // Wait up to 10 ms for the input to arrive
#define MAX_RING_TIME   50                      // Wait 50 ms for the ringing to stop

static unsigned int isr_state = PORT_STATE_IDLE;// Current aquisition state
static unsigned int isr_timer;                  // Elapsed time counter

static unsigned int motor_state   = MOTOR_STATE_IDLE; // Current motor state
static unsigned int motor_time    = 0;          // How long the motor will run for (ms)
static unsigned int motor_reload  = 0;          // Reload count
static unsigned int motor_cycles  = 0;          // Number of cycles to execute (stepper motor)

ISR(TIMER1_COMPA_vect)
{
  unsigned int pin;                             // Value read from the port

  TCNT1  = 0;                                   // Rest the counter back to 0

/*
 * Decide what to do if based on what inputs are present
 */
  pin = RUN_PORT & RUN_A_MASK;                 // Read in the RUN bits

/*
 * Read the timer hardware based on the ISR state
 */
  switch (isr_state)
  {
    case PORT_STATE_IDLE:                       // Idle, Wait for something to show up
      if ( pin != 0 )                           // Something has triggered
      { 
        isr_timer = 0;                          // Reset the timer to 0
        isr_state = PORT_STATE_WAIT;            // Got something wait for all of the sensors tro trigger
      }
      break;
          
    case PORT_STATE_WAIT:                       // Something is present, wait for all of the inputs
      if ( (pin == RUN_A_MASK)                  // We have all of the inputs
          || (isr_timer >= MAX_WAIT_TIME) )     // or ran out of time.  Read the timers and rwestart
      {
        aquire();                               // Read the counters
        clear_running();                        // Reset the RUN flip Flop
        isr_timer = 0;                          // Reset the timer
        isr_state = PORT_STATE_DONE;            // and wait for the all clear
      }
      else
      {
        isr_timer++;                            // Wait TBD milliseconds for them to arrive
      }
      break;
      
    case PORT_STATE_DONE:                       // Waiting for the ringing to stop
      if ( pin != 0 )                           // Something got latched
      {
        isr_timer = 0;
        clear_running();                        // Reset and try later
      }
      else
      {
        isr_timer++;                            // Quiet
        if ( isr_timer >= json_min_ring_time )  // Make sure there is no rigning
        {
          arm_timers();                         // and arm for the next time
          isr_state = PORT_STATE_IDLE;          // and go back to idle
        }
      }
      break;
  }

/*
 * Run the witness paper drive if enabled
 */
  switch (motor_state)
  {
    case MOTOR_STATE_IDLE:                       // Idle, Wait for something to show up
      break;
          
    case MOTOR_STATE_RUNNING:                   // The motor has been turned on
      if ( motor_time != 0 )                    // If there is time remaining
      {
        motor_time--;                           // run down the timer
      }
      else                                      // Hit zero, Time over
      {
        paper_on_off(false);                    // Turn off the motor
        if ( motor_cycles != 0 )
        {
          motor_cycles--;                         // Decriment cycles remaining
        }
        if ( motor_cycles == 0 )
        {
          motor_state = MOTOR_STATE_IDLE;       // None left, go to idle
        }
        else
        {
          motor_state = MOTOR_STATE_CYCLE;
        }
      }
      break;

    case MOTOR_STATE_CYCLE:                       // Issue one more motor pulse
      paper_on_off(true);                         // Turn it back on
      motor_time = motor_reload;                  // Reload the time
      motor_state = MOTOR_STATE_RUNNING;
      break;
  }
 
/*
 * Undo the mutex and return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * function: set_motor_time
 * 
 * brief:    Setup the duration of the witness paper motor
 *  
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * The duration of the motor time is set into memory.
 * 
 * The motor time is cumulative. Ie if the shot is registered
 * while the paper is moving then it is left moving and
 * the new shot added to it.
 * 
 *-----------------------------------------------------*/
void set_motor_time
  (
  unsigned int duration,              // Duration in milliseconds
  unsigned int cycles                 // Number of cycles
  )
{
  if ( duration == 0 )                // The duration == 0 
  {
    motor_state = MOTOR_STATE_IDLE;   // Force Idle
    return;
  }

  motor_time += (10*duration);        // Otherwiose remember for later
  motor_reload += (10*duration);      // Set the reload count
  motor_state = MOTOR_STATE_RUNNING;  // And start running
  motor_cycles += cycles;             // Number of steps to pulse

  if ( motor_cycles == 0 )
  {
    motor_cycles = 1;
  }
  
  return;
}
