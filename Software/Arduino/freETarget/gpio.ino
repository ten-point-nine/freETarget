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

GPIO init_table[] = {
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

  {LED_S,       OUTPUT, 1},
  {LED_X,       OUTPUT, 1},
  {LED_Y,       OUTPUT, 1},

  {LED_PWM,     OUTPUT, 1},
  {RTS_U,       OUTPUT, 1},
  {CTS_U,       INPUT_PULLUP, 0},

  {FACE_SENSOR, INPUT_PULLUP, 0},
  
  {SPARE_1,     OUTPUT, 1},               // 18-Paper drive active low
  {SPARE_2,     INPUT_PULLUP, 0},
  
  {EOF, EOF, EOF} };

void face_ISR(void);

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

/*
 * Special case of the witness paper
 */
  pinMode(PAPER, OUTPUT);
  if ( revision() < REV_300 )
  { 

    digitalWrite(PAPER, PAPER_OFF);
  }
  else 
  {
    digitalWrite(PAPER, PAPER_OFF_300);
  }
  
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

/*
 * Turn a LED on or off
 */
void set_LED(unsigned int led, bool state)
  {
  if ( state == 0 )
    digitalWrite(led, 1);     // ON = LOW
  else
    digitalWrite(led, 0);
    
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
 * time.
 * 
 * There is a hardare change between Version 2.2 which
 * used a transistor and 3.0 that uses a FET.
 * The driving circuit is reversed in the two boards.
 * 
 *-----------------------------------------------------*/
 void drive_paper(void)
 {
   if ( is_trace )
   {
     Serial.print("\r\nAdvancing paper...");
   }
    
   if ( revision() < REV_300 )
   {
     digitalWrite(PAPER, PAPER_ON);          // Advance the motor drive time
   }
   else
   {
     digitalWrite(PAPER, PAPER_ON_300);          // Advance the motor drive time
   }
   
   for (i=0; i != json_paper_time; i++ )
   {
     j = 7 * (1.0 - ((float)i / float(json_paper_time)));
     set_LED(LED_S, j & 1);                // Show the paper advancing
     set_LED(LED_X, j & 2);                // 
     set_LED(LED_Y, j & 4);                // 
     delay(PAPER_STEP);                    // in 100ms increments
    }
    
   if ( revision() < REV_300 )
   {
     digitalWrite(PAPER, PAPER_OFF);          // Advance the motor drive time
   }
   else
   {
     digitalWrite(PAPER, PAPER_OFF_300);          // Advance the motor drive time
     digitalWrite(PAPER, PAPER_OFF_300);          // Advance the motor drive time
   }

 /*
  * All done, return
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
 
