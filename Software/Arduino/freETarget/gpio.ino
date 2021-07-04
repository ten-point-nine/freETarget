/*-------------------------------------------------------
 * 
 * gpio.ino
 * 
 * General purpose GPIO driver
 * 
 * ----------------------------------------------------*/
#include "json.h"
#include "analog_io.h"

struct GPIO {
  byte port;
  byte in_or_out;
  byte value;
};

const GPIO init_table[] = {
  {D0,          INPUT_PULLUP, 0 },
  {D1,          INPUT_PULLUP, 0 },
  {D2,          INPUT_PULLUP, 0 },
  {D3,          INPUT_PULLUP, 0 },
  {D4,          INPUT_PULLUP, 0 },     
  {D5,          INPUT_PULLUP, 0 },
  {D6,          INPUT_PULLUP, 0 },

  {NORTH_HI,    OUTPUT, 1},
  {NORTH_LO,    OUTPUT, 1},
  {EAST_HI,     OUTPUT, 1},
  {EAST_LO,     OUTPUT, 1},
  {SOUTH_HI,    OUTPUT, 1},
  {SOUTH_LO,    OUTPUT, 1},
  {WEST_HI,     OUTPUT, 1},
  {WEST_LO,     OUTPUT, 1},      
        
  {RUN_NORTH,   INPUT_PULLUP, 0},
  {RUN_EAST,    INPUT_PULLUP, 0},
  {RUN_SOUTH,   INPUT_PULLUP, 0},
  {RUN_WEST,    INPUT_PULLUP, 0},     

  {QUIET,       OUTPUT, 1},
  {RCLK,        OUTPUT, 1},
  {CLR_N,       OUTPUT, 1},
  {STOP_N,      OUTPUT, 1},
  {CLOCK_START, OUTPUT, 0},
  
  {DIP_A,       INPUT_PULLUP, 0},
  {DIP_B,       INPUT_PULLUP, 0},
  {DIP_C,       INPUT_PULLUP, 0},
  {DIP_D,       INPUT_PULLUP, 0},  

  {LED_RDY,     OUTPUT, 1},
  {LED_X,       OUTPUT, 1},
  {LED_Y,       OUTPUT, 1},

  {LED_PWM,     OUTPUT, 0},
  {RTS_U,       OUTPUT, 1},
  {CTS_U,       INPUT_PULLUP, 0},

  {FACE_SENSOR, INPUT_PULLUP, 0},
  
  {SPARE_1,     OUTPUT, 1},               // 18-Paper drive active low
  {SPARE_2,     INPUT_PULLUP, 0},
  
  {EOF, EOF, EOF} };
  
static   int last_MFS_state;              // Last state read from MFS

void face_ISR(void);
static void paper_on_off(bool on);        // Turn the motor on or off

/*-----------------------------------------------------
 * 
 * function: gpio_init
 * 
 * brief: Initalize the various GPIO ports
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * The GPIO programming is held in a stgrucutre and 
 * copied out to the hardware on power up.
 *-----------------------------------------------------*/

void init_gpio(void)
{
  int i;

  i = 0;
  while (init_table[i].port != 0xff )
  {
    pinMode(init_table[i].port, init_table[i].in_or_out);
    if ( init_table[i].in_or_out == OUTPUT )
    {
      digitalWrite(init_table[i].port, init_table[i].value);
    }
    i++;
  }

  disable_interrupt();
  set_LED_PWM(0);             // Turn off the illumination for now
  
/*
 * Special case of the witness paper
 */
  pinMode(PAPER, OUTPUT);
  paper_on_off(false);        // Turn it off
  multifunction_init();       // Program the multifunction switch
  
/*
 * All done, return
 */  
  return;
}

/*-----------------------------------------------------
 * 
 * function: read_port
 * 
 * brief: Read 8 bits from a port
 * 
 * return: Eight bits returned from the port
 * 
 *-----------------------------------------------------
 *
 * To make the byte I/O platform independent, this
 * function reads the bits in one-at-a-time and collates
 * them into a single byte for return
 * 
 *-----------------------------------------------------*/
int port_list[] = {D7, D6, D5, D4, D3, D2, D1, D0};

unsigned int read_port(void)
{
  int i;
  int return_value = 0;

/*
 *  Loop and read in all of the bits
 */
  for (i=0; i != 8; i++)
    {
    return_value <<= 1;
    return_value |= digitalRead(port_list[i]) & 1;
    }

 /*
  * Return the result 
  */
  return (return_value & 0x00ff);
}

