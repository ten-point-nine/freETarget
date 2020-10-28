/*
 * Global functions
 */
#ifndef _JSON_H_
#define _JSON_H_

void read_JSON(void);         // Scan the serial port looking for JSON input

extern int    json_dip_switch;    // DIP switch overwritten by JSON message
extern double json_sensor_dia;    // Sensor radius overwitten by JSON message
extern int    json_sensor_angle;  // Angle sensors are rotated through
extern int    json_paper_time;    // Time to turn on paper backer motor
extern int    json_echo;          // Value to ech
extern int    json_test;          // Self test to be performed
extern int    json_calibre_x10;   // Pellet Calibre
extern int    json_north_x;       // North Adjustment
extern int    json_north_y;
extern int    json_east_x;        // East Adjustment
extern int    json_east_y;
extern int    json_south_x;       // South Adjustment
extern int    json_south_y;
extern int    json_west_x;        // WestAdjustment
extern int    json_west_y;
extern int    json_trip_point;    // Trip point in mV
#endif _JSON_H_
