/*-------------------------------------------------------
 * 
 * JSON.ino
 * 
 * JSON driver
 * 
 * ----------------------------------------------------*/

#include <EEPROM.h>

static char input_JSON[256];

int     json_dip_switch;            // DIP switch overwritten by JSON message
double  json_sensor_dia = DIAMETER; // Sensor daiamter overwitten by JSON message
int     json_sensor_angle;          // Angle sensors are rotated through
int     json_paper_time = 0;        // Time paper motor is applied
int     json_echo;                  // Test String 
double  json_d_echo;                // Test String
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
int     json_tabata_enable;         // Tabata feature enabled
int     json_tabata_on;             // Tabata ON timer
int     json_tabata_rest;           // Tabata resting timer
unsigned long json_rapid_on;        // Rapid Fire ON timer
int     json_vset_PWM;              // Starting PWM value
double  json_vset;                  // Desired VREF setting
int     json_follow_through;        // Follow through delay
int     json_keep_alive;            // Keep alive period
int     json_tabata_warn_on;        // Tabata warning time light on
int     json_tabata_warn_off;       // Tabata warning time to shot
int     json_face_strike;           // Number of cycles to accept a strike
int     json_wifi_channel;          // Wifi channel
int     json_rapid_count;           // Number of shots expected in string
int     json_rapid_enable;          // Set to TRUE if the rapid fire event is enabled
int     json_rapid_time;            // When will the rapid fire event end?
int     json_rapid_wait;            // Delay applied to rapid start
char    json_wifi_ssid[esp01_SSID_SIZE_32]; // Stored value of SSID
char    json_wifi_pwd[esp01_PWD_SIZE];// Stored value of password
int     json_wifi_dhcp;             // The ESP is a DHCP server
int     json_rh;                    // Relative Humidity 0-1005
int     json_min_ring_time;         // Time to wait for ringing to stop
double  json_doppler;               // Adjust for dopper inverse square

#define JSON_DEBUG false            // TRUE to echo DEBUG messages

       void show_echo(void);        // Display the current settings
