/*
 * Global functions
 */
#ifndef _JSON_H_
#define _JSON_H_

void read_JSON(void);                   // Scan the serial port looking for JSON input

extern unsigned int  json_dip_switch;   // DIP switch overwritten by JSON message
extern double        json_sensor_dia;   // Sensor radius overwitten by JSON message
extern unsigned int  json_paper_time;   // Time to turn on paper backer motor
extern unsigned int  json_echo;         // Value to ech
extern unsigned int  json_test;         // Self test to be performed
extern unsigned int  json_calibre_x10;  // Bullet/Pellet offset (pellet dia x 10, ex 0.177 --> 4.5 = 45)
extern          int  json_sensor_angle; // Angle applied to the target to compensate for assembly (Clockwise = -VE)
#endif _JSON_H_
