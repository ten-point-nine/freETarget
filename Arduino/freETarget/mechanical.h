/*
 * Mechanical Layout
 */
 
/*
 *  Sensor Geometry.  Note, values can be scaled on output
 */
#define RADIUS   (81.6/2.0)                 // Radius of target in mm
#define K         (RADIUS * 1.41421356237)  // Baseline of triangle
#define K_SQUARED (K*K)                     // K **2

/*
 * Target Geometry
 */
#define RIFLE   (46.0 / 2.0)       // Rifle target, 46 mm
#define PISTOL  (75.0 /2.0 )       // Pistol target, 75 m

/*
 * Oscillator Features
 */
#define CLOCK_RATE    (8.0)               // Clock rate in MHz
#define CLOCK_PERIOD  (1.0/CLOCK_RATE)    // Seconds per bit
