/*----------------------------------------------------------------
 *
 * helpers.h
 *
 * Helper functions
 *
 *---------------------------------------------------------------*/
#ifndef _HELPERS_H_
#define _HELPERS_H_

/*
 * Public Functions
 */
void target_name(char *name_space);   // Return target name
int  to_int(char h);                  // Convert char to integer ('A' = 0x0A)
int  instr(char *s1, char *s2);       // Compare two strings
bool prompt_for_confirm(void);        // Prompt for confirmation
void send_keep_alive(void);           // Send a keep alive message
void bye(unsigned int force_bye);     // Set to true to force a shutdown
void echo_serial(unsigned int delay); // Echo the serial port

/*
 * Global helper variables
 */
extern const char *names[]; // List of target names

#endif
