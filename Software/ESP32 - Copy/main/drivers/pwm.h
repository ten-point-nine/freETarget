/*----------------------------------------------------------------
 *
 * pwm.j
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _PWM_H_
#define _PWM_H_

// #include "freETarget.h"

/*
 * Global functions
 */
void pwm_init(unsigned int pwm_channel, unsigned int pwm_gpio);
void pwm_set(unsigned int pwm_channel, unsigned int percent); // Set a new PWM duty cycle

#endif