static void show_test(int v);       // Execute the self test once
static void show_test0(int v);      // Help Menu
static void show_names(int v);
static void nop(void);
static void set_trace(int v);       // Set the trace on and off

  
const json_message JSON[] = {
//    token                 value stored in RAM     double stored in RAM        convert    service fcn()     NONVOL location      Initial Value
  {"\"ANGLE\":",          &json_sensor_angle,                0,                IS_INT16,  0,                NONVOL_SENSOR_ANGLE,    45 },    // Locate the sensor angles
  {"\"BYE\":",            0,                                 0,                IS_VOID,   &bye,                             0,       0 },    // Shut down the target
  {"\"CAL\":",            0,                                 0,                IS_VOID,   &set_trip_point,                  0,       0 },    // Enter calibration mode
  {"\"CALIBREx10\":",     &json_calibre_x10,                 0,                IS_INT16,  0,                NONVOL_CALIBRE_X10,     45 },    // Enter the projectile calibre (mm x 10)
  {"\"DELAY\":",          0               ,                  0,                IS_INT16,  &diag_delay,                      0,       0 },    // Delay TBD seconds
  {"\"DIP\":",            &json_dip_switch,                  0,                IS_INT16,  0,                NONVOL_DIP_SWITCH,       0 },    // Remotely set the DIP switch
  {"\"DOPPLER\":",        0,                     &json_doppler,                IS_FLOAT,  0,                NONVOL_DOPPLER, (7.0d/(700.0d * 700.0d))},    // Adjust timing based on Doppler Inverse SQ
  {"\"ECHO\":",           0,                                 0,                IS_VOID,   &show_echo,                       0,       0 },    // Echo test
  {"\"FACE_STRIKE\":",    &json_face_strike,                 0,                IS_INT16,  0,                NONVOL_FACE_STRIKE,      5 },    // Face Strike Count 
  {"\"FOLLOW_THROUGH\":", &json_follow_through,              0,                IS_INT16,  0,                NONVOL_FOLLOW_THROUGH,   0 },    // Three second follow through
  {"\"INIT\":",           0,                                 0,                IS_INT16,  &init_nonvol,     NONVOL_INIT,             0 },    // Initialize the NONVOL memory
  {"\"KEEP_ALIVE\":",     &json_keep_alive,                  0,                IS_INT16,  0,                NONVOL_KEEP_ALIVE,     120 },    // TCPIP Keep alive period (in seconds)
  {"\"LED_BRIGHT\":",     &json_LED_PWM,                     0,                IS_INT16,  &set_LED_PWM_now, NONVOL_LED_PWM,         50 },    // Set the LED brightness
  {"\"MFS\":",            &json_multifunction,               0,                IS_INT16,  0,                NONVOL_MFS,  (LED_ADJUST*10000) 
                                                                                                                          + (POWER_TAP * 1000)
                                                                                                                          + (PAPER_SHOT * 100) 
                                                                                                                          + (ON_OFF * 10) 
                                                                                                                          + (PAPER_FEED) },  // Multifunction switch action
  {"\"MIN_RING_TIME\":",  &json_min_ring_time,               0,                IS_INT16,  0,                NONVOL_MIN_RING_TIME,  500 },    // Minimum time for ringing to stop (ms)
  {"\"NAME_ID\":",        &json_name_id,                     0,                IS_INT16,  &show_names,      NONVOL_NAME_ID,          0 },    // Give the board a name
  {"\"NONVOL_BACKUP\":",  0,                                 0,                IS_VOID,   &backup_nonvol,   0,                       0 },    // Backup the NONVOL
  {"\"NONVOL_RESTORE\":", 0,                                 0,                IS_VOID,   &restore_nonvol,  0,                       0 },    // Restore the NONVOL
  {"\"PAPER_ECO\":",      &json_paper_eco,                   0,                IS_INT16,  0,                NONVOL_PAPER_ECO,        0 },    // Ony advance the paper is in the black
  {"\"PAPER_TIME\":",     &json_paper_time,                  0,                IS_INT16,  0,                NONVOL_PAPER_TIME,      50 },    // Set the paper advance time
  {"\"POWER_SAVE\":",     &json_power_save,                  0,                IS_INT16,  0,                NONVOL_POWER_SAVE,      30 },    // Set the power saver time
  {"\"RAPID_COUNT\":",    &json_rapid_count,                 0,                IS_INT16,  0,                0,                       0 },    // Number of shots expected in series
  {"\"RAPID_ENABLE\":",   &json_rapid_enable,                0,                IS_INT16,  &rapid_enable,    0,                       0 },    // Enable the rapid fire fieature
  {"\"RAPID_TIME\":",     &json_rapid_time,                  0,                IS_INT16,  0,                0,                       0 },    // Set the duration of the rapid fire event and start
  {"\"RAPID_WAIT\":",     &json_rapid_wait,                  0,                IS_INT16,  0,                0,                       0 },    // Delay applied between enable and ready
  {"\"RESET\":",          0,                                 0,                IS_INT16,  &setup,           0,                       0 },    // Reinit the board
  {"\"RH\":",             &json_rh,                          0,                IS_INT16,  0,                NONVOL_RH,              50 },    // Relative Humidity
  {"\"SEND_MISS\":",      &json_send_miss,                   0,                IS_INT16,  0,                NONVOL_SEND_MISS,        0 },    // Enable / Disable sending miss messages
  {"\"SENSOR\":",         0,                                 &json_sensor_dia, IS_FLOAT,  &gen_position,    NONVOL_SENSOR_DIA,     230 },    // Generate the sensor postion array
  {"\"SN\":",             &json_serial_number,               0,                IS_FIXED,  0,                NONVOL_SERIAL_NO,   0xffff },    // Board serial number
  {"\"STEP_COUNT\":",     &json_step_count,                  0,                IS_INT16,  0,                NONVOL_STEP_COUNT,       0 },    // Set the duration of the stepper motor ON time
  {"\"STEP_TIME\":",      &json_step_time,                   0,                IS_INT16,  0,                NONVOL_STEP_TIME,        0 },    // Set the number of times stepper motor is stepped
  {"\"TABATA_ENABLE\":",  &json_tabata_enable,               0,                IS_INT16,  &tabata_enable,   0,                       0 },    // Enable the tabata feature
  {"\"TABATA_ON\":",      &json_tabata_on,                   0,                IS_INT16,  0,                0,                       0 },    // Time that the LEDs are ON for a Tabata timer (1/10 seconds)
  {"\"TABATA_REST\":",    &json_tabata_rest,                 0,                IS_INT16,  0,                0,                       0 },    // Time that the LEDs are OFF for a Tabata timer
  {"\"TABATA_WARN_OFF\":",&json_tabata_warn_off,             0,                IS_INT16,  0,                0,                       0 },    // Time that the LEDs are ON during a warning cycle
  {"\"TABATA_WARN_ON\":", &json_tabata_warn_on,              0,                IS_INT16,  0,                0,                     200 },    // Time that the LEDs are OFF during a warning cycle
  {"\"TARGET_TYPE\":",    &json_target_type,                 0,                IS_INT16,  0,                NONVOL_TARGET_TYPE,      0 },    // Marify shot location (0 == Single Bull)
  {"\"TEST\":",           0,                                 0,                IS_INT16,  &show_test,       NONVOL_TEST_MODE,        0 },    // Execute a self test
  {"\"TRACE\":",          0,                                 0,                IS_INT16,  &set_trace,                      0,        0 },    // Enter / exit diagnostic trace
  {"\"VERSION\":",        0,                                 0,                IS_INT16,  &POST_version,                   0,        0 },    // Return the version string
  {"\"V_SET\":",          0,                                 &json_vset,       IS_FLOAT,  &compute_vset_PWM,NONVOL_VSET,             0 },    // Set the voltage reference
  {"\"WIFI_CHANNEL\":",   &json_wifi_channel,                0,                IS_INT16,  0,                NONVOL_WIFI_CHANNEL,     6 },    // Set the wifi channel
  {"\"WIFI_PWD\":",       (int*)&json_wifi_pwd,              0,                IS_SECRET, 0,                NONVOL_WIFI_PWD,         0 },    // Password of SSID to attach to 
  {"\"WIFI_SSID\":",      (int*)&json_wifi_ssid,             0,                IS_TEXT,   0,                NONVOL_WIFI_SSID_32,     0 },    // Name of SSID to attach to 
  {"\"Z_OFFSET\":",       &json_z_offset,                    0,                IS_INT16,  0,                NONVOL_Z_OFFSET,        13 },    // Distance from paper to sensor plane (mm)
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
static void diag_delay(int x) { Serial.print(T("\r\n\"DELAY\":")); Serial.print(x); delay(x*1000);  return;}

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
 * {"PAPER":1, "DELAY":5, "PAPER":0, "TEST":16}
 * 
 * Find the lable, ex "DIP": and save in the
 * corresponding memory location
 * 
 *-----------------------------------------------------*/
static uint16_t in_JSON = 0;
static int16_t got_right = false;
static bool not_found;
static bool keep_space;   // Set to 1 if keeping spaces

bool read_JSON(void)
{
  unsigned int  i;      // Index across JSON message
  unsigned int  j;      // Index across JSON token table (JSON[])
           int  k;      // Result from string compare (-Ve => not found)
  unsigned int  l;      // Index across the gpio init table (init_table)
  unsigned int  m;      // Index into NONVOL text storage
  unsigned int  x;      // Temporary working variable
  char*    s;           // Pointer to stored text string
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
    if ( ch == '*' )
    {
      ch = '"';                             // Fix for European keyboards(?)
    }
    if ( ch == '?' )
    {
      show_echo();                         // Show status if ? entered
      return;
    }
/*
 * Parse the stream
 */
    switch (ch)
    {        
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
        keep_space = 0;
        break;

      case '}':
        if ( in_JSON != 0 )
        {
          got_right = in_JSON;
        }
        break;

      case 0x08:                            // Backspace
        if ( in_JSON != 0 )
        {
          in_JSON--;
        }
        input_JSON[in_JSON] = 0;            // Null terminate
        break;

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
        }
        break;
    }
    input_JSON[in_JSON] = 0;                  // Null terminate
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
    j = 0;                                                      // Index across the JSON token table
    l = 0;                                                      // Index across gpio init table
    
    while ( (JSON[j].token != 0) || (init_table[l].port != 0xff) ) // Cycle through the tokens
    {
      if ( JSON[j].token != 0 )
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
                        
            case IS_TEXT:                                       // Convert to text
            case IS_SECRET:
              while ( input_JSON[i+k] != '"' )                  // Skip to the opening quote
              {
                k++; 
              }
              k++;                                              // Advance to the text

              s = (char *)JSON[j].value;                        // Fake a pointer to text
              *s = 0;                                           // Put in a null
              m = 0;
              EEPROM.put(JSON[j].non_vol, 0);                   // Put in a null
              while ( input_JSON[i+k] != '"' )                  // Skip to the opening quote
              {
                if ( s != 0 )
                {
                   *s = input_JSON[i+k];                        // Save the value
                   s++;
                   *s = 0;                                      // Null terminate 
                }
                
                if ( JSON[j].non_vol != 0 )                     // Save to persistent storage if present
                {
                  EEPROM.put(JSON[j].non_vol+m, input_JSON[i+k]); // Store into NON-VOL
                  m++;
                  EEPROM.put(JSON[j].non_vol+m, 0);             // Null terminate
                }
                k++;
              }
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
            
            if ( JSON[j].f != 0 )                               // Call the handler if it is available
            {
              JSON[j].f(x);
            }

            
          }
        }
        if ( init_table[l].port != 0xff )                  // Cycle through the tokens
        {
          k = instr(&input_JSON[i], init_table[l].gpio_name );    // Compare the input against the list of JSON tags
          if ( k > 0 )                                            // Non zero, found something
          {
            not_found = false;                                    // Read and convert the JSON value
            Serial.print(T("\r\n")); Serial.print(init_table[j].gpio_name);
            if ( init_table[j].in_or_out == INPUT_PULLUP )
              {
                Serial.print(T(" is input only"));
              break;
            }
          
            if ( instr("LED_PWM", init_table[j].gpio_name ) > 0 )  // Special case analog output
            {
              x = atoi(&input_JSON[i+k]);
              analogWrite(LED_PWM, x); 
              Serial.print(x);
            }
            else if ( instr("vset_PWM", init_table[j].gpio_name ) > 0 )  // Special case analog output
            {
              x = atoi(&input_JSON[i+k]);
              analogWrite(vset_PWM, x); 
              Serial.print(x);
            }
            else
            {
              x = atoi(&input_JSON[i+k]);
              digitalWrite(init_table[j].port, x);
              Serial.print(x); 
              Serial.print(T(" Verify:")); Serial.print(digitalRead(init_table[j].port));
            }
          }
        }

     if ( JSON[j].token != 0 )
     {
       j++;
     }
     if ( init_table[l].gpio_name != 0xff )
     {
       l++;
     }
   }
  }

