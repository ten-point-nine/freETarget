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
time_count_64_t NTP_time_us(void); // Coordinated time in us
time_count_64_t NTP_time_ms(void); // Coordinated time in ms
time_count_64_t NTP_time_s(void);  // Coordinated time in seconds

/*
 * function Prototypes
 */
bool NTP_ttg(void);    // TRUE if we need an NTP refresh
void NTP_server(void); // Originating signal to start time sync
void NTP_client(void); // Receiving signal to syncronize time
void NTP_ask(void);    // Begin to calculate the loop time

/*
 *  Definitions
 */
#define _NTP_MASTER_ "NTP_ASK"    // Ask for a time sync
#define _NTP_CLIENT_ "NTP_CLIENT" // Synchronization message from trace
#define _NTP_SERVER_ "NTP_SERVER" // Work out the loop time
#endif
