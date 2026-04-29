/*----------------------------------------------------------------
 *
 * diag_tools.h
 *
 * Debug and test tools
 *
 *---------------------------------------------------------------*/
#ifndef _DIAG_TOOLS_H_
#define _DIAG_TOOLS_H_
#include "gpio.h"
/*
 * @function Prototypes
 */
void self_test(unsigned int test);
void POST_version(void);             // Show the version string
bool do_dlt(unsigned int level);     // Diagnostics Log and Trace
bool factory_test(void);             // Test the hardware in production
bool do_factory_test(bool test_run); // Carry out the factory test
void digital_input_test(void);       // Test the digital inputs
void digital_output_test(void);      // Test the digital outputs

/*
 *  Definitions
 */
#define BUILD_BOARD_BRINGUP 1

/*
 * LED status messages
 *
 */
#define build_mask(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, aa, bb, cc, dd, ee, ff)                   \
  ((a << 31) | (b << 30) | (c << 29) | (d << 28) | (e << 27) | (f << 26) | (g << 25) | (h << 24) | (i << 23) | (j << 22) | (k << 21) |     \
   (l << 20) | (m << 19) | (n << 18) | (o << 17) | (p << 16) | (q << 15) | (r << 14) | (s << 13) | (t << 12) | (u << 11) | (v << 10) |     \
   (w << 9) | (x << 8) | (y << 7) | (z << 6) | (aa << 5) | (bb << 4) | (cc << 3) | (dd << 2) | (ee << 1) | ff)

//                           A  A  A  A  A  A  A  A  B  B  B  B  B  B  B  B  C  C  C  C  C  C  C  C  D  D  D  D  D  D  D  D
#define LED_OFF   build_mask(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#define LED_ON    build_mask(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define LED_READY build_mask(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

/*
 *  On board expected values
 */
#define V12_WORKING 10.0 // Expect that the 12 Volt supply is over 10 volts
#define V12_CAUTION 5.0  // Caution if the 12 Volts supply is less than 5 volts

/*
 * Tracing
 */
#define DLT_NONE          0                        // No DLT messages displayed
#define DLT_CRITICAL      0x0001                   // Action failed and needs to be reported
#define DLT_INFO          (DLT_CRITICAL << 1)      // Information which is always displayed
#define DLT_APPLICATION   (DLT_INFO << 1)          // Application level messages displayed (trace.c compute_hit.c)
#define DLT_COMMUNICATION (DLT_APPLICATION << 1)   // Communications messages (wifi.c token.c serial_io.c)
#define DLT_DIAG          (DLT_COMMUNICATION << 1) // Hardware diagnostics messages displayed
#define DLT_DEBUG         (DLT_DIAG << 1)          // Specific debug information
#define DLT_SCORE         (DLT_DEBUG << 1)         // Display extended score record
#define DLT_HTTP          (DLT_SCORE << 1)         // Log HTTP requests
#define DLT_OTA           (DLT_HTTP << 1)          // Log OTA requests
#define DLT_CALIBRATION   (DLT_OTA << 1)           // Debug the calibraition software
#define DLT_HEARTBEAT     (0x2000)                 // Kick out the time to see if we are alive
#define DLT_VERBOSE       (0x4000)                 // Turn on verbose tracing
#define DLT_AMB           (0x8000)                 // Special Debug DLT

/*
 *  Enable compile level tracing
 */
#define TRACE_APPLICATION   (0 == 1)
#define TRACE_COMMUNICATION (0 == 1)
#define TRACE_DIAGNOSTICS   (0 == 1)
#define TRACE_DEBUG         (0 == 1)
#define TRACE_SCORE         (0 == 1)
#define TRACE_HTTP          (0 == 1)
#define TRACE_OTA           (0 == 1)
#define TRACE_HEARTBEAT     (0 == 1)
#define TRACE_CALIBRATION   (0 == 1)
#define TRACE_VERBOSE       (1 == 1)

// clang-format off
#define DLT(level, z) if ( do_dlt(level) )  { z }                                                                                                                       \
  // clang-format on

typedef struct
{
  unsigned int dlt_mask; // ex DLT_CRITICAL
  char        *dlt_text; // ex "DLT_CRITICAL"
  char         dlt_id;   // ex C
} dlt_name_t;            // Names and masks for DLT levels

extern const dlt_name_t dlt_names[];

/*
 *  Variables
 */
#endif
