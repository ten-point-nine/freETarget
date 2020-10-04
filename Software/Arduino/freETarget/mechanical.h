/*
 * Mechanical Layout
 */
 
/*
 *  Sensor Geometry.  Note, values can be scaled on output
 */
#define DIAMETER      (230.0d)        // Diameter to sensors in mm
#define RADIUS_RIFLE  ( 45.0d/2.0d)   // Rifle Target
#define RADIUS_PISTOL (230.0d/2.0d)   // Pistol Target

#define PAPER_STEP          100       // Motor runs in increments of 100 ms (1/10 second)
#define PAPER_LIMIT   (2 * 1000)      // Limit motor duration to 2 seconds
