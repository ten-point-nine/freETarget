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
  {D0, INPUT_PULLUP, 0 },
  {D1, INPUT_PULLUP, 0 },
  {D2, INPUT_PULLUP, 0 },
  {D3, INPUT_PULLUP, 0 },
  {D4, INPUT_PULLUP, 0 },     
  {D5, INPUT_PULLUP, 0 },
  {D6, INPUT_PULLUP, 0 },

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

  {QUIET, OUTPUT, 1},
  {READ,  OUTPUT, 1},
  {CLEAR, OUTPUT, 1},
  {STOP,  OUTPUT, 1},

  {DIP_A, INPUT_PULLUP, 0},
  {DIP_B, INPUT_PULLUP, 0},
  {DIP_C, INPUT_PULLUP, 0},
  {DIP_D, INPUT_PULLUP, 0},  

  {LED_S,  OUTPUT, 1},
  {LED_X, OUTPUT, 1},
  {LED_Y,  OUTPUT, 1},
  
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

int read_port(void)
  {
  int i;
  int return_value = 0;

/*
 *  Loop and read in all of the bits
 */
  for (i=0; i != 8; i++)
    {
    return_value <<= 1;
    return_value += digitalRead(port_list[i]);
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
  int return_value;     // 16 bit port value
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
  return_value = read_port() << 8;
  digitalWrite(direction_register[direction * 2 + 0], 1);
  digitalWrite(direction_register[direction * 2 + 1], 0);
  return_value += read_port();
  digitalWrite(direction_register[direction * 2 + 1], 1);

/*
 *  All done, return
 */
  return return_value;
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
 * Read in the running registers, and if any one of them
 * is zero, return a true
 * 
 *-----------------------------------------------------*/

bool is_running (void)
  {
  if ( digitalRead(RUN_NORTH) == 0 )
    return true;
    
  if ( digitalRead(RUN_EAST) == 0 )
    return true;

  if ( digitalRead(RUN_SOUTH) == 0 )
    return true;
    
  if ( digitalRead(RUN_WEST)== 0 )
    return true;  

 /*
  *  Nothing is running, return false
  */
  return false;
  }

/*
 *  Arm the system
 */
void arm_counters(void)
  {
  digitalWrite(READ,  1);  // Take out of read mode
  digitalWrite(QUIET, 0);  // Arm the counter
  digitalWrite(CLEAR, 0);  // Reset the counters 
  digitalWrite(CLEAR, 1);  // Remove the counter reset 
  digitalWrite(STOP,  1);  // Let the counters run

  return;
  }

/*
 *  Prepare to read the counters
 */
void stop_counters(void)
  {
  digitalWrite(STOP,  0);   // Stop the counters
  digitalWrite(READ,  0);   // Prepare to read
  digitalWrite(QUIET, 1);   // Kill the oscillator
  
  return;
  }

/*
 *  Read the DIP switch
 */
unsigned int read_DIP(void)
  {
  return (digitalRead(DIP_A) << 3) + (digitalRead(DIP_B) << 2) + (digitalRead(DIP_C) << 1) + digitalRead(DIP_D);
  }  

/*
 * Turn a LED on or off
 */
void set_LED(unsigned int led, bool state)
  {
  digitalWrite(led, 1 - state);     // ON = LOW
  return;  
  }
