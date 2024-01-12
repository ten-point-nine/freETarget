/*----------------------------------------------------------------
 *
 * mechanical.h
 *
 * Header file for physical constant
 *
 *---------------------------------------------------------------*/
#ifndef _MECHANICAL_H_
#define _MECHANICAL_H_
 
/*
 *  Sensor Geometry.  Note, values can be scaled on output
 */
#define DIAMETER      (230.0d)        // Diameter to sensors in mm
#define RADIUS_RIFLE  ( 45.0d/2.0d)   // Rifle Target
#define RADIUS_PISTOL (230.0d/2.0d)   // Pistol Target

#define PAPER_STEP            1       // Motor runs in increments of 1 ms (1/1000 second)
#define PAPER_LIMIT   (2 * 1000)      // Limit motor duration to 2 seconds

#define FACE_STRIKE_TRIP  50          // Recognize a trip after 50 interrupts
#endif