/*
 * Report an error if input not found
 */
  if ( not_found == true )
  {
    Serial.print(T("\r\n\r\nCannot decode: {")); Serial.print(input_JSON); Serial.print(T("}. Use")); 
    j = 0;    
    while ( JSON[j].token != 0 ) 
    {
      Serial.print(T("\r\n")); Serial.print(JSON[j].token);
      j++;
      if ( (j%4) == 0 ) 
      {
        Serial.print(T("\r\n"));
      }
    }
    Serial.print(T("\r\n\r\n  *** GPIO ***"));
    j=0;
    while ( init_table[j].port != 0xff ) 
    {
      if ( init_table[j].in_or_out == OUTPUT )
      {
        Serial.print(T("\r\n")); Serial.print(init_table[j].gpio_name);
        if ( (j%4) == 0 ) 
        {
          Serial.print(T("\r\n"));
        }
      }
      j++;
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

void show_echo(void)
{
  unsigned int i, j;
  char   s[512], str_c[32];   // String holding buffers
  
  sprintf(s, "\r\n{\r\n\"NAME\":\"%s\", \r\n", names[json_name_id]);
  output_to_all(s);

/*
 * Loop through all of the JSON tokens
 */
  i=0;
  while ( JSON[i].token != 0 )                 // Still more to go?  
  {
    if ( (JSON[i].value != NULL) || (JSON[i].d_value != NULL) )              // It has a value ?
    {
      switch ( JSON[i].convert )              // Display based on it's type
      {
        default:
        case IS_VOID:
          break;

        case IS_TEXT:
        case IS_SECRET:
            j = 0;
            while ( *((char*)(JSON[i].value)+j) != 0)
            {
              if ( JSON[i].convert == IS_SECRET )
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
            sprintf(s, "%s \"%s\", \r\n", JSON[i].token, str_c);
            break;
            
        case IS_INT16:
        case IS_FIXED:
          sprintf(s, "%s %d, \r\n", JSON[i].token, *JSON[i].value);
          break;

        case IS_FLOAT:
        case IS_DOUBLE:
          dtostrf(*JSON[i].d_value, 8, 6, str_c );
          sprintf(s, "%s %s, \r\n", JSON[i].token, str_c);
          break;
      }
      output_to_all(s);
      if ( esp01_connected() )            // If the wifi is attached
      {
        delay(100);                        // Slow down to let it catch up
      }
    }
    i++;
  }
  
/*
 * Finish up with the special cases
 */
  sprintf(s, "\n\r");                                                                    // Blank Line
  output_to_all(s);
  
  multifunction_display();
  
  sprintf(s, "\"TRACE\": %d, \n\r", is_trace);                                             // TRUE to if trace is enabled
  output_to_all(s);

  sprintf(s, "\"RUNNING_MINUTES\": %ld, \n\r", micros()/1000000/60);                      // On Time
  output_to_all(s);
  
  dtostrf(temperature_C(), 4, 2, str_c );
  sprintf(s, "\"TEMPERATURE\": %s, \n\r", str_c);                                         // Temperature in degrees C
  output_to_all(s);
  
  dtostrf(speed_of_sound(temperature_C(), json_rh), 4, 2, str_c );
  sprintf(s, "\"SPEED_SOUND\": %s, \n\r", str_c);                                         // Speed of Sound
  output_to_all(s);

  dtostrf(TO_VOLTS(analogRead(V_REFERENCE)), 4, 2, str_c );
  sprintf(s, "\"V_REF\": %s, \n\r", str_c);                                               // Trip point reference
  output_to_all(s);
  
  dtostrf(5.0d * (float)analogRead(V_12_LED) * K_12 / 1023.0d , 4, 2, str_c );            // Analog Reference * input * resistor divider / max MSB
  sprintf(s, "\"V_12_LED\": %s, \n\r", str_c);                                            // 12 Volt LED supply
  output_to_all(s);
  
  sprintf(s, "\"VSET_PWM\": %d, \n\r", json_vset_PWM);                                    // Setpoint adjust PWM
  output_to_all(s);
  
  sprintf(s, "\"TIMER_COUNT\": %d, \n\r", (int)(SHOT_TIME * OSCILLATOR_MHZ));             // Maximum number of clock cycles to record shot (target dependent)
  output_to_all(s);


  sprintf(s, "\"WiFi_PRESENT\": %d, \n\r", esp01_is_present());                           // TRUE if WiFi is available
  output_to_all(s);
  
  if ( esp01_is_present() )
  {
    esp01_myIP(str_c);
    sprintf(s, "\"WiFi_IP_ADDRESS\": \"%s:1090\", \n\r", str_c);                            // Print out the IP address
    output_to_all(s);
  
    for ( i=0; i != esp01_N_CONNECT; i++)
    {
      sprintf(s, "\"WiFi_CONNECT %d\": %d, \n\r", i+1, esp01_connect[i]);                   // TRUE if Client[i] connected
      output_to_all(s);
    }
  }
  
  sprintf(s, "\"VERSION\": %s, \n\r", SOFTWARE_VERSION);                                  // Current software version
  output_to_all(s);  

  EEPROM.get(NONVOL_PS_VERSION, j);
  sprintf(s, "\"PS_VERSION\": %d, \n\r", j);                                             // Current persistent storage version
  output_to_all(s); 
  
  dtostrf(revision()/100.0, 4, 2, str_c );              
  sprintf(s, "\"BD_REV\": %s \n\r", str_c);                                               // Current board versoin
  output_to_all(s);
  
  sprintf(s, "}\r\n"); 
  output_to_all(s);
  
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
  
  Serial.print(T("\r\nNames\r\n"));
  
  i=0;
  while (names[i] != 0 )
  {
    Serial.print(i); Serial.print(T(": \""));  Serial.print(names[i]); Serial.print(T("\", \r\n"));
    i++;
  }

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
  Serial.print(T("\r\nSelf Test:")); Serial.print(test_number); Serial.print(T("\r\n"));
  
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
   char s[32]; 

   switch (trace)
   {
    default: 
    case DLT_NONE:        sprintf(s, "\r\nDLT NONE\r\n");       break;
    case DLT_CRITICAL:    sprintf(s, "\r\nDLT CRITICAL\r\n");   break;
    case DLT_APPLICATION: sprintf(s, "\r\nDLT APPLICATON\r\n"); break;
    case DLT_DIAG:        sprintf(s, "\r\nDLT DIAG\r\n");       break;
    case DLT_INFO:        sprintf(s, "\r\nDLT INFO\r\n");       break;
   }
   
   output_to_all(s);
   
/*
 * The DIP switch has been remotely set
 */
   is_trace = trace;
   return;   
 }
