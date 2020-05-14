/*----------------------------------------------------------------
 *
 * diag_tools.ino
 *
 * Debug and test tools 
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"
#include "gpio.h"

const char* which_one[4] = {"  <<NORTH>>  ", "  <<EAST>>  ", "  <<SOUTH>>  ", "  <<WEST>>  "};

/*----------------------------------------------------------------
 *
 * void show_counters
 *
 * Read the counter registers and print the results
 *
 *----------------------------------------------------------------
 *
 *--------------------------------------------------------------*/

void show_counters(void)
{
  double timer_value[4];    // Array of timer values
  int    i;
  
/*
 *  Read in the counter values 
 */
 timer_value[N] = read_counter(NORTH_HI);
 timer_value[E] = read_counter(EAST_HI);
 timer_value[S] = read_counter(SOUTH_HI);
 timer_value[W] = read_counter(WEST_HI);

/*
 * Determine the location of the reference counter (longest time)
 */
  for (i=0; i != 4; i++)
  {
    Serial.print(which_one[i]);
    Serial.print(timer_value[i]);
    Serial.println();
  }
  
  /*
   * All done return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * void sample_calculations()
 *
 * Setup a known target for sample calculations
 *
 *----------------------------------------------------------------
 * 
 * See excel spread sheet sample calculations.xls
 *    
 *--------------------------------------------------------------*/
#define BASELINE 1000
void sample_calculations (void)
{
  timer_value[N] = BASELINE - 316;
  timer_value[E] = BASELINE - 447;
  timer_value[S] = BASELINE - 707;
  timer_value[W] = BASELINE - 633;

  return;
}
