/*----------------------------------------------------------------
 *
 * token.h
 *
 * Header file for token ring functions
 *
 *---------------------------------------------------------------*/
#ifndef _TOKEN_H_
#define _TOKEN_H_

/*
 * Global functions
 */
void token_init(void);                        // Initialize the token ring
int  token_take(void);                        // Grab the token ring
int  token_give(void);                        // Rease the token ring
int  token_available(void);                   // TRUE if the token ring can be taken
unsigned long sys_time(void);                 // Return system time
void sys_time_reset(void);                    // Set the system time back to 0

/*
 *  State Definitions
 */
#define TOKEN_BYTE    0x80                  // Token control if bit 7 set
#define TOKEN_ENUM_REQUEST    (TOKEN_BYTE | 0x00)   // Request a token ring enumeration
#define TOKEN_ENUM            (TOKEN_BYTE | 0x10)   // Enumeration request
#define TOKEN_TAKE_REQUEST    (TOKEN_BYTE | 0x20)   // Take the token ring
#define TOKEN_TAKE            (TOKEN_BYTE | 0x30)   // Take the token ring
#define TOKEN_RELEASE_REQUEST (TOKEN_BYTE | 0x40)   // Ask to release the token ring
#define TOKEN_RELEASE         (TOKEN_BYTE | 0x50)   // Release the token ring
#define TOKEN_1               (TOKEN_BYTE | 0x60)   //
#define TOKEN_2               (TOKEN_BYTE | 0x70)   //
#define TOKEN_CONTROL ( TOKEN_ENUM_REQUEST | TOKEN_ENUM | TOKEN_TAKE_REQUEST | TOKEN_TAKE | TOKEN_RELEASE_REQUEST | TOKEN_RELEASE )
 
#define TOKEN_RING    0x07                  // Which token ring location
#define TOKEN_NONE    0x00                  // No token ring installed
#define TOKEN_MASTER  0x01                  // I am the token ring master
#define TOKEN_SLAVE   0x02                  // I am a token ring slave

#define TOKEN_UNDEF   -1                    // The token state is undefined
#define TOKEN_OWN     1                     // The token is owmed by me

/*
 * #defines
 */
#define TOKEN_TIME_OUT  2000                // Token timeout in ms

#endif
