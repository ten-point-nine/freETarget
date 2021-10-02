/*-------------------------------------------------------
 * 
 * JSON.ino
 * 
 * JSON driver
 * 
 * ----------------------------------------------------*/

#include <EEPROM.h>
#include "json.h"
#include "esp-01.h"
#include "nonvol.h"

static char input_JSON[256];

int     json_dip_switch;            // DIP switch overwritten by JSON message
double  json_sensor_dia = DIAMETER; // Sensor daiamter overwitten by JSON message
int     json_sensor_angle;          // Angle sensors are rotated through
int     json_paper_time = 0;        // Time paper motor is applied
int     json_echo;                  // Test String 
double  json_d_echo;                // Test String
int     json_test;                  // Self test to be performed
int     json_calibre_x10;           // Pellet Calibre
int     json_north_x;               // North Adjustment
int     json_north_y;
int     json_east_x;                // East Adjustment
int     json_east_y;
int     json_south_x;               // South Adjustment
int     json_south_y;
int     json_west_x;                // WestAdjustment
int     json_west_y;
int     json_name_id;               // Name identifier
int     json_1_ring_x10;            // Size of the 1 ring in mm
int     json_LED_PWM;               // LED control value 
int     json_power_save;            // Power down time
int     json_send_miss;             // Send a miss message
int     json_serial_number;         // Electonic serial number
int     json_step_count;            // Number of steps ouput to motor
int     json_step_time;             // Duration of each step
int     json_multifunction;         // Multifunction switch operation
int     json_z_offset;              // Distance between paper and sensor plane in 0.1mm
int     json_paper_eco;             // Do not advance paper if outside of the black
int     json_target_type;           // Modify target type (0 == single bull)

int     temp;                       // Temporary variable

#define JSON_DEBUG false                    // TRUE to echo DEBUG messages

       void show_echo(int v);               // Display the current settings
