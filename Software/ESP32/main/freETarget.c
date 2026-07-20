/*----------------------------------------------------------------
 *
 * freETarget
 *
 * Software to run the Air-Rifle / Small Bore Electronic Target
 *
 *-------------------------------------------------------------*/
#include "esp_timer.h"
#include "driver\gpio.h"
#include "esp_random.h"
#include "stdio.h"
#include "math.h"
#include "nvs.h"
#include "mpu_wrappers.h"
#include "assert.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include <esp_wifi.h>

#define FREETARGET_C
#include "freETarget.h"
#include "board_assembly.h"
#include "helpers.h"
#include "gpio.h"
#include "gpio_define.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"
#include "nonvol.h"
#include "mechanical.h"
#include "diag_tools.h"
#include "timer.h"
#include "token.h"
#include "serial_io.h"
#include "dac.h"
#include "pcnt.h"
#include "WiFi.h"
#include "diag_tools.h"
#include "mfs.h"
#include "http_client.h"
#include "http_services.h"
#include "OTA.h"
#include "calibrate.h"
#include "timed_fire.h"

/*
 *  Variables
 */
unsigned int number_of_connections = 0; // How many people are connected to me?

                                        // Keep alive timer

static enum {
  START = 0,                         // 0 et the operating mode
  WAIT,                              // 1 ARM the circuit and wait for a shot
  REDUCE                             // 2 Reduce the data and send the score
} freETarget_state;

#define NEXT_CONTINUE 0              // Continue to the next state
#define NEXT_START    1              // Go to the start state
#define NEXT_LOOP_1   2              // Loop back one step
#define NEXT_LOOP_2   3              // Loop back two steps

extern int isr_state;

volatile unsigned int run_state = 0; // Current operating state

/*
 * Function Prototypes
 */
static unsigned int set_mode(void); // Set the target running mode
static unsigned int arm(void);      // Arm the circuit for a shot
static unsigned int wait(void);     // Wait for the shot to arrive
static unsigned int reduce(void);   // Reduce the shot data
extern void         gpio_init(void);

/*----------------------------------------------------------------
 *
 * @function: freeETarget_init()
 *
 * @brief: Initialize the board and prepare to run
 *
 * @return: None
 *
 *--------------------------------------------------------------*/