/*-----------------------------------------------------
 * 
 * function: read_counter
 * 
 * brief: Read specified counter register
 * 
 * return: 16 bit counter register
 * 
 *-----------------------------------------------------
 *
 * Set the address bits and read in 16 bits
 * 
 *-----------------------------------------------------*/

int direction_register[] = {NORTH_HI, NORTH_LO, EAST_HI, EAST_LO, SOUTH_HI, SOUTH_LO, WEST_HI, WEST_LO};

unsigned int read_counter
  (
  unsigned int direction         // What direction are we reading?
  )
{
  int i;
  unsigned int return_value_LO, return_value_HI;     // 16 bit port value
  
/*
 *  Reset all of the address bits
 */
  for (i=0; i != 8; i++)
    {
    digitalWrite(direction_register[i], 1);
    }
  digitalWrite(RCLK,  1);   // Prepare to read
  
/*
 *  Set the direction line to low
 */
  digitalWrite(direction_register[direction * 2 + 0], 0);
  return_value_HI = read_port();
  digitalWrite(direction_register[direction * 2 + 0], 1);
  
  digitalWrite(direction_register[direction * 2 + 1], 0);
  return_value_LO = read_port();
  digitalWrite(direction_register[direction * 2 + 1], 1);

/*
 *  All done, return
 */
  return (return_value_HI << 8) + return_value_LO;
}

/*-----------------------------------------------------
 * 
 * function: is_running
 * 
 * brief: Determine if the clocks are running
 * 
 * return: TRUE if any of the counters are running
 * 
 *-----------------------------------------------------
 *
 * Read in the running registers, and return a 1 for every
 * register that is running.
 * 
 *-----------------------------------------------------*/

unsigned int is_running (void)
{
  unsigned int i;
  i = 0;
  
  if ( digitalRead(RUN_NORTH) == 1 )
    i += 1;
    
  if ( digitalRead(RUN_EAST) == 1 )
    i += 2;

  if ( digitalRead(RUN_SOUTH) == 1 )
    i += 4;
    
  if ( digitalRead(RUN_WEST) == 1 )
    i += 8;  

 /*
  *  Return the running mask
  */
  return i;
  }

/*-----------------------------------------------------
 * 
 * function: arm_counters
 * 
 * brief: Strobe the control lines to start a new cycle
 * 
 * return: NONE
 * 
 *-----------------------------------------------------
 *
 * The counters are armed by
 * 
 *   Stopping the current cycle
 *   Taking the counters out of read
 *   Making sure the oscillator is running
 *   Clearing the counters
 *   Enabling the counters to run again
 * 
 *-----------------------------------------------------*/
void arm_counters(void)
  {
  digitalWrite(CLOCK_START, 0);   // Make sure Clock start is OFF
  digitalWrite(STOP_N, 0);        // Cycle the stop just to make sure
  digitalWrite(RCLK,   0);        // Set READ CLOCK to LOW
  digitalWrite(QUIET,  1);        // Arm the counter
  digitalWrite(CLR_N,  0);        // Reset the counters 
  digitalWrite(CLR_N,  1);        // Remove the counter reset 
  digitalWrite(STOP_N, 1);        // Let the counters run
  
  return;
  }



/*
 *  Stop the oscillator
 */
void stop_counters(void)
  {
  digitalWrite(STOP_N,0);   // Stop the counters
  digitalWrite(QUIET, 0);   // Kill the oscillator 
  return;
  }

/*
 *  Trip the counters for a self test
 */
void trip_counters(void)
{
  digitalWrite(CLOCK_START, 0);
  digitalWrite(CLOCK_START, 1);     // Trigger the clocks from the D input of the FF
  digitalWrite(CLOCK_START, 0);

  return;
}
/*-----------------------------------------------------
 * 
 * function: enable_interrupt
 * function: disable_interrupt
 * 
 * brief: Turn on the face interrupt
 * 
 * return: NONE
 * 
 *-----------------------------------------------------
 *
 * Enable interrupts works by attaching an interrupt
 * 
 *-----------------------------------------------------*/
void enable_interrupt(void)
{
  if ( revision() >= REV_300 )
  {
    attachInterrupt(digitalPinToInterrupt(FACE_SENSOR),  face_ISR, CHANGE);
  }
  
  return;
}

