/*
 * Global functions
 */
#ifndef _JSON_H_
#define _JSON_H_

void read_JSON(void);                   // Scan the serial port looking for JSON input

extern int    json_dip_switch;   // DIP switch overwritten by JSON message
extern double json_sensor_dia;   // Sensor radius overwitten by JSON message
extern int    json_paper_time;   // Time to turn on paper backer motor
extern int    json_echo;         // Value to ech
extern int    json_test;         // Self test to be performed
extern int    json_calibre_x10;  // Bullet/Pellet offset (pellet dia x 10, ex 0.177 --> 4.5 = 45)
extern int    json_sensor_angle; // Angle applied to the target to compensate for assembly (Clockwise = -VE)

extern int    json_north_x;      // X position of North sensor
extern int    json_north_y;      // Y position of North sensor
extern int    json_east_x;       // X position of East sensor
extern int    json_east_y;       // Y position of East sensor
extern int    json_south_x;      // X position of South sensor
extern int    json_south_y;      // Y position of South sensor
extern int    json_west_x;       // X position of West sensor
extern int    json_west_y;       // Y position of West sensor
#endif _JSON_H_
