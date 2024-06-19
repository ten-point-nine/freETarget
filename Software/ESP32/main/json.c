/*-------------------------------------------------------
 * 
 * JSON.c
 * 
 * JSON driver
 * 
 * ----------------------------------------------------*/
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nonvol.h"
#include "ctype.h"
#include "string.h"
#include "stdio.h"
#include "stdio.h"

#include "freETarget.h"
#include "diag_tools.h"
#include "json.h"
#include "serial_io.h"
#include "analog_io.h"
#include "token.h"
#include "serial_io.h"
#include "mechanical.h"
#include "wifi.h"
#include "mfs.h"

/*
 *  Function Prototypes
 */
static void handle_json(void);    // Breakdown the JSON and execute it

/*
 *  Variables
 */
static char input_JSON[256];

int     json_calibre_x10;           // Pellet Calibre
int     json_dip_switch;            // DIP switch overwritten by JSON message
double  json_sensor_dia = DIAMETER; // Sensor daiamter overwitten by JSON message
int     json_echo;                  // Test String 
double  json_d_echo;                // Test String
int     json_north_x;               // North Adjustment
int     json_north_y;
int     json_east_x;                // East Adjustment
int     json_east_y;
int     json_south_x;               // South Adjustment
int     json_south_y;
int     json_west_x;                // WestAdjustment
int     json_west_y;
int     json_name_id;               // Name identifier
int     json_LED_PWM;               // LED control value 
int     json_power_save;            // Power down time
int     json_send_miss;             // Send a miss message
int     json_serial_number;         // Electonic serial number
int     json_step_count;            // Number of steps ouput to motor
int     json_step_time;             // Duration of each step
int     json_multifunction;         // Multifunction switch operation
int     json_multifunction2;        // Multifunction Switch 2
int     json_z_offset;              // Distance between paper and sensor plane in 0.1mm
int     json_paper_eco;             // Do not advance paper if outside of the black
int     json_target_type;           // Modify target type (0 == single bull)
int     json_tabata_enable;         // Tabata feature enabled
int     json_tabata_on;             // Tabata ON timer
int     json_tabata_rest;           // Tabata resting timer
unsigned long json_rapid_on;        // Rapid Fire ON timer
int     json_vset_PWM;              // Starting PWM value
double  json_vset;                  // Desired VREF setting
int     json_follow_through;        // Follow through delay
int     json_keep_alive;            // Keep alive period
int     json_sensor_angle;          // Angle sensors are rotated through
int     json_paper_time = 0;        // Time paper motor is applied
int     json_tabata_warn_on;        // Tabata warning time light on
int     json_tabata_warn_off;       // Tabata warning time to shot
int     json_face_strike;           // Number of cycles to accept a strike
int     json_wifi_channel;          // Wifi channel
int     json_rapid_count;           // Number of shots expected in string
int     json_rapid_enable;          // Set to TRUE if the rapid fire event is enabled
int     json_rapid_time;            // When will the rapid fire event end?
int     json_rapid_wait;            // Delay applied to rapid start
char    json_wifi_ssid[SSID_SIZE];  // Stored value of SSID
char    json_wifi_pwd[PWD_SIZE];    // Stored value of password
int     json_wifi_dhcp;             // The ESP is a DHCP server
int     json_min_ring_time;         // Time to wait for ringing to stop
int     json_token;                 // Token ring state
double  json_vref_lo;               // Low Voltage DAC setting
double  json_vref_hi;               // High Voltage DAC setting
int     json_pcnt_latency;          // pcnt interrupt latency
int     json_mfs_hold_ab;           // Hold A and B
int     json_mfs_tap_b;             // Tap B
int     json_mfs_tap_a;             // Tap A
int     json_mfs_hold_b;            // Hold B
int     json_mfs_hold_a;            // Hold A
int     json_mfs_hold_d;            // Hold D
int     json_mfs_hold_c;            // Hold C
int     json_mfs_select_cd;         // Select C and D operation
char    json_remote_url[URL_SIZE];  // URL of calling server
int     json_remote_active;         // Set to 1 to send score to a remote server
char    json_athlete[SMALL_STRING];// Shooter name
char    json_event[SMALL_STRING];   // Shooting event
char    json_target_name[SMALL_STRING]; // Target name