void disable_interrupt(void)
{
  if ( revision() >= REV_300 )
  {
    detachInterrupt(digitalPinToInterrupt(FACE_SENSOR));
  }

  return;
}
/*-----------------------------------------------------
 * 
 * function: read_DIP
 * 
 * brief: READ the jumper block setting
 * 
 * return: TRUE for every position with a jumper installed
 * 
 *-----------------------------------------------------
 *
 * The DIP register is read and formed into a word.
 * The word is complimented to return a 1 for every
 * jumper that is installed.
 * 
 * OR in the json_dip_switch to allow remote testing
 * OR in  0xF0 to allow for compile time testing
 *-----------------------------------------------------*/
unsigned int read_DIP(void)
{
  unsigned int return_value;
  
  if ( revision() < REV_300 )          // The silkscreen was reversed in Version 3.0  oops
  {
    return_value =  (~((digitalRead(DIP_A) << 0) + (digitalRead(DIP_B) << 1) + (digitalRead(DIP_C) << 2) + (digitalRead(DIP_D) << 3))) & 0x0F;  // DIP Switch
  }
  else
  {
    return_value =  (~((digitalRead(DIP_D) << 0) + (digitalRead(DIP_C) << 1) + (digitalRead(DIP_B) << 2) + (digitalRead(DIP_A) << 3))) & 0x0F;  // DIP Switch
  }
  return_value |= json_dip_switch;  // JSON message
  return_value |= 0xF0;             // COMPILE TIME

  return return_value;
}  

/*-----------------------------------------------------
 * 
 * function: set_LED
 * 
 * brief:    Set the state of all the LEDs
 * 
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * The state of the LEDs can be turned on or off 
 * 
 * -1 Leave alone
 *  0 Turn LED off
 *  1 Turn LED on
 * 
 *-----------------------------------------------------*/
void set_LED
  (
    int state_RDY,        // State of the Rdy LED
    int state_X,          // State of the X LED
    int state_Y           // State of the Y LED
    )
{
  if ( state_RDY >= 0 )
  {
    digitalWrite(LED_RDY, state_RDY == 0 );
  }
  
  if ( state_X >= 0 )
  {
    digitalWrite(LED_X, state_X == 0);
  }

  if ( state_Y >= 0 )
  {
    digitalWrite(LED_Y, state_Y == 0);
  }
    
  return;  
  }

/* 
 *  HAL Discrete IN
 */
bool read_in(unsigned int port)
{
  return digitalRead(port);
}

/*-----------------------------------------------------
 * 
 * function: read_timers
 * 
 * brief:   Read the timer registers
 * 
 * return:  All four timer registers read and stored
 * 
 *-----------------------------------------------------
 *
 * Force read each of the timers
 * 
 *-----------------------------------------------------*/
void read_timers(void)
{
  timer_value[N] = read_counter(N);  
  timer_value[E] = read_counter(E);
  timer_value[S] = read_counter(S);
  timer_value[W] = read_counter(W);

  return;
}

/*-----------------------------------------------------
 * 
 * function: drive_paper
 * 
 * brief:    Turn on the witness paper motor for json_paper_time
 * 
 * return:  None
 * 
 *-----------------------------------------------------
 *
 * The function turns on the motor for the specified
 * time.  The motor is cycled json_paper_step times
 * to drive a stepper motor using the same circuit.
 * 
 * Use an A4988 to drive te stepper in place of a DC
 * motor
 * 
 * There is a hardare change between Version 2.2 which
 * used a transistor and 3.0 that uses a FET.
 * The driving circuit is reversed in the two boards.
 * 
 *-----------------------------------------------------*/
 
 void drive_paper(void)
 {
  unsigned int i, j, k;                           // Iteration Counters
  
  if ( is_trace )
  {
    Serial.print("\r\nAdvancing paper ");
  }

/*
 * Drive the motor on and off for the number of cycles
 * at duration
 */

  for (i=0; i != json_paper_step; i++)            // Number of steps
  {
   paper_on_off(true);                            // Turn the motor on
   
   if ( is_trace )
   {
     Serial.print("On ");
   }
   
   for (j=0; j != json_paper_time; j++ )          // Delay in 10 ms increments
   {
     k = 7 * (1.0 - ((float)i / float(json_paper_time)));
     set_LED(k & 4, k & 2, k & 1);                // Show the count going downb
     delay(PAPER_STEP);                           // in 10ms increments
   }

   paper_on_off(false);                           // Turn the motor off
   
   if ( is_trace )
   {
     Serial.print("Off ");
   }

   if ( json_paper_step == 1)                    // DC motors only have 
   {                                              // one step, so exit
     break;   
   }

   delay(PAPER_STEP);                             // Let the A4988 catch ujp
  }

 /*
  * All done, return
  */
  return;
 }