static void show_test(int v);               // Execute the self test once
static void show_test0(int v);              // Help Menu
static void show_names(int v);
static void nop(void);
static void set_trace(int v);               // Set the trace on and off


  
const json_message JSON[] = {
//    token                 value stored in RAM     double stored in RAM         type     service fcn()     NONVOL location      Initial Value
  {"\"ANGLE\":",          &json_sensor_angle,                0,                IS_INT16,  0,                NONVOL_SENSOR_ANGLE,    45 },    // Locate the sensor angles
  {"\"CAL\":",            0,                                 0,                IS_VOID,   &set_trip_point,                  0,       0 },    // Enter calibration mode
  {"\"CALIBREx10\":",     &json_calibre_x10,                 0,                IS_INT16,  0,                NONVOL_CALIBRE_X10,     45 },    // Enter the projectile calibre (mm x 10)
  {"\"DIP\":",            &json_dip_switch,                  0,                IS_INT16,  0,                NONVOL_DIP_SWITCH,       0 },    // Remotely set the DIP switch
  {"\"ECHO\":",           0,                                 0,                IS_VOID,   &show_echo,                       0,       0 },    // Echo test
  {"\"INIT\":",           0,                                 0,                IS_FIXED,  &init_nonvol,                     0,       0 },    // Initialize the NONVOL memory
  {"\"LED_BRIGHT\":",     &json_LED_PWM,                     0,                IS_INT16,  &set_LED_PWM_now, NONVOL_LED_PWM,         50 },    // Set the LED brightness
  {"\"MFS\":",            &json_multifunction,               0,                IS_INT16,  0,                NONVOL_MFS,              0 },    // Multifunction switch action
  {"\"NAME_ID\":",        &json_name_id,                     0,                IS_INT16,  &show_names,      NONVOL_NAME_ID,          0 },    // Give the board a name
  {"\"PAPER_ECO\":",      &json_paper_eco,                   0,                IS_INT16,  0,                NONVOL_PAPER_ECO,        0 },    // Ony advance the paper is in the black
  {"\"PAPER_TIME\":",     &json_paper_time,                  0,                IS_INT16,  0,                NONVOL_PAPER_TIME,       0 },    // Set the paper advance time
  {"\"POWER_SAVE\":",     &json_power_save,                  0,                IS_INT16,  0,                NONVOL_POWER_SAVE,      30 },    // Set the power saver time
  {"\"SEND_MISS\":",      &json_send_miss,                   0,                IS_INT16,  0,                NONVOL_SEND_MISS,        0 },    // Enable / Disable sending miss messages
  {"\"SENSOR\":",         0,                                 &json_sensor_dia, IS_FLOAT,  &gen_position,    NONVOL_SENSOR_DIA,     230 },    // Generate the sensor postion array
  {"\"SN\":",             &json_serial_number,               0,                IS_FIXED,  0,                NONVOL_SERIAL_NO,   0xffff },    // Board serial number
  {"\"STEP_COUNT\":",     &json_step_count,                  0,                IS_INT16,  0,                NONVOL_STEP_COUNT,       0 },    // Set the duration of the stepper motor ON time
  {"\"STEP_TIME\":",      &json_step_time,                   0,                IS_INT16,  0,                NONVOL_STEP_TIME,        0 },    // Set the number of times stepper motor is stepped
  {"\"TARGET_TYPE\":",    &json_target_type,                 0,                IS_INT16,  0,                NONVOL_TARGET_TYPE,      0 },    // Marify shot location (0 == Single Bull)
  {"\"TEST\":",           &json_test,                        0,                IS_INT16,  &show_test,       NONVOL_TEST_MODE,        0 },    // Execute a self test
  {"\"TRACE\":",          0,                                 0,                IS_INT16,  &set_trace,                      0,        0 },    // Enter / exit diagnostic trace
/*  
  {"\"TRGT_1_RINGx10\":", &json_1_ring_x10,                  0,                IS_INT16,  0,                NONVOL_1_RINGx10,     1555 },    // Enter the 1 ring diamater (mm x 10)
*/
  {"\"VERSION\":",        0,                                 0,                IS_INT16,  &POST_version,                   0,        0 },    // Return the version string
  {"\"Z_OFFSET\":",       &json_z_offset    ,                0,                IS_INT16,  0,                NONVOL_Z_OFFSET,         0 },    // Distance from paper to sensor plane (mm)

  {"\"NORTH_X\":",        &json_north_x,                     0,                IS_INT16,  0,                NONVOL_NORTH_X,          0 },    //
  {"\"NORTH_Y\":",        &json_north_y,                     0,                IS_INT16,  0,                NONVOL_NORTH_Y,          0 },    //
  {"\"EAST_X\":",         &json_east_x,                      0,                IS_INT16,  0,                NONVOL_EAST_X,           0 },    //
  {"\"EAST_Y\":",         &json_east_y,                      0,                IS_INT16,  0,                NONVOL_EAST_Y,           0 },    //
  {"\"SOUTH_X\":",        &json_south_x,                     0,                IS_INT16,  0,                NONVOL_SOUTH_X,          0 },    //
  {"\"SOUTH_Y\":",        &json_south_y,                     0,                IS_INT16,  0,                NONVOL_SOUTH_Y,          0 },    //
  {"\"WEST_X\":",         &json_west_x,                      0,                IS_INT16,  0,                NONVOL_WEST_X,           0 },    //
  {"\"WEST_Y\":",         &json_west_y,                      0,                IS_INT16,  0,                NONVOL_WEST_Y,           0 },    //

{ 0, 0, 0, 0, 0, 0}
};

int instr(char* s1, char* s2);

/*-----------------------------------------------------
 * 
 * function: read_JSON()
 * 
 * brief: Accumulate input from the serial port
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * The format of the JSON stings used here is
 * 
 * { "LABLE":value }
 * 
 * {"ECHO":23"}
 * {"ECHO":12, "DIP":8}
 * {"DIP":9, "SENSOR":230.0, "ECHO":32}
 * {"TEST":7, "ECHO":5}
 * 
 * Find the lable, ex "DIP": and save in the
 * corresponding memory location
 * 
 *-----------------------------------------------------*/
static uint16_t in_JSON = 0;
static bool not_found;

