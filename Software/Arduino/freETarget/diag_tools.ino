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
 * Estimate 0.02mm / delta count   
 *   --> 400 counts -> 8mm
 *   
 *--------------------------------------------------------------*/
unsigned int sample_scores[][4] = {
/*0*/  { 0, 0, 0, 0 },                     // Unused
//        NORTH    EAST   SOUTH    WEST
/*1*/  { 0x4000, 0x4000, 0x4000, 0x4000 }, // 10.9   // Works
/*2*/  {   8250,   8250,   8000,   8000 }, // @N-E   // Works
/*3*/  {   8250,   8000,   8000,   8250 }, // @N-W   // Works
  
/*4*/  {   8000,   8000,   8250,   8250 }, // @S-W   // Works
/*5*/  {   8000,   8250,   8250,   8000 }, // @S-E

/*6*/  {   8400,   8400,   8000,   8000 }, // @N-E
/*7*/  {   8400,   8000,   8000,   8400 }, // @N-W
  
/*8*/  {   8000,   8000,   8400,   8400 }, // @S-W
/*9*/  {   8000,   8400,   8400,   8000 }, // @S-E

/*10*/ {   8000,   8000,   8900,   8900 }, // @S-W
/*11*/ {   8000,   8900,   8900,   8000 }, // @S-E

/*12*/ {   4600,   2000,   2000,   4600 }, // @S-W  MAX
/*13*/ {   2000,   4600,   4600,   2000 }  // @S-E
};

void sample_calculations (unsigned int sample)
{
  Serial.print("\n\rSample calculations:"); Serial.print(sample); 
  
  timer_value[N] = sample_scores[sample][N];
  timer_value[E] = sample_scores[sample][E];
  timer_value[S] = sample_scores[sample][S];
  timer_value[W] = sample_scores[sample][W];

  return;
}
