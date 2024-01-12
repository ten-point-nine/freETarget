/*----------------------------------------------------------------
 *
 * mfs.h
 *
 * Multifunction Switches 
 *
 *---------------------------------------------------------------*/
#ifndef _MFS_H_
#define _MFS_H_

/*
 * Global functions
 */
void multifunction_init(void);                            // Initialize the multifunction switches
void multifunction_switch(void);                          // Handle the actions of the DIP Switch signal
void multifuction_display(void);                          // Display the MFS settings
void multifunction_wait_open(void);                       // Wait for both multifunction switches to be open


/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD1(x)    LO10((x))          // Low digit        xxxx2
#define HOLD2(x)    HI10((x))          // High digit       xxx2x
#define TAP1(x)     HLO10((x))         // High Low digit   xx2xx
#define TAP2(x)     HHI10((x))         // High High digit  x2xxx
#define HOLD12(x)   HHH10((x))         // Highest digit    2xxxx


/*
 *  MFS Use
 */
#define POWER_TAP     0                   // DIP A/B used to wake up
#define PAPER_FEED    1                   // DIP A/B used as a paper feed
#define LED_ADJUST    2                   // DIP A/B used to set LED brightness
#define PAPER_SHOT    3                   // DIP A/B Advance paper one cycle
#define PC_TEST       4                   // DIP A/B used to trigger fake shot
#define ON_OFF        5                   // DIP A/B used to turn the target ON or OFF
#define MFS_SPARE_6   6
#define MFS_SPARE_7   7
#define MFS_SPARE_8   8
#define TARGET_TYPE   9                   // Sent target type with score

#define NO_ACTION     0                   // DIP usual function
#define RAPID_RED     1                   // Rapid Fire Red Output
#define RAPID_GREEN   2                   // Rapid Fire Green Output

#endif