void freeETarget_init(void)
{
  run_state = IN_STARTUP;
  is_trace  = DLT_FATAL | DLT_CRITICAL | DLT_INFO;

#if TRACE_APPLICATION
  is_trace |= DLT_APPLICATION;   // Enable application tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT APPLICATON enabled");))
#endif
#if TRACE_COMMUNICATION
  is_trace |= DLT_COMMUNICATION; // Enable application tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT COMMUNICATION enabled");))
#endif
#if TRACE_DIAGNOSTICS
  is_trace |= DLT_DIAG;          // Enable diagnostics tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT DIAGNOSTICS enabled");))
#endif
#if TRACE_DEBUG
  is_trace |= DLT_DEBUG;         // Enable debug tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT DEBUG enabled");))
#endif
#if TRACE_SCORE
  is_trace |= DLT_SCORE;         // Enable score tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT SCORE enabled");))
#endif
#if TRACE_HTTP
  is_trace |= DLT_HTTP;          // Enable HTTP tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT HTTP enabled");))
#endif
#if TRACE_OTA
  is_trace |= DLT_OTA;           // Enable OTA tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT OTA enabled");))
#endif
#if TRACE_HEARTBEAT
  is_trace |= DLT_HEARTBEAT;     // Enable heartbeat tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT HEARTBEAT enabled");))
#endif
#if TRACE_CALIBRATION
  is_trace |= DLT_CALIBRATION;   // Enable calibration tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT CALIBRATION enabled");))
#endif
#if TRACE_VERBOSE
  is_trace |= DLT_VERBOSE;       // Enable verbose messages
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT VERBOSE enabled");))
#endif
#if TRACE_RAPID_FIRE
  is_trace |= DLT_RAPID_FIRE;    // Enable rapid fire tracing
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "DLT RAPID FIRE enabled");))
#endif

  /*
   *  Setup the hardware
   */
  json_aux_mode = 0;        // Assume the AUX port is not used
  gpio_init();              // Setup the hardware
  serial_io_init();         // Setup the console for debug message
  read_nonvol();            // Read in the settings
  serial_aux_init();        // Update the serial port if there is a change
  set_VREF();               // Set the reference voltages
  DAC_calibrate();          // Adjust the DAC to compensate for voltage drop
  multifunction_init();     // Override the MFS if we have to
  get_target_calibration(); // Retrieve the target settings

  /*
   * Put up a self test
   */
  set_status_LED(LED_RAPID_OFF);
  set_status_LED(LED_HELLO_WORLD); // Hello World
  set_status_LED(LED_RAPID_WARN);  // Red
  vTaskDelay(ONE_SECOND);
  set_status_LED(LED_RAPID_ON);    // Green
  vTaskDelay(ONE_SECOND);
  set_status_LED(LED_OFF);

  WiFi_init();

  /*
   *  Set up the long running timers
   */
  ft_timer_new(&keep_alive, (time_count_t)json_keep_alive * ONE_SECOND, send_keep_alive, "keep alive");                 // Keep alive timer
  ft_timer_new(&power_save, (time_count_t)(json_power_save) * (time_count_t)ONE_SECOND * 60L, &bye_tick, "power save"); // Power save timer
  ft_timer_new(&time_since_last_shot, HTTP_CLOSE_TIME * 60 * ONE_SECOND, NULL, "time since last shot"); // 15 minutes since last shot
  ft_timer_new(&time_to_go, 0, NULL, "time to go");                                                     // Time remaining in session
  ft_timer_new(&shot_timer, 0, NULL, "shot timer");                                                     // Wait for the shot to arrive
  ft_timer_new(&ring_timer, 0, NULL, "ring timer");                                                     // Wait for the ringing to stop
  ft_timer_new(&event_timer, 0, NULL, "event timer"); // Timed event, ex rapid fire or tabata

  /*
   * Run the power on self test
   */
  POST_counters();            // POST counters does not return if there is an error
  if ( check_12V() == false ) // Verify the 12 volt supply
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "12V supply not present");))
  }

  /*
   * Ready to go
   */
  show_echo();
  set_LED_PWM(json_LED_PWM);
  serial_flush(ALL);              // Get rid of everything
  shot_in         = 0;            // Clear out any junk
  shot_out        = 0;
  connection_list = CONSOLE;      // The consule is always connected
  reset_run_time();               // Reset the time of day
  time_to_go = 1000 * ONE_SECOND; // Infinite amount of time to start

  if ( DIP_SW_A )                 // Switch A pressed
  {
    OTA_load();                   // Load in a new OTA
  }

  if ( DIP_SW_B )                 // Switch B pressed
  {
    OTA_rollback();               // Roll back to old software
  }

  /*
   * Start the tasks running
   */
  run_state &= ~IN_STARTUP; // Exit startup
  return;
}

/*----------------------------------------------------------------
 *
 * @function: freeEtarget_task
 *
 * @brief: Main control loop
 *
 * @return: None
 *
 *----------------------------------------------------------------
 */

unsigned int sensor_status; // Record which sensors contain valid data
unsigned int location;      // Sensor location

