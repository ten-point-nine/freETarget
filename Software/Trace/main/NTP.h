/*----------------------------------------------------------------
 *
 * NTP.h
 *
 * Header file for Network Time Protocol
 *
 *---------------------------------------------------------------*/
#ifndef _NTP_H_
#define _NTP_H_

/*
 * Variables
 */

/*
 * function Prototypes
 */
bool NTP_ttg(void);    // TRUE if we need an NTP refresh
void NTP_master(void); // Originating signal to start time sync
void NTP_slave(void);  // Receiving signal to syncronize time
void NTP_offset(void); // Calculate the loop time

/*
 *  Definitions
 */
#define _NTP_MASTER_ "NTP_MASTER" // Synchronization message from target
#define _NTP_SLAVE_  "NTP_SLAVE"  // Synchronization message from trace
#endif
