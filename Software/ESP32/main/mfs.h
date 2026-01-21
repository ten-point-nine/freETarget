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
typedef struct
{
  unsigned int index; // Index used to identify MFS action (ex POWER_TAP)
  void (*fcn)(void);  // Function to carry out the MFS action
  char *text;         // Help text associated with the index
} mfs_action_t;

/*
 * Global functions
 */
void          multifunction_init(void);          // Initialize the multifunction switches
void          multifunction_switch(void);        // Handle the actions of the DIP Switch signal
void          multifunction_switch_tick(void);   // Monitor the switches for long and short presses
void          multifunction_wait_open(void);     // Wait for both multifunction switches to be open
mfs_action_t *mfs_find(unsigned int action);     // Find the MFS entry corresponding to the index
void          mfs_show(void);                    // Show the available settings
void          mfs_RS485_control(bool direction); // Control RS485 direction

/*
 * Multifunction Switch Use when using DIP Switch for MFS
 * Do not delete, keep for original software
 */
#define HOLD_1(x)    LO10((x))  // Low digit        xxxx2
#define HOLD_2(x)    HI10((x))  // High digit       xxx2x
#define TAP_1(x)     HLO10((x)) // High Low digit   xx2xx
#define TAP_2(x)     HHI10((x)) // High High digit  x2xxx
#define HOLD_12(x)   HHH10((x)) // Highest digit    2xxxx
#define HOLD_C(x)    LO10((x))  // Low digit        xxxx2
#define HOLD_D(x)    HI10((x))  // High digit       xxx2x
#define SELECT_CD(x) HLO10(x)   // High Low digit   xx2xx

#define IS_HOLD_C(function) (json_mfs_hold_c == (function))
#define IS_HOLD_D(function) (json_mfs_hold_d == (function))

/*
 *  MFS Use
 */
#define TARGET_ON  0      // DIP A/B used to turn the target ON
#define PAPER_FEED 1      // DIP A/B used as a paper feed
#define LED_ADJUST 2      // DIP A/B used to set LED brightness
#define PAPER_SHOT 3      // DIP A/B Advance paper one cycle
#define PC_TEST    4      // DIP A/B used to trigger fake shot
#define TARGET_OFF 5      // DIP A/B used to turn the target OFF

#define NO_ACTION      9  // DIP usual function
#define TARGET_TYPE    10 // Input outputs target type with score (Uses DIP_C or DIP_D state)
#define SHOOTER_LEVEL  11 // Shooter experiance level (Uses DIP_C or DIP_D state)
#define MFS2_NU_2      14
#define MFS2_NU_3      15
#define MFS2_DIP       16 // C and D are DIPs
#define RAPID_RED      18 // Rapid Fire Red Output
#define RAPID_GREEN    20 // Rapid Fire Green Output
#define RAPID_LOW      22 // Select Rapid Fire LED type
#define RAPID_HIGH     24 // Select Rapid Fire LED type
#define STEPPER_DRIVE  26 // The output drives a stepper motor
#define STEPPER_ENABLE 28 // The output enables the stepper motor
#define RS485_SELECT   30 // Select RS488 mode
#endif