void freeETarget_target_loop(void *arg)
{
  DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "freeETarget_target_loop()");))

  set_status_LED(LED_READY);

  if ( (PCNT_HIGH_GPIO & board_mask)         // Are the PCNT HIGH counters provided
       && (json_pcnt_latency != 0) )         // If the second set of timers has been enabled
  {
    DLT(DLT_INFO, SEND(CONSOLE, sprintf(_xs, "Initializing PCNT high inputs");))
    gpio_init_single(PCNT_HI);               // Program the port
  }

  start_new_session(0);
  shot_number = 1;                           // Start counting shots at 1
  run_state |= IN_SHOT;                      // Default to IN_SHOT unless a timed event overrides it

  while ( 1 )
  {
    IF_IN(IN_SLEEP | IN_TEST | IN_FATAL_ERR) // If Not in operation,
    {
      run_state &= ~IN_OPERATION;            // Exit operation
      IF_IN(IN_FATAL_ERR)                    // Have we deteted a fatal error?
      {
        set_status_LED(LED_FATAL);           // but show something really wrong
      }
      vTaskDelay(ONE_SECOND);
      continue;
    }

    run_state |= IN_OPERATION;               // In operation

    /*
     * Cycle through the state machine
     */
    switch ( freETarget_state )
    {
      default:
      case START:                                                                              // Start of the loop
        DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "state: START");))
        power_save           = (time_count_t)json_power_save * (time_count_t)ONE_SECOND * 60L; //  Reset the timer
        time_since_last_shot = HTTP_CLOSE_TIME * 60l * ONE_SECOND;                             // 15 minutes since last shot
        set_mode(); // Set the mode for the next string of shot (ex Tabata or Rapid Fire)
        arm();      // Arm the circuit and check for errors
        set_status_LED(LED_READY);
        freETarget_state = WAIT;
        DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "state: WAIT");))
        break;

      case WAIT:
        freETarget_state = wait();
        break;

      case REDUCE:
        DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "state: REDUCE");))
        reduce();
        freETarget_state = START;
        break;
    }
    /*
     * End of the loop. timeout till the next time
     */
    vTaskDelay(TICK_10ms);
  }
}

/*----------------------------------------------------------------
 *
 * @function: set_mode()
 *
 * @brief: Set up the modes for the next string of shots
 *
 * @return: Exit to the ARM state
 *
 *----------------------------------------------------------------
 *
 * The shot cycle is about to start.
 *
 * This initializes the variables.
 *
 * The illumination LED is set depending on whether or not
 * the Tabata or Rapid fire feature is enabled
 *
 *--------------------------------------------------------------*/
unsigned int set_mode(void)
{
  unsigned int i;

  DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "set_mode()");))

  for ( i = 0; i != SHOT_SPACE; i++ )
  {
    record[i].face_strike = 100; // Disable face strikes
  }

  set_LED_PWM(json_LED_PWM);     // Keep the LEDs ON

  /*
   * Proceed to the WAITing state
   */
  return WAIT; // Carry on to the target
}

/*----------------------------------------------------------------
 *
 * @function: arm()
 *
 * @brief:  Arm the circuit and check for errors
 *
 * @return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 *
 *----------------------------------------------------------------
 *
 * The shot cycle is about to start.
 *
 * This initializes the variables.
 *
 *
 *--------------------------------------------------------------*/
unsigned int arm(void)
{
  DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "arm()");))

  face_strike = 0;                // Reset the face strike count
  enable_face_strike_interrupt(); // Enable the face strike interrupt
  stop_timers();
  arm_timers();                   // Arm the counters

  sensor_status = is_running();   // and immediatly read the status
  if ( sensor_status == 0 )       // After arming, the sensor status should be zero
  {
    return WAIT;                  // Fall through to WAIT
  }

  /*
   * The sensors are tripping, display the error
   */
  set_diag_LED(find_sensor(sensor_status)->diag_LED, 5);

  /*
   * Finished displaying the error so trying again
   */
  return START;
}

/*----------------------------------------------------------------
 *
 * @function: wait()
 *
 * @brief: Wait here for a shot to be fired
 *
 * @return: Exit to the WAIT state if the circuit is ready
 *         Stay in the ARM state if a hardware fault was detected
 *
 *----------------------------------------------------------------
 *
 * This loop is executed indefinitly until a shot is detected
 *
 * Once the shot has been detected (or rapid fire complete) the
 * state machine calls reduce() to process the timers and send
 * the score.
 *
 * IMPORTANT
 *
 * Since the shots are buffered in a queue, it is possible for
 * the wait loop to be behind by more than one shot.
 *
 *--------------------------------------------------------------*/
unsigned int wait(void)
{
  /*
   * See if any shots have arrived
   */

  if ( shot_in != shot_out )
  {
    return REDUCE;
  }

  /*
   * All done, keep waiting
   */
  return WAIT;
}

