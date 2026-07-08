/*----------------------------------------------------------------
 *
 * timed_fire.h
 *
 * Software to run the Air-Gun / Small Bore Electronic Target
 *
 *--------------------------------------------------------------*/

#ifndef _TIMED_FIRE_H
#define _TIMED_FIRE _H

extern time_count_t event_timer; // Timer used for rapid fire events

/*
 * timed fire functions
 */
void timed_event_task(void); // Run the Rapid Fire state machine

#endif