static void show_test(int v);       // Execute the self test once
static void show_names(int v);
static void set_trace(int v);       // Set the trace on and off
static void diag_delay(int x) ;     // Insert a delay

  
const json_message_t JSON[] = {
//    token                 value stored in RAM     double stored in RAM        convert    service fcn()     NONVOL location      Initial Value
  {EC, "\"ANGLE\":",          &json_sensor_angle,                0,                IS_INT32,  0,                NONVOL_SENSOR_ANGLE,    45 },    // Locate the sensor angles
  {EI, "\"BYE\":",            0,                                 0,                IS_VOID,   &bye,             0,                       0 },    // Shut down the target
//  {1, "\"CALIBREx10\":",     &json_calibre_x10,                 0,                IS_INT32,  0,                NONVOL_CALIBRE_X10,     45 },    // Enter the projectile calibre (mm x 10)
  {EI, "\"DELAY\":",          0,                                 0,                IS_INT32,  &diag_delay,                      0,       0 },    // Delay TBD seconds
  {EI, "\"ECHO\":",           0,                                 0,                IS_VOID,   &show_echo,       0,                       0 },    // Echo test
 //  {1, "\"FACE_STRIKE\":",    &json_face_strike,                 0,                IS_INT32,  0,                NONVOL_FACE_STRIKE,      0 },    // Face Strike Count 
  {EO, "\"FOLLOW_THROUGH\":", &json_follow_through,              0,                IS_INT32,  0,                NONVOL_FOLLOW_THROUGH,   0 },    // Three second follow through
  {EI, "\"INIT\":",           0,                                 0,                IS_INT32,  &nonvol_init,     NONVOL_INIT,             0 },    // Initialize the NONVOL memory
  {ES, "\"KEEP_ALIVE\":",     &json_keep_alive,                  0,                IS_INT32,  0,                NONVOL_KEEP_ALIVE,     120 },    // TCPIP Keep alive period (in seconds)
  {ES, "\"LED_BRIGHT\":",     &json_LED_PWM,                     0,                IS_INT32,  &set_LED_PWM_now, NONVOL_LED_PWM,         50 },    // Set the LED brightness
  {EO, "\"MFS_HOLD_AB\":",    &json_mfs_hold_ab,                 0,                IS_MFS,    0,                NONVOL_MFS_HOLD_AB,  LED_ADJUST },
  {EO, "\"MFS_TAP_B\":",      &json_mfs_tap_b,                   0,                IS_MFS,    0,                NONVOL_MFS_TAP_B,    POWER_TAP  },
  {EO, "\"MFS_TAP_A\":",      &json_mfs_tap_a,                   0,                IS_MFS,    0,                NONVOL_MFS_TAP_A,    PAPER_SHOT },
  {EO, "\"MFS_HOLD_B\":",     &json_mfs_hold_b,                  0,                IS_MFS,    0,                NONVOL_MFS_HOLD_B,   ON_OFF     },
  {EO, "\"MFS_HOLD_A\":",     &json_mfs_hold_a,                  0,                IS_MFS,    0,                NONVOL_MFS_HOLD_A,   PAPER_FEED },
  {EO, "\"MFS_HOLD_D\":",     &json_mfs_hold_d,                  0,                IS_MFS,    0,                NONVOL_MFS_HOLD_D,   NO_ACTION  },
  {EO, "\"MFS_HOLD_C\":",     &json_mfs_hold_c,                  0,                IS_MFS,    0,                NONVOL_MFS_HOLD_C,   NO_ACTION  },
  {EO, "\"MFS_SELECT_CD\":",  &json_mfs_select_cd,               0,                IS_MFS,    0,                NONVOL_MFS_SELECT_CD,NO_ACTION  },
  
  {EO, "\"MIN_RING_TIME\":",  &json_min_ring_time,               0,                IS_INT32,  0,                NONVOL_MIN_RING_TIME,  500 },    // Minimum time for ringing to stop (ms)
  {EN, "\"NAME_ID\":",        &json_name_id,                     0,                IS_INT32,  &show_names,      NONVOL_NAME_ID,          0 },    // Give the board a name
  {EO, "\"PAPER_ECO\":",      &json_paper_eco,                   0,                IS_INT32,  0,                NONVOL_PAPER_ECO,        0 },    // Ony advance the paper is in the black
  {ES, "\"PAPER_TIME\":",     &json_paper_time,                  0,                IS_INT32,  0,                NONVOL_PAPER_TIME,     500 },    // Set the paper advance time
  {EC, "\"PCNT_LATENCY\":",   &json_pcnt_latency,                0,                IS_INT32,  0,                NONVOL_PCNT_LATENCY,    33 },    // Interrupt latency for PCNT adjustment
  {EO, "\"POWER_SAVE\":",     &json_power_save,                  0,                IS_INT32,  0,                NONVOL_POWER_SAVE,       0 },    // Set the power saver time
  {EN, "\"REMOTE_ACTIVE\":",  &json_remote_active,               0,                IS_INT32,  0,                NONVOL_REMOTE_ACTIVE,    0 },    // Send score to a remote server
  {EN, "\"REMOTE_URL\":",     (int*)&json_remote_url,            0,                IS_TEXT+URL_SIZE, 0,         NONVOL_REMOTE_URL,       0 },    // Reserve space for remote URL
  {EO, "\"RAPID_COUNT\":",    &json_rapid_count,                 0,                IS_INT32,  0,                0,                       0 },    // Number of shots expected in series
  {EO, "\"RAPID_ENABLE\":",   &json_rapid_enable,                0,                IS_INT32,  0,                0,                       0 },    // Enable the rapid fire fieature
  {EO, "\"RAPID_TIME\":",     &json_rapid_time,                  0,                IS_INT32,  0,                0,                       0 },    // Set the duration of the rapid fire event and start
  {EO, "\"RAPID_WAIT\":",     &json_rapid_wait,                  0,                IS_INT32,  0,                0,                       0 },    // Delay applied between enable and ready
  {EO, "\"SEND_MISS\":",      &json_send_miss,                   0,                IS_INT32,  0,                NONVOL_SEND_MISS,        0 },    // Enable / Disable sending miss messages
  {EC, "\"SENSOR\":",         0,                                 &json_sensor_dia, IS_FLOAT,  0,                NONVOL_SENSOR_DIA,  232000 },    // Generate the sensor postion array
  {ES, "\"SN\":",             &json_serial_number,               0,                IS_FIXED,  0,                NONVOL_SERIAL_NO,   0xffff },    // Board serial number
  {EO, "\"STEP_COUNT\":",     &json_step_count,                  0,                IS_INT32,  0,                NONVOL_STEP_COUNT,       0 },    // Set the duration of the stepper motor ON time
  {EO, "\"STEP_TIME\":",      &json_step_time,                   0,                IS_INT32,  0,                NONVOL_STEP_TIME,        0 },    // Set the number of times stepper motor is stepped
  {EO, "\"TABATA_ENABLE\":",  &json_tabata_enable,               0,                IS_INT32,  &tabata_enable,   0,                       0 },    // Enable the tabata feature
  {EO, "\"TABATA_ON\":",      &json_tabata_on,                   0,                IS_INT32,  0,                0,                       0 },    // Time that the LEDs are ON for a Tabata timer (1/10 seconds)
  {EO, "\"TABATA_REST\":",    &json_tabata_rest,                 0,                IS_INT32,  0,                0,                       0 },    // Time that the LEDs are OFF for a Tabata timer
  {EO, "\"TABATA_WARN_OFF\":",&json_tabata_warn_off,             0,                IS_INT32,  0,                0,                       0 },    // Time that the LEDs are ON during a warning cycle
  {EO, "\"TABATA_WARN_ON\":", &json_tabata_warn_on,              0,                IS_INT32,  0,                0,                     200 },    // Time that the LEDs are OFF during a warning cycle
  {EO, "\"TARGET_TYPE\":",    &json_target_type,                 0,                IS_INT32,  0,                NONVOL_TARGET_TYPE,      0 },    // Marify shot location (0 == Single Bull)
  {EO, "\"TEST\":",           0,                                 0,                IS_INT32,  &show_test,       0,                       0 },    // Execute a self test
  {EN, "\"TOKEN\":",          &json_token,                       0,                IS_INT32,  0,                NONVOL_TOKEN,            0 },    // Token ring state
  {EX, "\"TRACE\":",          0,                                 0,                IS_INT32,  &set_trace,       0,                       0 },    // Enter / exit diagnostic trace
  {ES, "\"VERSION\":",        0,                                 0,                IS_INT32,  &POST_version,    0,                       0 },    // Return the version string
  {EO, "\"VREF_LO\":",        0,                                 &json_vref_lo,    IS_FLOAT,  &set_VREF,        NONVOL_VREF_LO,       1250 },    // Low trip point value (Volts)
  {EO, "\"VREF_HI\":",        0,                                 &json_vref_hi,    IS_FLOAT,  &set_VREF,        NONVOL_VREF_HI,       2000 },    // High trip point value (Volts)
  {EN, "\"WIFI_CONFIG\"",     0,                                 0,                IS_VOID,   &WiFi_setup,      0,                         },    // Set the wifi configuration
  {EN, "\"WIFI_CHANNEL\":",   &json_wifi_channel,                0,                IS_INT32,  0,                NONVOL_WIFI_CHANNEL,     6 },    // Set the wifi channel
  {EN, "\"WIFI_PWD\":",       (int*)&json_wifi_pwd,              0,                IS_SECRET+PWD_SIZE, 0,       NONVOL_WIFI_PWD,         0 },    // Password of SSID to attach to 
  {EN, "\"WIFI_SSID\":",      (int*)&json_wifi_ssid,             0,                IS_TEXT+SSID_SIZE,  0,       NONVOL_WIFI_SSID,        0 },    // Name of SSID to attach to 
  {EX, "\"ZAPPLE\":",         0,                                 0,                IS_VOID,   &zapple,          0,                       0 },    // Start a ZAPPLE console monitor
  {EO, "\"Z_OFFSET\":",       &json_z_offset,                    0,                IS_INT32,  0,                NONVOL_Z_OFFSET,        13 },    // Distance from paper to sensor plane (mm)
  {EX, "\"NORTH_X\":",        &json_north_x,                     0,                IS_INT32,  0,                NONVOL_NORTH_X,          0 },    //
  {EX, "\"NORTH_Y\":",        &json_north_y,                     0,                IS_INT32,  0,                NONVOL_NORTH_Y,          0 },    //
  {EX, "\"EAST_X\":",         &json_east_x,                      0,                IS_INT32,  0,                NONVOL_EAST_X,           0 },    //
  {EX, "\"EAST_Y\":",         &json_east_y,                      0,                IS_INT32,  0,                NONVOL_EAST_Y,           0 },    //
  {EX, "\"SOUTH_X\":",        &json_south_x,                     0,                IS_INT32,  0,                NONVOL_SOUTH_X,          0 },    //
  {EX, "\"SOUTH_Y\":",        &json_south_y,                     0,                IS_INT32,  0,                NONVOL_SOUTH_Y,          0 },    //
  {EX, "\"WEST_X\":",         &json_west_x,                      0,                IS_INT32,  0,                NONVOL_WEST_X,           0 },    //
  {EX, "\"WEST_Y\":",         &json_west_y,                      0,                IS_INT32,  0,                NONVOL_WEST_Y,           0 },    //
  {EN, "\"ATHLETE\":",        &json_athlete,                     0,                IS_TEXT+SMALL_STRING,  0,    0,                       0 },    // Athelete name from PC client
  {EN, "\"EVENT\":",          &json_event,                       0,                IS_TEXT+SMALL_STRING,  0,    0,                       0 },    // Event being shot
  {EN, "\"TARGET_NAME\":",    &json_target_name,                 0,                IS_TEXT+SMALL_STRING,  0,    0,                       0 },    // Name of target being shot
  {0,                     0,                                 0,                0,         0,                0,                       0 },    //

};

