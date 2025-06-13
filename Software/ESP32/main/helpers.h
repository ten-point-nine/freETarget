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
void         target_name(char *name_space);                             // Return target name
int          to_int(char h);                                            // Convert char to integer ('A' = 0x0A)
int          instr(char *s1, char *s2);                                 // Compare two strings
bool         contains(char *s1, char *s2);                              // Return true if s1 contains s2
bool         prompt_for_confirm(void);                                  // Prompt for confirmation
void         send_keep_alive(void);                                     // Send a keep alive message
void         bye(unsigned int force_bye);                               // Set to true to force a shutdown
void         bye_tick(void);                                            // Tick for the bye state machine
void         echo_serial(int delay, int in_port, int out_port);         // Echo the serial port
void         build_json_score(shot_record_t *shot, const char *format); // Create the JSON score string
int          http_target_type(void);                                    // Cnovert the target type to a number
void         squish(char *source, char *destination);                   // Convert the uri into an arguement
void         test_build_json_score(voids);                              // Test build_json_score
double       sq(double x);                                              // Square (x)
unsigned int hamming_weight(unsigned int word);                         // Add up the numbr of bits in a word
void         to_binary(unsigned int x, unsigned int bits, char *s);     // Convert a number to a binary string

/*
 * Global helper variables
 */
extern const char *names[]; // List of target names

#endif