bool read_JSON(void)
{
static int16_t got_right;
unsigned int  i, j, x;
int     k;
char    ch;
double  y;
bool    return_value;

  return_value = false;
  
/*
 * See if anything is waiting and if so, add it in
 */
  while ( AVAILABLE != 0)
  {
    return_value = true;
    
    ch = GET();

#if ( JSON_DEBUG == true )
    char_to_all(ch);
#endif

/*
 * Parse the stream
 */
    switch (ch)
    {
      case ' ':                             // Ignore spaces
        break;

      case '%':
        self_test(T_XFR_LOOP);             // Transfer self test
        break;
        
      case '^':
        drive_paper();                     // Page Feed
        break;

      case '{':
        in_JSON = 0;
        input_JSON[0] = 0;
        got_right = 0;
        break;

      case '}':
        if ( in_JSON != 0 )
        {
          got_right = in_JSON;
        }
        break;

      default:
        input_JSON[in_JSON] = ch;            // Add in the latest
        if ( in_JSON < (sizeof(input_JSON)-1) )
        {
        in_JSON++;
        }
        input_JSON[in_JSON] = 0;            // Null terminate
        break;
    }
  }
  
  if ( got_right == 0 )
  {
    return return_value;
  }
  
/*
 * Found out where the braces are, extract the contents.
 */
  not_found = true;
  for ( i=0; i != got_right; i++)                             // Go across the JSON input 
  {
    j = 0;

    while ( JSON[j].token != 0 )                              // Cycle through the tokens
    {
      k = instr(&input_JSON[i], JSON[j].token );              // Compare the input against the list of JSON tags

      if ( k > 0 )                                            // Non zero, found something
      {
        not_found = false;                                    // Read and convert the JSON value
        switch ( JSON[j].convert )
        {
          default:
          case IS_VOID:                                       // Void, default to zero
          case IS_FIXED:                                      // Fixed cannot be changed
            x = 0;
            y = 0;
          break;
            
          case IS_INT16:                                      // Convert an integer
            x = atoi(&input_JSON[i+k]);
            if ( JSON[j].value != 0 )
            {
              *JSON[j].value = x;                             // Save the value
            }
            if ( JSON[j].non_vol != 0 )
            {
              EEPROM.put(JSON[j].non_vol, x);                 // Store into NON-VOL
            }
            
            break;
  
          case IS_FLOAT:                                      // Convert a floating point number
          case IS_DOUBLE:
            y = atof(&input_JSON[i+k]);
            if ( JSON[j].d_value != 0 )
            {
              *JSON[j].d_value = y;                           // Save the value
            }
            if ( JSON[j].non_vol != 0 )
            {
              EEPROM.put(JSON[j].non_vol, y);                 // Store into NON-VOL
            }
            break;
        }

        if ( JSON[j].f != 0 )                                 // Call the handler if it is available
        {
          JSON[j].f(x);
        }
      } 
     j++;
   }
  }
/*
 * Report an error if input not found
 */
  if ( not_found == true )
  {
    Serial.print("\r\n\r\nCannot decode: {"); Serial.print(input_JSON); Serial.print("}. Use"); 
    j = 0;    
    while ( JSON[j].token != 0 ) 
    {
      Serial.print("\r\n"); Serial.print(JSON[j].token);
      j++;
      if ( (j%4) == 0 ) 
      {
        Serial.print("\r\n");
      }
    }
  }
  
/*
 * All done
 */   
  in_JSON   = 0;                // Start Over
  got_right = 0;                // Neet to wait for a new Right Bracket
  input_JSON[in_JSON] = 0;      // Clear the input
  return return_value;
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
 * function: show_echo
 * 
 * brief: Display the current settings
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * Loop and display the settings
 * 
 *-----------------------------------------------------*/

void show_echo(int v)
{
  unsigned int i, j;
  char   s[512], str_c[10];  // String holding buffers

  sprintf(s, "\r\n{\r\n\"NAME\":\"%s\", \r\n", names[json_name_id]);
  
  output_to_all(s);
    
/*
 * Loop through all of the JSON tokens
 */
  i=0;
  j=1;
  while ( JSON[i].token != 0 )                 // Still more to go?  
  {
    if ( (JSON[i].value != NULL) || (JSON[i].d_value != NULL) )              // It has a value ?
    {
      switch ( JSON[i].convert )              // Display based on it's type
      {
        default:
        case IS_VOID:
          break;
          
        case IS_INT16:
        case IS_FIXED:
          sprintf(s, "%s %d, \r\n", JSON[i].token, *JSON[i].value);
          break;

        case IS_FLOAT:
        case IS_DOUBLE:
          dtostrf(*JSON[i].d_value, 4, 2, str_c );
          sprintf(s, "%s %s, \r\n", JSON[i].token, str_c);
          break;
      }
      output_to_all(s);
    }
    i++;
  }
  
/*
 * Finish up with the special cases
 */
  sprintf(s, "\"IS_TRACE\": %d, \n\r", is_trace);                                         // TRUE to if trace is enabled
  output_to_all(s);
  
  dtostrf(temperature_C(), 4, 2, str_c );
  sprintf(s, "\"TEMPERATURE\": %s, \n\r", str_c);                                         // Temperature in degrees C
  output_to_all(s);
  
  dtostrf(speed_of_sound(temperature_C(), RH_50), 4, 2, str_c );
  sprintf(s, "\"SPEED_SOUND\": %s, \n\r", str_c);                                         // Speed of Sound
  output_to_all(s);

  dtostrf(TO_VOLTS(analogRead(V_REFERENCE)), 4, 2, str_c );
  sprintf(s, "\"V_REF\": %s, \n\r", str_c);                                               // Trip point reference
  output_to_all(s);

  sprintf(s, "\"TIMER_COUNT\":%d, \n\r", (int)(SHOT_TIME * OSCILLATOR_MHZ));              // Maximum number of clock cycles to record shot (target dependent)
  output_to_all(s);

  sprintf(s, "\"DIP_HEX\": 0x%c, \n\r", to_hex[0x0F & read_DIP()]);                       // DIP switch status
  output_to_all(s);

  sprintf(s, "\"WiFi\": %d, \n\r", esp01_is_present());                                 // TRUE if WiFi is available
  output_to_all(s);

  sprintf(s, "\"VERSION\": %s, \n\r", SOFTWARE_VERSION);                                  // Current software version
  output_to_all(s); 

  dtostrf(revision()/100.0, 4, 2, str_c );              
  sprintf(s, "\"BD_REV\": %s \n\r", str_c);                                               // Current board versoin
  output_to_all(s);
  
  sprintf(s, "}\r\n"); 
  output_to_all(s);
  output_to_all(0);
  
/*
 *  All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * function: show_names
 * 
 * brief: Display the list of names
 * 
 * return: None
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
  
  Serial.print("\r\n{\r\n");
  
  i=0;
  while (names[i] != 0 )
  {
    Serial.print("\"NAME_"); Serial.print(i); Serial.print("\": \"");  Serial.print(names[i]); Serial.print("\", \r\n");
    i++;
  }
  Serial.print("}\r\n");
/*
 *  All done, return
 */
  return;
}

/*-----------------------------------------------------
 * 
 * function: show_test
 * 
 * brief: Execute one iteration of the self test
 * 
 * return: None67!
 * 
 *----------------------------------------------------*/

static void show_test(int test_number)
 {
  Serial.print("\r\nSelf Test:"); Serial.print(test_number); Serial.print("\r\n");
  
  self_test(test_number);
  return;
 }

 /*-----------------------------------------------------
 * 
 * function: set_trace
 * 
 * brief:    Turn the software trace on and off
 * 
 * return: None
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
   Serial.print("\r\nTrace: ");
   
   if ( trace == 0 )
   {
      is_trace = 0;
      Serial.print("OFF");
   }
   else
   {
      is_trace = 1;
      Serial.print("ON");
   }

   Serial.print("\r\n");
   
  /*
   * The DIP switch has been remotely set
   */
    return;   
 }
