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
 *  Types
 */
typedef struct {
    unsigned int index;       // Index used to identify MFS action (ex POWER_TAP)
    void        (*fcn)(void); // Function to carry out the MFS action
    char*       text;         // Help text associated with the index
    } mfs_action_t;

/*
 * Global functions
 */
void multifunction_init(void);                            // Initialize the multifunction switches
void multifunction_switch(void);                          // Handle the actions of the DIP Switch signal
void multifunction_switch_tick(void);                     // Monitor the switches for long and short presses
void multifunction_wait_open(void);                       // Wait for both multifunction switches to be open
mfs_action_t* mfs_find(unsigned int action);              // Find the MFS entry corresponding to the index 

void mfs_power_tap (void);                               // Functions to carry out mfs actions.
void mfs_paper_feed(void);
void mfs_paper_shot(void);
void mfs_paper_test(void);
void mfs_on_off(void);
void mfs_led_adjust(void);
void mfs_pc_test(void);
void mfs_on_off(void);

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD_A(x)    LO10((x))          // Low digit        xxxx2
#define _HOLD_A              1
#define HOLD_B(x)    HI10((x))          // High digit       xxx2x
#define _HOLD_B              2
#define TAP_A(x)     HLO10((x))         // High Low digit   xx2xx
#define _TAP_A               3
#define TAP_B(x)     HHI10((x))         // High High digit  x2xxx
#define _TAP_B               4
#define HOLD_AB(x)   HHH10((x))         // Highest digit    2xxxx
#define _HOLD_AB             5
#define HOLD_C(x)     LO10((x))         // Low digit        xxxx2
#define _HOLD_C              6
#define HOLD_D(x)     HI10((x))         // High digit       xxx2x
#define _HOLD_D              7
#define SELECT_CD(x)   HLO10(x)         // High Low digit   xx2xx
#define _SELECT_CD           8
#define _SELECT_LED_LOW      0          // Active low LED
#define _SELECT_LED_HIGH     1          // Active high LED
#define _SELECT_LED_ADDR     2          // Adressable LED

#define SHIFT_HOLD_A      1             // Hold 1 place
#define SHIFT_HOLD_B     10             // *
#define SHIFT_TAP_A     100             // *
#define SHIFT_TAP_B    1000             // *
#define SHIFT_HOLD_AB 10000             // * 

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

#define NO_ACTION     9                   // DIP usual function
#define TARGET_TYPE  10                   // Input outputs target type with score
#define MFS2_NU_2    11
#define MFS2_NU_3    12
#define MFS2_DIP     13                   // 0-4 are DIPs
#define RAPID_RED    14                   // Rapid Fire Red Output
#define RAPID_GREEN  15                   // Rapid Fire Green Output
#define MFS2_NU_7    16
#define MFS2_NU_8    17
#define MFS2_NU_9    18
#endif
