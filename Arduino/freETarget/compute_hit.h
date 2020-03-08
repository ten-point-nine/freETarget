/*
 * compute_hit.h
 */
 
void init_sensors(void);                            // Initialize sensor structure
void compute_hit(double* ptr_x, double* ptr_y);     // Find the location of the shot
void send_score(  unsigned int shot, double x_time, double y_time);
