/*-------------------------------------------------------
 * 
 * gpio.ino
 * 
 * General purpose GPIO driver
 * 
 * ----------------------------------------------------*/

struct GPIO {
  byte port;
  byte in_or_out;
  byte value;
};

GPIO init_table[] = {
  {D0,       INPUT_PULLUP, 0 },
  {D1,       INPUT_PULLUP, 0 },
  {D2,       INPUT_PULLUP, 0 },
  {D3,       INPUT_PULLUP, 0 },
  {D4,       INPUT_PULLUP, 0 },     
  {D5,       INPUT_PULLUP, 0 },
  {D6,       INPUT_PULLUP, 0 },

  {NORTH_HI, OUTPUT, 1},
  {NORTH_LO, OUTPUT, 1},
  {EAST_HI,  OUTPUT, 1},
  {EAST_LO,  OUTPUT, 1},
  {SOUTH_HI, OUTPUT, 1},
  {SOUTH_LO, OUTPUT, 1},
  {WEST_HI,  OUTPUT, 1},
  {WEST_LO,  OUTPUT, 1},      

  {RUN_NORTH, INPUT_PULLUP, 0},
  {RUN_EAST,  INPUT_PULLUP, 0},
  {RUN_SOUTH, INPUT_PULLUP, 0},
  {RUN_WEST,  INPUT_PULLUP, 0},     

  {QUIET,   OUTPUT, 1},
  {RCLK,    OUTPUT, 1},
  {CLR_N,   OUTPUT, 1},
  {STOP_N,  OUTPUT, 1},

  {DIP_A,   INPUT_PULLUP, 0},
  {DIP_B,   INPUT_PULLUP, 0},
  {DIP_C,   INPUT_PULLUP, 0},
  {DIP_D,   INPUT_PULLUP, 0},  

  {LED_S,   OUTPUT, 1},
  {LED_X,   OUTPUT, 1},
  {LED_Y,   OUTPUT, 1},
  
  {EOF, EOF, EOF} };


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
 * All done, return
 */  
  Serial.print("\n\rGPIO Ready");
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
    return_value |= digitalRead(port_list[i]);
    }

 /*
  * Return the result 
  */
  return return_value;
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
 * return: TRUE if the counters are running
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
    
  if ( digitalRead(RUN_WEST)== 1 )
    i += 8;  

 /*
  *  Return the running mask
  */
  return i;
  }

/*-----------------------------------------------------
 * 
 * function: arm_counterw
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
  digitalWrite(STOP_N, 0);    // Cycle the stop just to make sure
  digitalWrite(RCLK,   0);    // Set READ CLOCK to LOW
  digitalWrite(QUIET,  1);    // Arm the counter
  digitalWrite(CLR_N,  0);    // Reset the counters 
  digitalWrite(CLR_N,  1);    // Remove the counter reset 
  digitalWrite(STOP_N, 1);    // Let the counters run

  return;
  }

/*
 *  Prepare to read the counters
 */
void stop_counters(void)
  {
  digitalWrite(STOP_N,0);   // Stop the counters
  digitalWrite(QUIET, 0);   // Kill the oscillator
  digitalWrite(RCLK,  1);   // Prepare to read
 
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
 *-----------------------------------------------------*/
unsigned int read_DIP(void)
{
  unsigned int return_value;
  
  return_value =  (digitalRead(DIP_A) << 0) + (digitalRead(DIP_B) << 1) + (digitalRead(DIP_C) << 2) + (digitalRead(DIP_D) << 3);

  return (~return_value) & 0x0f;
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
