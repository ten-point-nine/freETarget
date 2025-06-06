/*----------------------------------------------------------------
 *
 * token.h
 *
 * Header file for token ring functions
 *
 *---------------------------------------------------------------*/
#ifndef _TOKEN_H_
#define _TOKEN_H_

#ifdef TOKEN_C
#define EXTERN
#else
#define EXTERN extern
#endif

/*
 * Global functions
 */
void token_init(void);      // Initialize the token ring
int  token_take(void);      // Grab the token ring
int  token_give(void);      // Rease the token ring
int  token_available(void); // TRUE if the token ring can be taken
void token_poll(void);      // Poll the token ring
void token_cycle(void);     // Token ring cyclic monitor

/*
 *  State Definitions
 */
#define TOKEN_BYTE            0x80     // Token control if bit 7 set
#define TOKEN_ENUM_REQUEST    (0 << 3) // Request a token ring enumeration
#define TOKEN_ENUM            (1 << 3) // Enumeration request
#define TOKEN_TAKE_REQUEST    (2 << 3) // Ask to take the token ring
#define TOKEN_TAKE            (3 << 3) // Take the token ring
#define TOKEN_RELEASE_REQUEST (4 << 3) // Ask to release the token ring
#define TOKEN_RELEASE         (5 << 3) // Release the token ring
#define TOKEN_CONTROL         (TOKEN_ENUM_REQUEST | TOKEN_ENUM | TOKEN_TAKE_REQUEST | TOKEN_TAKE | TOKEN_RELEASE_REQUEST | TOKEN_RELEASE)

#define TOKEN_RING   0x07              // Which token ring location
#define TOKEN_NONE   0x00              // No token ring installed
#define TOKEN_MASTER 0x01              // I am the token ring master
#define TOKEN_SLAVE  0x02              // I am a token ring slave

#define TOKEN_UNDEF -1                 // The token state is undefined
#define TOKEN_OWN   1                  // The token is owmed by me

/*
 * #defines
 */
#define TOKEN_TIME_OUT 2000 // Token timeout in ms

/*
 *  GLobal Variables
 */
EXTERN int my_ring   = TOKEN_UNDEF; // Token ring address
EXTERN int whos_ring = TOKEN_UNDEF; // Who owns the ring right now?

#endif