/*----------------------------------------------------------------
 *
 * @function: reduce()
 *
 * @brief: Loop through one or more shots and present the score
 *
 * @return: Stay in the reduce state if a follow through timer is active
 *         Jump to ARM state if more shots are ecpected
 *
 *----------------------------------------------------------------
 *
 * This function runs in foreground and loops through the
 * shot structure whenever there are shots in hardware to be
 * reduced.
 *
 * In the case of Rapid Fire, the Rapid fire loop will hold all
 * of the shots until the time runs out our all of the shots have
 * been made.
 *
 * Sample Settings
 *
 * {"PAPER_ECO":0,  "PAPER_SHOT": 0}
 * {"PAPER_ECO":10, "PAPER_SHOT": 5}
 *
 *--------------------------------------------------------------*/
#define FORCE_PAPER_MOVE 10               // Force a paper move if we get 10 misses

unsigned int reduce(void)
{
  static unsigned int paper_shot     = 0; // Count of reduced shots
  static unsigned int paper_shot_out = 0; // Count of missed shots

  run_state |= IN_REDUCTION;

  /*
   * Loop and process the shots.  Possibly more than one shot
   */
  while ( shot_out != shot_in ) // Process the shots on the queue
  {
    DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "shot_in: %d,  shot_out:%d", shot_in, shot_out);))
    DLT(DLT_DEBUG, show_sensor_status(record[shot_out].sensor_status);)

    if ( (record[shot_out].sensor_status & 0x0f) != 0x0f )
    {
      show_sensor_fault(record[shot_out].sensor_status);
    }
    else
    {
      location = compute_hit(&record[shot_out]); // Compute the score

                                                 /*
                                                  *  Delay for a follow through
                                                  */

      if ( location != MISS ) // Was it a miss or face strike?
      {
        prepare_score(&record[shot_out], shot_out, NOT_MISSED_SHOT);

        build_json_score(&record[shot_out], SCORE_USB);
        serial_to_all(_xs, (CONSOLE | AUX_PORT));

        build_json_score(&record[shot_out], SCORE_TCPIP);
        serial_to_all(_xs, TCPIP);

        if ( (json_remote_modes & REMOTE_MODE_CLIENT) != 0 )
        {
          build_json_score(&record[shot_out], SCORE_TCPIP);
          http_native_request(json_remote_url, METHOD_POST, _xs, sizeof(_xs));
        }

        /*
         *  Advance the paper
         */
        if ( IS_DC_WITNESS || IS_STEPPER_WITNESS )                 // Has the witness paper been enabled?
        {
          if ( (json_paper_eco == 0)                               // PAPER_ECO turned off
               || (record[shot_out].radius < (json_paper_eco / 2)) // Inside the black (radius)
               || (paper_shot_out > FORCE_PAPER_MOVE) )            // Too many misses
          {
            paper_shot++;                                          //
            DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "Radius: %4.2f/%d good shot: %d/%d", record[shot_out].radius, json_paper_eco / 2,
                                                 paper_shot, json_paper_shot);))
            if ( (paper_shot >= json_paper_shot)                   // Have met the number of good shots?
                 || (paper_shot_out >= FORCE_PAPER_MOVE) )         // Or we just shot too many bad ones?
            {
              paper_start();                                       // Roll the paper
              paper_shot     = 0;                                  // And start over
              paper_shot_out = 0;                                  // Reset the outside shots
            }
          }
          else
          {
            paper_shot_out++;                                      // Outside of the desired radius, keep track of the misses
            DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "Radius: %4.2f/%d bad shot: %d/%d", record[shot_out].radius, json_paper_eco / 2,
                                                 paper_shot, json_paper_shot);))
          }
        }
      }
      else                                                         // We have a miss
      {
        DLT(DLT_INFO, show_sensor_status(record[shot_out].sensor_status);)
        set_status_LED(LED_MISS);
        if ( json_send_miss != 0 )
        {
          prepare_score(&record[shot_out], shot_out, MISSED_SHOT); // Show a miss
        }
      }
      shot_out = (shot_out + 1) % SHOT_SPACE;                      // Increment to the next shot
    }
  }

  /*
   * All done, Exit to FINISH if the timer has expired
   */
  while ( ring_timer > 0 ) // Wait here to make sure the ringing has stopped
  {
    DLT(DLT_DEBUG, SEND(CONSOLE, sprintf(_xs, "ring_timer: %ld", ring_timer);))
    vTaskDelay(10);
  }

                           /*
                            * Finished reduction and going back into the shot
                            */
  run_state &= ~IN_REDUCTION;

  return START;
}

