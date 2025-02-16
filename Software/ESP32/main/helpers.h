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
void target_name(char *name_space); // Return target name
int  to_int(char h);                // Convert char to integer ('A' = 0x0A)
int  instr(char *s1, char *s2);     // Compare two strings

#endif