int instr(char* s1, char* s2);
static void diag_delay(int x) { printf("\r\n\"DELAY\":%d", x); vTaskDelay(x*1000);  return;}

/*-----------------------------------------------------
 * 
 * @function: freeETarget_json
 * 
 * @brief: Accumulate input from the serial port
 * 
 * @return: None
 * 
 *-----------------------------------------------------
 *
 * The format of the JSON stings used here is
 * 
 * {1, "LABLE":value }
 * 
 * {1, "ECHO":23"}
 * {1, "ECHO":12, "DIP":8}
 * {1, "DIP":9, "SENSOR":230.0, "ECHO":32}
 * {1, "TEST":7, "ECHO":5}
 * {1, "PAPER":1, "DELAY":5, "PAPER":0, "TEST":16}
 * {1, "ATHLETE":"Allan Brown", "EVENT":"Rapid Final", "TARGET_NAME":"AP10m.png"}
 * 
 * Find the lable, ex "DIP": and save in the
 * corresponding memory location
 * 
 *-----------------------------------------------------*/
static unsigned int in_JSON = 0;
static unsigned int got_right_bracket = 0;
static bool not_found;
static bool keep_space;             // Set to 1 if keeping spaces
static bool got_left_bracket;       // Set to 1 if we have a bracket

