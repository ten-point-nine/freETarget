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
void multifunction_init(void);                  // Initialize the multifunction switches
void multifunction_switch(void);                // Handle the actions of the DIP Switch signal
void multifunction_switch_tick(void);           // Monitor the switches for long and short presses
void multifunction_wait_open(void);             // Wait for both multifunction switches to be open
mfs_action_t* mfs_find(unsigned int action);    // Find the MFS entry corresponding to the index 
void mfs_show(void);                            // Show the available settings

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 */
#define HOLD_1(x)    LO10((x))          // Low digit        xxxx2
#define _HOLD_1              1
#define HOLD_2(x)    HI10((x))          // High digit       xxx2x
#define _HOLD_2              2
#define TAP_1(x)     HLO10((x))         // High Low digit   xx2xx
#define _TAP_1               3
#define TAP_2(x)     HHI10((x))         // High High digit  x2xxx
#define _TAP_2               4
#define HOLD_12(x)   HHH10((x))         // Highest digit    2xxxx
#define _HOLD_12             5
#define HOLD_C(x)     LO10((x))         // Low digit        xxxx2
#define _HOLD_C              6
#define HOLD_D(x)     HI10((x))         // High digit       xxx2x
#define _HOLD_D              7
#define SELECT_CD(x)   HLO10(x)         // High Low digit   xx2xx


/*
 *  MFS Use
 */
#define TARGET_ON     0                   // DIP A/B used to turn the target ON
#define PAPER_FEED    1                   // DIP A/B used as a paper feed
#define LED_ADJUST    2                   // DIP A/B used to set LED brightness
#define PAPER_SHOT    3                   // DIP A/B Advance paper one cycle
#define PC_TEST       4                   // DIP A/B used to trigger fake shot
#define TARGET_OFF    5                   // DIP A/B used to turn the target OFF
#define MFS_SPARE_6   6
#define MFS_SPARE_7   7
#define MFS_SPARE_8   8

#define NO_ACTION     9                   // DIP usual function
#define TARGET_TYPE  10                   // Input outputs target type with score
#define MFS2_NU_2    12
#define MFS2_NU_3    14
#define MFS2_DIP     16                   // C and D are DIPs
#define RAPID_RED    18                   // Rapid Fire Red Output
#define RAPID_GREEN  20                   // Rapid Fire Green Output
#define RAPID_LOW    22                   // Select Rapid Fire LED type
#define RAPID_HIGH   24                   // Select Rapid Fire LED type
#define STEPPER_DRIVE  26                 // The output drives a stepper motor
#define STEPPER_ENABLE 28                 // The output enables the stepper motor
#endif