/*----------------------------------------------------------------
 *
 * @function: start_new_session()
 *
 * @brief: Reset and start a new session from the beginning
 *
 * @return: None
 *
 *----------------------------------------------------------------
 *
 * The target can be commanded to start a new session by receiving
 * the command {"SESSION": type}
 *
 * Session types are
 *  0 - Clear all existing sessions
 *  2 - Sighters
 *  4 - Score
 *  10 - Display all shots
 *  12 - Display sighters
 *  14 - Display score
 *
 *--------------------------------------------------------------*/
#define SESSION_PRINT 10                 // Display the session

void start_new_session(int session_type) //
{
  unsigned int i;

  DLT(DLT_APPLICATION, SEND(CONSOLE, sprintf(_xs, "start_new_session(%d)", session_type);))
  event_override();                      // Look for any event overrides in the event name

  switch ( session_type & (~SESSION_VALID) )
  {
    default:
    case SESSION_EMPTY:
      for ( i = 0; i != SHOT_SPACE; i++ )
      {
        record[i].session_type = SESSION_EMPTY;
      }
      shot_in  = 0;
      shot_out = 0;
      reset_run_time();
      time_to_go = 1000;
      break;

    case SESSION_SIGHT:     // Nothing to do
      time_to_go = 15 * 60; // 15 minute sighting timer
      reset_run_time();
      break;

    case SESSION_MATCH:
      time_to_go = 75 * 60;
      reset_run_time();
      break;

    case SESSION_PRINT + SESSION_EMPTY:
    case SESSION_PRINT + SESSION_SIGHT:
    case SESSION_PRINT + SESSION_MATCH:
      for ( i = 0; i != shot_out; i++ )
      {
        if ( ((record[i].session_type & SESSION_VALID) != 0)   // The session has valid data
             && (record[i].session_type % SESSION_PRINT) == (session_type % SESSION_PRINT) )
        {
          build_json_score(&record[i], SCORE_USB);             // Send out to the USB
          serial_to_all(_xs, ALL);

          if ( (json_remote_modes & REMOTE_MODE_CLIENT) != 0 ) // Send out to the server
          {
            build_json_score(&record[i], SCORE_TCPIP);
            http_native_request(json_remote_url, METHOD_POST, _xs, sizeof(_xs));
          }
        }
      }
      break;
  }

  /*
   *  All done, return
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: polled_target_test
 *
 * @brief:    Abbreviated state machine to test the target aquisition
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This arms the target and waits for a shot to be fired.  Once
 * the shot has been received, it is displayed on the console for
 * analysis.
 *
 * This function polls the sensors to make sure that the
 *
 *--------------------------------------------------------------*/
void polled_target_test(void)
{

  int i;
  int running;               // Copy of the is_running state

  freeETarget_timer_pause(); // Kill the background timer interrupt

  /*
   * Stay here watching the counters
   */
  while ( 1 )
  {
    arm_timers();
    SEND(ALL, sprintf(_xs, "\r\nArmed\r\n");)
    while ( (is_running() & RUN_MASK) != RUN_MASK )
    {
      running = is_running();
      SEND(ALL, sprintf(_xs, "\r\nis_running: %02X", running);)

      for ( i = 0; i != 8; i++ )
      {
        if ( running & (1 << i) )
        {
          SEND(ALL, sprintf(_xs, " %s ", find_sensor(1 << i)->long_name);)
        }
      }
    }
    stop_timers();
    vTaskDelay(10);
  }

  /*
   * Nothing more to do
   */
  set_VREF();
  freeETarget_timer_start(); // Turn on the timers again
  return;
}

/*----------------------------------------------------------------
 *
 * @function: interrupt_target_test
 *
 * @brief:    Abbreviated state machine to test the target aquisition
 *
 * @return:   Nothing
 *
 *----------------------------------------------------------------
 *
 * This arms the target and waits for a shot to be fired.  Once
 * the shot has been received, it is displayed on the console for
 * analysis.
 *
 * This function polls the sensors to make sure that the
 *
 *--------------------------------------------------------------*/
void interrupt_target_test(void)
{

  int i;

  SEND(ALL, sprintf(_xs, "\r\nInterrupt target shot test: shot_in: %d   shot_out: %d\r\n", shot_in, shot_out);)

  /*
   * Stay here watching the counters
   */
  while ( 1 )
  {
    while ( shot_in != shot_out ) // While we have a queue o shots
    {
      SEND(ALL, sprintf(_xs, "\r\n");)
      for ( i = 0; i != 8; i++ )
      {
        SEND(ALL, sprintf(_xs, "%s:%ld  ", find_sensor(1 << i)->long_name, record[shot_out].shot_time);)
      }
      shot_out = (shot_out + 1) % SHOT_SPACE;
    }
    vTaskDelay(TICK_10ms);
  }

  /*
   * Nothing more to do
   */
  return;
}

/*----------------------------------------------------------------
 *
 * @function: find_sensor()
 *
 * @brief:    Point to the sensor structure belonging to the run mask
 *
 * @return:   pointer to the sensor structure
 *
 *----------------------------------------------------------------
 *
 * Scan throught the sensor structure looking for the match to
 * the run mask.
 *
 *--------------------------------------------------------------*/
sensor_ID_t *find_sensor(unsigned int run_mask // Run mask to look for a match
)
{
  unsigned int i;

  /*
   *  Loop throught the sensors looking for a matching run mask
   */
  for ( i = N; i <= W; i++ )
  {
    if ( (run_mask & s[i].low_sense.run_mask) != 0 )
    {
      return &s[i].low_sense;
    }

    if ( (run_mask & s[i].high_sense.run_mask) != 0 )
    {
      return &s[i].high_sense;
    }
  }

  /*
   * Not found, return null
   */
  return LED_READY;
}
/*----------------------------------------------------------------
 *
 * @function: generate_fake_shot()
 *
 * @brief:    Geerate a fake shot by faking the registers
 *
 * @return:   None
 *
 *----------------------------------------------------------------
 *
 * The function runs a raster of shots and converts them to time
 * counts that are then used to generate a fake shot complete
 * with motor runs.
 *
 *--------------------------------------------------------------*/
#define MIN_BOUND    -(165 / 2) // Minimum bound of the target in mm
#define MAX_BOUND    (165 / 2)  // Maximum bound of the target in mm
#define SHOT_SPACING 15         // Spacing between shots in mm

void generate_fake_shot(void)
{
  int    i;                     // Loop index
  int    x, y;                  // Shot location coordinates
  real_t radius;                // Radius of shot
  real_t distance;              // Distance from shot to sensor
  shot_in  = 0;                 // Reset the shot queue
  shot_out = 0;

  /*
   *  Create fake data and reduct the result
   */
  while ( serial_available(CONSOLE) == false ) // Loop until there is input
  {
    for ( x = MIN_BOUND; x <= MAX_BOUND; x += SHOT_SPACING )
    {
      for ( y = MIN_BOUND; y <= MAX_BOUND; y += SHOT_SPACING )
      {
        radius = sqrtf(SQ(x) + SQ(y));         // Compute the radius for polar coordinates
        if ( radius <= (165.0 / 2.0) )         // inside the target radius
        {
          for ( i = N; i <= W; i++ )           // Loop through the sensors to compute the time counts
          {
            distance                  = sqrt((SQ(x - s[i].x_mm) + SQ(y - s[i].y_mm)));
            record[shot_in].shot_time = (int)(SHOT_TIME * OSCILLATOR_MHZ) - (distance / 0.35 * OSCILLATOR_MHZ); // Fake the travel time in
          }

          record[shot_in].shot          = shot_in;
          record[shot_in].face_strike   = 0;                                                                    // No face strikes
          record[shot_in].sensor_status = 0x0f;                                                                 // All sensors valid
          ring_timer                    = json_min_ring_time * ONE_SECOND / 1000;                               // Reset the ring timer
          shot_in                       = (shot_in + 1) % SHOT_SPACE;

          reduce();                                                                                             // Process the shot
          vTaskDelay(ONE_SECOND * 2);
        }
      }
    }
    vTaskDelay(ONE_SECOND * 5);
  }

  /*
   * Nothing more to do
   */
  serial_flush(ALL);
  SEND(ALL, sprintf(_xs, _DONE_);)
  return;
}