static int to_int(char h)
{
  h = toupper(h);
  
  if ( h > '9' )
  {
    return 10 + (h-'A');
  }
  else
  {
    return h - '0';
  }
}

void freeETarget_json
(
    void *pvParameters
)
{
  char          ch;

  DLT(DLT_CRITICAL, printf("freeETarget_json()");)

  while (1)
  {
    IF_NOT(IN_OPERATION) { vTaskDelay(ONE_SECOND); continue;}

/*
 * See if anything is waiting and if so, add it in
 */
    while ( serial_available(ALL) != 0 )
    {
      ch = serial_getch(ALL);
      serial_putch(ch, ALL);

/*
 * Parse the stream
 */
      switch (ch)
      {     
        case '}':
          if ( in_JSON != 0 )
          {
            got_left_bracket = false;
            got_right_bracket = in_JSON;
            handle_json();                    // Fall through to reinitialize
          }   

        case '{':
          in_JSON = 0;
          input_JSON[0] = 0;
          got_right_bracket = 0;
          got_left_bracket = true;
          keep_space = 0;
          break;

        case 0x08:                            // Backspace
          if ( in_JSON != 0 )
          {
            in_JSON--;
          }
          input_JSON[in_JSON] = 0;            // Null terminate
          break;

        case '*':                             // Force echo for PC Client
          if ( got_left_bracket == false )    // Whenever we are not between
          {                                   // {}
            POST_version();
            show_echo(EA);
            break;
          }                                   // Otherwise fall through

        case '"':                             // Start or end of text
          keep_space = (keep_space ^ 1) & 1;
        
        default:
          if ( (ch != ' ') || keep_space )
          {
            input_JSON[in_JSON] = ch;            // Add in the latest
            if ( in_JSON < (sizeof(input_JSON)-1) )
            {
            in_JSON++;
            }
            input_JSON[in_JSON] = 0;                  // Null terminate
          }
          break;
      }   // End switch

    }     // End if char available
    vTaskDelay( MIN_DELAY );
  }
  
/*
 *  Never get here
 */
}

