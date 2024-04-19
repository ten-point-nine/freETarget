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
void multifunction_switch_tick(void);                     // Monitor the switches for long and short presses
void multifuction_display(void);                          // Display the MFS settings
void multifunction_wait_open(void);                       // Wait for both multifunction switches to be open
void multifunction_show(unsigned int);                    // Show the value of the settings 
unsigned int multifunction_hold12(unsigned int);          // Modify the hold 12 field
unsigned int multifunction_hold2(unsigned int);           // Modify the hold 2 field
unsigned int multifunction_hold1(unsigned int);           // Modify the hold 1 field
unsigned int multifunction_tap2(unsigned int);            // Modify the tap 2 field
unsigned int multifunction_tap1(unsigned int);            // Modify the tap 1 field
char* multifunction_str(unsigned int);                    // Return the string name of the switch action

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD1(x)    LO10((x))          // Low digit        xxxx2
#define _HOLD1              1
#define HOLD2(x)    HI10((x))          // High digit       xxx2x
#define _HOLD2              2
#define TAP1(x)     HLO10((x))         // High Low digit   xx2xx
#define _TAP1               3
#define TAP2(x)     HHI10((x))         // High High digit  x2xxx
#define _TAP2               4
#define HOLD12(x)   HHH10((x))         // Highest digit    2xxxx
#define _HOLD12             5
#define SHIFT_HOLD1      1             // Hold 1 place
#define SHIFT_HOLD2     10             // *
#define SHIFT_TAP1     100             // *
#define SHIFT_TAP2    1000             // *
#define SHIFT_HOLD12 10000             // * 

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
