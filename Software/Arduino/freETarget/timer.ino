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
#define FREQUENCY 1000ul

void init_timer(void)
{
/*
 * Timer 1
 */
  TCCR1A = 0;                           // set entire TCCR0A register to 0
  TCCR1B = 0;                           // same for TCCR0B
  TCNT1  = 0;                           // initialize counter value to 0

  OCR1A = 16000000 / 64 / FREQUENCY;
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
 * There are three states
 * 
 * IDLE - No inputs are present
 * WAIT - Inputs are present, but we have to wait
 *        for all of the inputs to be present or
 *        timed out
 * DONE - We have read the counters but need to
 *        wait for the ringing to stop
 * 
 *-----------------------------------------------------*/
#define PORT_STATE_IDLE 0                       // There are no sensor inputs
#define PORT_STATE_WAIT 1                       // Some sensor inputs are present, but not all
#define PORT_STATE_DONE 2                       // All of the inmputs are present

#define MAX_WAIT_TIME   10                      // Wait up to 10 ms for the input to arrive
#define MAX_RING_TIME    5                      // Wait 5 ms for the ringing to stop

static unsigned int isr_state = PORT_STATE_IDLE;// Current aquisition state
static unsigned int isr_timer;                  // Elapsed time counter

ISR(TIMER1_COMPA_vect)
{
  unsigned int pin;                             // Value read from the port

  TCNT1  = 0;                                   // Rest the counter back to 0

/*
 * Decide what to do if based on what inputs are present
 */
  pin = RUN_PORT & RUN_A_MASK;                 // Read in the RUN bits

/*
 * Execute a state based on the current state and the inputs
 */
  switch (isr_state)
  {
    case PORT_STATE_IDLE:                       // Idle, Wait for something to show up
      if ( pin == 0 )                           // Nothing yet
      { 
        isr_timer = 0;                          // Reset the timer to 0
        break;                                  // and try again next time
      }
      isr_state = PORT_STATE_WAIT;              // Got something wait for all of the sensors tro trigger
          
    case PORT_STATE_WAIT:                       // Something is present, wait for all of the inputs
      if ( (pin == RUN_A_MASK)                  // We have some but not all of the inputs
          || (isr_timer >= MAX_WAIT_TIME) )     // All of the inputs are here, continue on
      {
        aquire();                               // Read the counters
        isr_timer = 0;                          // Reset the timer
        clear_running();                        // Reset the RUN flip Flop
        isr_state = PORT_STATE_DONE;            // and wait for the all clear
      }
      else
      {
        isr_timer++;                            // Wait TBD milliseconds for them to arrive
        break;
      }
      
    case PORT_STATE_DONE:                       // Waiting for the ringing to stop
      if ( pin != 0 )                           // Something got latched
      {
        isr_timer = 0;
        clear_running();                        // Reset and try later
      }
      else
      {
        isr_timer++;                            // Quiet
        if ( isr_timer >= MAX_RING_TIME )       // Make sure there is no rigning
        {
          arm_timers();                         // and arm for the next time
          isr_state = PORT_STATE_IDLE;          // and go back to idle
        }
      }
      break;
  }

/*
 * Undo the mutex and return
 */
  return;
}