/*-----------------------------------------------------
 * 
 * @function: handle_json
 * 
 * @brief:  Breakdown the JSON and handle it
 * 
 * @return: None
 * 
 *-----------------------------------------------------
 *
 * The input stream is parsed one JSON token at a time
 * and the JSON[] is used to determine the action
 * 
 *-----------------------------------------------------*/
 
static void handle_json(void)
{
  int   x;
  float f;
  int   i, j, k;
  char  s[64];          // Place to store a string
  int   m;

/*
 * Found out where the braces are, extract the contents.
 */
  not_found = true;
  for ( i=0; i != got_right_bracket; i++)                       // Go across the JSON input 
  {
    j = 0;                                                      // Index across the JSON token table
    
    while ( (JSON[j].token != 0) )                              // Cycle through the tokens
    {
      x = 0;
      if ( JSON[j].token != 0 )
      {
        k = instr(&input_JSON[i], JSON[j].token );              // Compare the input against the list of JSON tags
        if ( k > 0 )                                            // Non zero, found something
        {
          not_found = false;                                    // Read and convert the JSON value
          switch ( JSON[j].convert & IS_MASK )
          {
            default:
            case IS_VOID:                                       // Void, default to zero
            case IS_FIXED:                                      // Fixed cannot be changed
              x = 0;
            break;

            case IS_TEXT:                                       // Convert to text
            case IS_SECRET:
              while ( input_JSON[i+k] != '"' )                  // Skip to the opening quote
              {
                k++; 
              }
              k++;                                              // Advance to the text

              m = 0;
              s[0] = 0;                                         // Put in a null
              while ( (input_JSON[i+k] != '"') 
                  && (m != (JSON[j].convert & FLOAT_MASK)) )                  // Skip to the opening quote
              {
                s[m] = input_JSON[i+k];      
                m++;
                s[m] = 0;                                      // Null terminate 
                k++;
              }
              if ( JSON[j].value != 0 )                         // Save to json memory
              {
                strcpy((char*)JSON[j].value, &s[0]);
              }        
              if ( JSON[j].non_vol != 0 )                       // Save to persistent storage if present
              {
                nvs_set_str(my_handle, JSON[j].non_vol, s);     // Store into NON-VOL
              }
              break;
              
            case IS_MFS:
            case IS_INT32:                                      // Convert an integer
              if ( (input_JSON[i+k] == '0')
                  && ( (input_JSON[i+k+1] == 'X') || (input_JSON[i+k+1] == 'x')) )  // Is it Hex?
              {
                x = (to_int(input_JSON[i+k+2]) << 4) + to_int(input_JSON[i+k+3]);
              }
              else
              {
                x = atoi(&input_JSON[i+k]);                     // Integer
              }
              if ( JSON[j].value != 0 )
              {
                *JSON[j].value = x;                             // Save the value
              }
              if ( JSON[j].non_vol != 0 )
              {
                nvs_set_i32(my_handle, JSON[j].non_vol, x);    // Store into NON-VOL
              }
              break;
  
            case IS_FLOAT:                                      // Convert a floating point number
              f = atof(&input_JSON[i+k]);                       // Float
              x = f * 1000;                                     // Integer
              if ( JSON[j].d_value != 0 )
              {
                *JSON[j].d_value = f;                             // Working Value
              }
              if ( JSON[j].non_vol != 0 )
              {
                nvs_set_i32(my_handle, JSON[j].non_vol, x);    // Store into NON-VOL as an integer * 1000
              }
              break;
          }

          if ( JSON[j].f != 0 )                               // Call the handler if it is available
          {
            JSON[j].f(x);
          }            
        }
      }
    j++;
    }
     nvs_commit(my_handle);                                   // Save to memory
  }

/*
 * Report an error if input not found
 */
  if ( not_found == true )
  {
    printf("\r\n\r\nCannot decode: {%s}", input_JSON);
  }
  
/*
 * All done
 */   
  in_JSON   = 0;                // Start Over
  got_right_bracket = 0;        // Need to wait for a new Right Bracket
  got_left_bracket = false;
  input_JSON[in_JSON] = 0;      // Clear the input
  return;
}

