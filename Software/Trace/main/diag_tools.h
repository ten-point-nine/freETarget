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

#define LED_32           "01234567890123456789012345678901"
#define LED_OFF          "                                "
#define LED_ON           "********************************"
#define LED_STARTUP      "* * * * * * * * * * * * * * * * "
#define LED_READY        "********                        "
#define LED_FIFO_FILLING "********     ****               "
#define LED_REDUCTION    "********     ****     ****      "
#define LED_NO_CAL       "********  **  **  **  ** **  ** "
#define LED_ERROR        "****                            "

/*
 *  On board expected values
 */

/*
 * Tracing
 */
#define DLT_NONE          0                        // No DLT messages displayed
#define DLT_FATAL         0x0001                   // Fatal error that prevents the trace from working
#define DLT_CRITICAL      (DLT_FATAL << 1)         // Action failed and needs to be reported
#define DLT_INFO          (DLT_CRITICAL << 1)      // Information which is always displayed
#define DLT_APPLICATION   (DLT_INFO << 1)          // Application level messages displayed (trace.c compute_hit.c)
#define DLT_COMMUNICATION (DLT_APPLICATION << 1)   // Communications messages (wifi.c token.c serial_io.c)
#define DLT_DIAG          (DLT_COMMUNICATION << 1) // Hardware diagnostics messages displayed
#define DLT_DEBUG         (DLT_DIAG << 1)          // Specific debug information
#define DLT_HTTP          (DLT_DEBUG << 1)         // Log HTTP requests
#define DLT_CALIBRATION   (DLT_HTTP << 1)          // Debug the calibraition software
#define DLT_PAUSE         (DLT_CALIBRATION << 1)   // Enable pausing
#define DLT_HEARTBEAT     (0x2000)                 // Kick out the time to see if we are alive
#define DLT_VERBOSE       (0x4000)                 // Turn on verbose tracing
#define DLT_AMB           (0x8000)                 // Special Debug DLT

#if ( ((DLT_FATAL | DLT_CRITICAL | DLT_INFO | DLT_APPLICATION | DLT_COMMUNICATION | DLT_DIAG | DLT_DEBUG | DLT_SCORE | DLT_HTTP |          \
        DLT_PAUSE) &                                                                                                                       \
       (DLT_HEARTBEAT | DLT_VERBOSE | DLT_AMB)) != 0 )
#error "DLT masks overlap"
#endif

#define PAUSE(prompt) DLT(DLT_PAUSE, { SEND(CONSOLE, sprintf(_xs, "%s", prompt);) prompt_for_confirm("Pause."); })

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
#define TRACE_PAUSE         (0 == 1)
#define TRACE_VERBOSE       (0 == 1)

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