/*-----------------------------------------------------
 * 
 * function: paper_on_off
 * 
 * brief:    Turn the withness paper motor on or off
 * 
 * return:  None
 * 
 *-----------------------------------------------------
 *
 * The witness paper motor changed polarity between 2.2
 * and Version 3.0.
 * 
 * This function reads the board revision and controls 
 * the FET accordingly
 * 
 *-----------------------------------------------------*/
 
static void paper_on_off                        // Function to turn the motor on and off
  (
  bool on                                       // on == true, turn on motor drive
  )
{
  if ( on == true )
  {
    if ( revision() < REV_300 )                 // Rev 3.0 changed the motor sense
    {
      digitalWrite(PAPER, PAPER_ON);            // Turn it on
    }
    else
    {
      digitalWrite(PAPER, PAPER_ON_300);        //
    }
  }
  else
  {
    if ( revision() < REV_300 )                 // Rev 3.0 changed the motor sense
    {
      digitalWrite(PAPER, PAPER_OFF);            // Turn it off
    }
    else
    {
      digitalWrite(PAPER, PAPER_OFF_300);        //
    }
  }

/*
 * No more, return
 */
  return;
}


/*-----------------------------------------------------
 * 
 * function: face_ISR
 * 
 * brief:    Face Strike Interrupt Service Routint
 * 
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * Sensor #5 is attached to the digital input #19 and
 * is used to generate an interrrupt whenever a face
 * strike has been detected.
 * 
 * The ISR simply counts the number of cycles.  Anything
 * above 0 is an indication that sound was picked up
 * on the front face.
 * 
 *-----------------------------------------------------*/
 void face_ISR(void)
 {
  face_strike = true;      // Got a face strike

  if ( is_trace )
  {
    Serial.print("\r\nface_ISR()");
  }

  disable_interrupt();

  return;
 }

 /*
 * Common function to indicate a fault // Cycle LEDs 5x
 */
void blink_fault
  (                                        
  unsigned int fault_code                 // Fault code to blink
  )
{
  unsigned int i;

  for (i=0; i != 3; i++)
  {
    set_LED(fault_code & 4, fault_code & 2, fault_code & 1);  // Blink the LEDs to show an error
    delay(ONE_SECOND/4);
    fault_code = ~fault_code;
    set_LED(fault_code & 4, fault_code & 2, fault_code & 1);                    // Blink the LEDs to show an error
    delay(ONE_SECOND/4);
    fault_code = ~fault_code;
  }

 /*
  * Finished
  */
  return;
}

/*-----------------------------------------------------
 * 
 * function: multifunction_init
 * 
 * brief:    Initialize the direction of the MFS on SPARE_1
 * 
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * SPARE_1 is an unassigned GPIO that will change
 * function based on the software configuration.
 * 
 * This function uses a switch statement to determine
 * the settings of the GPIO
 * 
 *-----------------------------------------------------*/
 void multifunction_init(void)
 {
  switch (json_multifunction)
  {
    case PAPER_FEED:
      pinMode(SPARE_1,INPUT_PULLUP);
      last_MFS_state = digitalRead(SPARE_1);    // Remember the starting state
      break;

  }

/*
 * The GPIO has been programmed per the multifunction mode
 */
 return;
 }

 
/*-----------------------------------------------------
 * 
 * function: multifunction_switch
 * 
 * brief:    Carry out the functions of the multifunction switch
 * 
 * return:   Switch state
 * 
 *-----------------------------------------------------
 *
 * The actions of SPARE_1 will change depending on the 
 * mode that is programmed into it.
 * 
 *-----------------------------------------------------*/
unsigned int multifunction_switch
  (
    unsigned int new_state               // If output, drive to a new state
  )
 {
    unsigned int return_value;          // Value returned to caller

/*
 * Manage the GPIO based on the configuration
 */
 
  switch (json_multifunction)
  {
    case PAPER_FEED:                      // The switch acts as paper feed control
      return_value = digitalRead(SPARE_2);
      Serial.print(return_value);
      if ( (return_value ^ last_MFS_state)// Got a change?
         & ( return_value == 0 ) )        // And its a contact to ground
      {      
        drive_paper();                    // Drive the paper once
      }
      last_MFS_state = return_value;      // and remember for next time
      break;

    case GPIO_IN:                         // The switch is a general purpose input
      return_value = digitalRead(SPARE_1);
      break;

    case GPIO_OUT:                        // The switch is a general purpose outptu 
      return_value = new_state;
      digitalWrite(SPARE_1, new_state);
      break;

    default:
      break;
  }

/*
 * All done, return the GPIO state
 */
  return return_value;
}