// Compare two strings.  Return -1 if not equal, length of string if equal
// S1 Long String, S2 Short String . if ( instr("CAT Sam", "CAT") == 3)
int instr(char* s1, char* s2)
{
  int i;

  i=0;
  while ( (*s1 != 0) && (*s2 != 0) )
  {
    if ( *s1 != *s2 )
    {
      return -1;
    }
    s1++;
    s2++;
    i++;
  }

/*
 * Reached the end of the comparison string. Check that we arrived at a NULL
 */
  if ( *s2 == 0 )       // Both strings are the same
  {
    return i;
  }
  
  return -1;                            // The strings are different
}

/*-----------------------------------------------------
 * 
 * @function: show_echo
 * 
 * @brief: Display the current settings
 * 
 * @return: None
 * 
 *-----------------------------------------------------
 *
 * Loop and display the settings
 * 
 *-----------------------------------------------------*/

void show_echo
(
  unsigned int level            // Filter level
)
{
  unsigned int i, j;
  char str_c[32];   // String holding buffers
  mfs_action_t* mfs_ptr;

  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF) )
  {
    SEND(sprintf(_xs, "\r\n{\r\n\"NAME\":\"%s\", \r\n", names[json_name_id]);)
  }
  else
  {
    SEND(sprintf(_xs, "\r\n{\r\n\"NAME\":\"%s\", \r\n", names[json_name_id+my_ring]);)
  }

