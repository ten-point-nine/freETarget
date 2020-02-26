/*----------------------------------------------------------------
 *
 * Read_Clock
 *
 * Module to manage the clock registers on the daughuter board
 *
 *----------------------------------------------------------------
 *
 */

unsigned long  north_c, east_c, south_c, west_c;  // Old carry stat
unsigned int clock9(unsigned int location, unsigned long shift_register);


/*----------------------------------------------------------------
 *
 * arm_clock()
 *
 * Arm the clock circuit and prepare to aquire data
 *
 *----------------------------------------------------------------
 *
 * The timing clocks are reset, and the main time base is armed
 * waiting for a sound to be detected
 *
 *--------------------------------------------------------------*/
void arm_clock()
{
  Serial.println("\n\rInfo: Arming Clock");
/*
 * Turn on the oscillator
 */
  digitalWrite(OSCILLATOR, OSC_ACTIVE);   
  
/*
 * Pulse the ARM pin to arm the flip-flop
 */
  digitalWrite(ARM_GPIO, ARM_DISARMED);
  digitalWrite(ARM_GPIO, ARM_ARMED);
  
/*
 * Pulse the RESET pin to clear the counters
 */
  digitalWrite(RESET_COUNT, RESET_ACTIVE);
  digitalWrite(RESET_COUNT, RESET_IDLE);
  digitalWrite(RESET_COUNT, RESET_ACTIVE);
  digitalWrite(RESET_COUNT, RESET_IDLE);
  
/*
 * Initialize the variables for a new sample
 */
  north_c = 0;
  east_c = 0;
  south_c = 0;
  west_c = 0;
  
/*
 * The board is ready to aquire data, exit
 */
  noInterrupts();
  return;
}

/*----------------------------------------------------------------
 *
 * aquire_shot()
 *
 * Loop while the shot is being measured
 *
 *----------------------------------------------------------------
 *
 * A shot has been triggered, and is being measured in the counter
 * registers.
 *
 * This function polls the counter, and keeps track of the carry
 * bits
 *
 *--------------------------------------------------------------*/

bool aquire_shot()
{    
/*
 * Poll the counters 30 times to pick up the carry
 * 
 */
 
//0
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//4
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//8
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//12
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//16
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//20
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//24
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 

//28
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
       
  north_c = (north_c << 1) + NORTH_CARRY;    // shift the old carry
  east_c  = (east_c << 1)  + EAST_CARRY;     // oldest in B32
  south_c = (south_c << 1) + SOUTH_CARRY;    // shift the old carry
  west_c  = (west_c << 1)  + WEST_CARRY;     // shift the old carry 
  
  digitalWrite(OSCILLATOR, OSC_IDLE);               // Shutdown the counters

  interrupts();
  
 /*
  * The timer have stopped, collect the data
  */
  north = clock9(NORTH, north_c);     // Read the ports
  east  = clock9(EAST, east_c);
  south = clock9(SOUTH, south_c);
  west  = clock9(WEST, west_c);

/*
 * The GPIO is disarmed HERE after the clocks have been read.  This is done to prevent a 
 * state where the clock can be retriggeerd due to an echo in the sound chamber
 */
  digitalWrite(ARM_GPIO, ARM_DISARMED);   
  
/*
 * All done, return
 */
  return true;
}

/*----------------------------------------------------------------
 *
 * clock9(unsitned long shift_register)
 *
 * Extend the 8 bit counters to 9 bits
 *
 *----------------------------------------------------------------
 *
 * While running, the software accumulates a shift register of
 * the 8'th bit of the counter to make up a 9'th bit
 * 
 * This funcion counts the number of 1-0 transitions and 
 * fakes bits 9 and 10
 * 
 *--------------------------------------------------------------*/
unsigned int clock9(unsigned int location, unsigned long shift_register)
{
  unsigned int count;
  unsigned int  counter_register;
  unsigned int i;

  counter_register = read_port(location);
  count = 0;
  
#if ( TRACE_COUNTERS == true )
  Serial.print("\n\r"); show_location(location); Serial.print(counter_register); Serial.print("  "); Serial.print(shift_register + 0x8000000, BIN);
#endif
  shift_register &= 0x3ffffff;
  for (i=0; i != 30; i++)
  {
    if ( (shift_register & 0x3UL) == 0x2UL )
    {
      count += 0x100;
    }

    shift_register >>= 1;
  }
  
#if ( TRACE_COUNTERS == true )
  Serial.print("  "); Serial.print(count);
#endif

  count += counter_register;

#if ( TRACE_COUNTERS == true )
  Serial.print("  "); Serial.print(count);
#endif

  return count;
}
/*----------------------------------------------------------------
 *
 * void debug_conters()
 *
 * Print the counter registers
 *
 *----------------------------------------------------------------
 *
 * A shot has been triggered, and is being measured in the counter
 * registers.
 *
 * This function polls the counter, and keeps track of the carry
 * bits
 *
 *--------------------------------------------------------------*/
 void debug_counters(void)
 {
 #if ( TRACE_COUNTERS == true )
  Serial.print("\n\rNORTH: 0x"); Serial.print(north, HEX);
  Serial.print("  EAST: 0x"); Serial.print(east, HEX);
  Serial.print("  SOUTH: 0x"); Serial.print(south, HEX);
  Serial.print("  WEST: 0x"); Serial.print(west, HEX);
#endif

 }