/*
 * Loop through all of the JSON tokens
 */

  i=0;
  while ( JSON[i].token != 0 )                 // Still more to go?  
  {
    if ( ((JSON[i].level & level) || (level == 0))
      && ((JSON[i].value != NULL) || (JSON[i].d_value != NULL)) )              // It has a value ?
    {
      switch ( JSON[i].convert & IS_MASK )              // Display based on it's type
      {
        default:
        case IS_VOID:
          break;

        case IS_TEXT:
        case IS_SECRET:
            j = 0;
            while ( *((char*)(JSON[i].value)+j) != 0)
            {
              if ( (JSON[i].convert & IS_MASK) == IS_SECRET )
              {
                str_c[j] = '*';
              }
              else
              {
                str_c[j] = *((char*)(JSON[i].value)+j);
              }
              j++;
            }
            str_c[j] = 0;
            SEND(sprintf(_xs, "%-18s \"%s\", \r\n", JSON[i].token, str_c);)
            break;
            
          case IS_MFS:                                        // Covert to a switch ID
              mfs_ptr = mfs_find(*JSON[i].value);
              if ( mfs_ptr != NULL )
              {
                SEND(sprintf(_xs, "%-18s \"(%d) - %s\", \r\n", JSON[i].token, *JSON[i].value, mfs_ptr->text);)
              }
          break;

        case IS_INT32:
        case IS_FIXED:
          SEND(sprintf(_xs, "%-18s %d, \r\n", JSON[i].token, *JSON[i].value);)
          break;

        case IS_FLOAT:
          SEND(sprintf(_xs, "%-18s %6.2f, \r\n", JSON[i].token, *JSON[i].d_value);)
          break;
      }
      vTaskDelay(10); 
    }
    i++;
  }
  
/*
 * Finish up with the special cases
 */
  SEND(sprintf(_xs, "\n\rStatus\r\n");)                                                                    // Blank Line
  SEND(sprintf(_xs, "\"TRACE\":             %d, \n\r", is_trace);)         // TRUE to if trace is enabled
  SEND(sprintf(_xs, "\"RUN_STATE\":         %d, \n\r", run_state);)    // TRUE to if trace is enabled
  SEND(sprintf(_xs, "\"RUNNING_MINUTES\":  %10.6f, \n\r", esp_timer_get_time()/100000.0/60.0);)  // On Time
  SEND(sprintf(_xs, "\"TIME_TO_SLEEP\":     %4.2f, \n\r", (float)power_save/(float)(ONE_SECOND*60));)                 // How long until we sleep
  SEND(sprintf(_xs, "\"TEMPERATURE\":       %4.2f, \n\r", temperature_C());)                          // Temperature in degrees C
  SEND(sprintf(_xs, "\"RELATIVE_HUMIDITY\": %4.2f, \n\r", humidity_RH());)
  SEND(sprintf(_xs, "\"SPEED_OF_SOUND\":    %4.2f, \n\r", speed_of_sound(temperature_C(), humidity_RH()));)
  SEND(sprintf(_xs, "\"TIMER_COUNT\":       %d, \n\r", (int)(SHOT_TIME * OSCILLATOR_MHZ));)             // Maximum number of clock cycles to record shot (target dependent)
  SEND(sprintf(_xs, "\"V12\":               %4.2f, \n\r", v12_supply());)    // 12 Volt LED supply
  WiFi_MAC_address(str_c);
  SEND(sprintf(_xs, "\"WiFi_MAC\":          \"%02X:%02X:%02X:%02X:%02X:%02X\", \n\r", str_c[0], str_c[1],str_c[2], str_c[3], str_c[4], str_c[5]);)
  WiFi_my_IP_address(str_c);
  SEND(sprintf(_xs, "\"WiFi_IP_ADDRESS\":   \"%s\", \n\r", str_c);)

  if ( json_wifi_ssid[0] == 0 )                       // The SSID is undefined
  {
    SEND(sprintf(_xs, "\"WiFi_MODE\":         \"Access Point: FET-%s\",\n\r", names[json_name_id]);)    // Print out the IP address
  }
  else
  {
    SEND(sprintf(_xs, "\"WiFi_MODE\":         \"Station connected to SSID %s\",\n\r", (char*)&json_wifi_ssid);) 
  }

  SEND(sprintf(_xs, "\"TOKEN_RING\":        %d, \n\r", my_ring);)           // My token ring address
  SEND(sprintf(_xs, "\"TOKEN_OWNER\":       %d, \n\r", whos_ring);)         // Who owns the token ring
  
  SEND(sprintf(_xs, "\"VERSION\":           %s, \n\r", SOFTWARE_VERSION);)        // Current software version
  nvs_get_i32(my_handle, NONVOL_PS_VERSION, &j);
  SEND(sprintf(_xs, "\"PS_VERSION\":        %d, \n\r", j);)                    // Current persistent storage version
  SEND(sprintf(_xs, "\"BD_REV\":            %4.2f \n\r", (float)(revision())/100.0);)                                             // Current board versoin
  SEND(sprintf(_xs, "}\r\n");) 
  
/*
 *  All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * @function: show_names
 * 
 * @brief: Display the list of names
 * 
 * @return: None
 * 
 *-----------------------------------------------------
 *
 * If the name is Loop and display the settings
 * 
 *-----------------------------------------------------*/

static void show_names(int v)
{
  unsigned int i;

  if ( v != 0 )
  {
    return;
  }
  
  printf("\r\nNames\r\n");
  
  i=0;
  while (names[i] != 0 )
  {
    printf("%d: \"%s\", \r\n", i, names[i]);
    i++;
  }

/*
 *  All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * @function: show_test
 * 
 * @brief: Execute one iteration of the self test
 * 
 * @return: None67!
 * 
 *----------------------------------------------------*/

static void show_test(int test_number)
 {
  printf("\r\nSelf Test: %d\r\n", test_number);
  
  self_test(test_number);
  return;
 }

 /*-----------------------------------------------------
 * 
 * @function: set_trace
 * 
 * @brief:    Turn the software trace on and off
 * 
 * @return: None
 * 
 *-----------------------------------------------------
 *
 * Uset the trace to set the DIP switch
 * 
 *-----------------------------------------------------*/
 static void set_trace
   (
   int trace                // Trace on or off
   )
 {
  trace |= DLT_CRITICAL;        // Critical is always enabled
    
  if ( trace & DLT_CRITICAL)    {SEND(sprintf(_xs, "\r\r%03d DLT CRITICAL", DLT_CRITICAL);)}
  if ( trace & DLT_APPLICATION) {SEND(sprintf(_xs, "\r\n%03d DLT APPLICATON", DLT_APPLICATION);)}
  if ( trace & DLT_DIAG)        {SEND(sprintf(_xs, "\r\n%03d DLT DIAG", DLT_DIAG);)}
  if ( trace & DLT_INFO)        {SEND(sprintf(_xs, "\r\n%03d DLT INFO", DLT_INFO);)}
  SEND(sprintf(_xs, "\r\n");)
  
  is_trace = trace;
  return;   
 }
