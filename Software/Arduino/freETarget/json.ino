/*-------------------------------------------------------
 * 
 * JSON.ino
 * 
 * JSON driver
 * 
 * ----------------------------------------------------*/

#include <EEPROM.h>
#include "json.h"
#include "nonvol.h"

static char input_JSON[128];

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

int     temp;                       // Temporary variable

#define IS_VOID    0
#define IS_INT16   1
#define IS_FLOAT   2
#define IS_DOUBLE  3
#define JSON_DEBUG false                    // TRUE to echo DEBUG messages

       void show_echo(int v);               // Display the current settings
static void show_test(int v);               // Execute the self test once
static void show_test0(int v);              // Help Menu
static void show_names(int v);
static void nop(void);
static void set_trace(int v);               // Set the trace on and off

typedef struct  {
  char*           token;    // JSON token string, ex "RADIUS": 
  int*            value;    // Where value is stored 
  double*       d_value;    // Where value is stored 
  unsigned int  convert;    // Conversion type
  void       (*f)(int x);   // Function to execute with message
  unsigned int  non_vol;    // Storage in NON-VOL
} json_message;

  
static json_message JSON[] = {
//    token                 value stored in RAM     double stored in RAM         type     service fcn()     NONVOL location
  {"\"ANGLE\":",          &json_sensor_angle,                0,                IS_INT16,  0,                NONVOL_SENSOR_ANGLE},    // Locate the sensor angles
  {"\"CAL\":",            0,                                 0,                IS_VOID,   &set_trip_point,                  0  },    // Enter calibration mode
  {"\"CALIBREx10\":",     &json_calibre_x10,                 0,                IS_INT16,  0,                NONVOL_CALIBRE_X10 },    // Enter the projectile calibre
  {"\"DIP\":",            &json_dip_switch,                  0,                IS_INT16,  0,                NONVOL_DIP_SWITCH  },    // Remotely set the DIP switch
  {"\"ECHO\":",           &json_echo,                        0,                IS_INT16,  &show_echo,                       0  },    // Echo test
  {"\"INIT\"",            0,                                 0,                IS_VOID,   &init_nonvol,                     0  },    // Initialize the NONVOL memory
  {"\"LED_BRIGHT\":",     &json_LED_PWM,                     0,                IS_INT16,  &set_LED_PWM,     NONVOL_LED_PWM     },    // Set the LED brightness
  {"\"NAME_ID\":",        &json_name_id,                     0,                IS_INT16,  &show_names,      NONVOL_NAME_ID     },    // Give the board a name
  {"\"PAPER\":",          &json_paper_time,                  0,                IS_INT16,  0,                NONVOL_PAPER_TIME  },    // Set the paper advance time
  {"\"POWER_SAVE\":",     &json_power_save,                  0,                IS_INT16,  0,                NONVOL_POWER_SAVE  },    // Set the power saver time
  {"\"SEND_MISS\":",      &json_send_miss,                   0,                IS_INT16,  0,                NONVOL_SEND_MISS   },    // Enable / Disable sending miss messages
  {"\"SENSOR\":",         0,                                 &json_sensor_dia, IS_FLOAT,  &gen_position,    NONVOL_SENSOR_DIA  },    // Generate the sensor postion array
  {"\"TEST\":",           &json_test,                        0,                IS_INT16,  &show_test,       NONVOL_TEST_MODE   },    // Execute a self test
  {"\"TRACE\":",          &temp,                             0,                IS_INT16,  &set_trace,                       0  },    // Enter / exit diagnostic trace
  {"\"TRGT_1_RINGx10\":", &json_1_ring_x10,                  0,                IS_INT16,  0,                NONVOL_1_RINGx10   },    // Enter the 1 ring diamater
  {"\"NORTH_X\":",        &json_north_x,                     0,                IS_INT16,  0,                NONVOL_NORTH_X     },    //
  {"\"NORTH_Y\":",        &json_north_y,                     0,                IS_INT16,  0,                NONVOL_NORTH_Y     },    //
  {"\"EAST_X\":",         &json_east_x,                      0,                IS_INT16,  0,                NONVOL_EAST_X      },    //
  {"\"EAST_Y\":",         &json_east_y,                      0,                IS_INT16,  0,                NONVOL_EAST_Y      },    //
  {"\"SOUTH_X\":",        &json_south_x,                     0,                IS_INT16,  0,                NONVOL_SOUTH_X     },    //
  {"\"SOUTH_Y\":",        &json_south_y,                     0,                IS_INT16,  0,                NONVOL_SOUTH_Y     },    //
  {"\"WEST_X\":",         &json_west_x,                      0,                IS_INT16,  0,                NONVOL_WEST_X      },    //
  {"\"WEST_Y\":",         &json_west_y,                      0,                IS_INT16,  0,                NONVOL_WEST_Y      },    //

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
  while (Serial.available() != 0)
  {
    return_value = true;
    
    ch = Serial.read();
#if ( JSON_DEBUG == true )
    Serial.print(ch);
#endif

/*
 * Parse the stream
 */
    switch (ch)
    {
      case ' ':                             // Ignore spaces
        break;

      case '%':
        self_test(T_XFR_LOOP);          // Transfer self test
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
            x = 0;
            y = 0;
          break;
            
          case IS_INT16:                                      // Convert an integer
            x = atoi(&input_JSON[i+k-1]);
            *JSON[j].value = x;                               // Save the value
            if ( JSON[j].non_vol != 0 )
            {
              EEPROM.put(JSON[j].non_vol, x);                 // Store into NON-VOL
            }
            break;
  
          case IS_FLOAT:                                      // Convert a floating point number
          case IS_DOUBLE:
            y = atoi(&input_JSON[i+k-1]);
            *JSON[j].d_value = y;                             // Save the value
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
    Serial.print("\r\n{\"JSON\":0, "); 
    j = 0;    
    while ( JSON[j].token != 0 ) 
    {
      Serial.print(JSON[j].token); Serial.print("0, ");
      j++;
    }
    Serial.print(" \"VERSION\": "); Serial.print(SOFTWARE_VERSION); Serial.print("}\r\n"); 
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
  if ( *s2 == 0 )
  {
    return i;
  }
  return -1;
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
  unsigned int i;
  
  Serial.print("\r\n{\r\n");
  Serial.print("\"NAME\":\""), Serial.print(names[json_name_id]); Serial.print("\", \r\n");
  
  i=0;
  while (JSON[i].token != 0 )
  {
    switch ( JSON[i].convert )
    {
      default:
      case IS_VOID:
        break;
          
      case IS_INT16:
        Serial.print(JSON[i].token);
        Serial.print(*JSON[i].value); Serial.print(", \r\n");
        break;

      case IS_FLOAT:
      case IS_DOUBLE:
        Serial.print(JSON[i].token);
        Serial.print(*JSON[i].d_value); Serial.print(", \r\n");
        break;
    }
    i++;
  }

  EEPROM.get(NONVOL_INIT, i);
  Serial.print("\r\n");
  Serial.print("\"INIT\":");        Serial.print(i);                Serial.print(", \r\n");
  Serial.print("\"IS_TRACE\":");    Serial.print(is_trace);         Serial.print(", \r\n");
  Serial.print("\"TEMPERATURE\":"); Serial.print(temperature_C());  Serial.print(", \r\n");
  Serial.print("\"V_REF\":");       Serial.print(TO_VOLTS(analogRead(V_REFERENCE))); Serial.print(", \r\n");
  Serial.print("\"VERSION\":");     Serial.print(SOFTWARE_VERSION); Serial.print(", \r\n");
  Serial.print("\"BRD_REV\":");     Serial.print(revision()); 
  Serial.print("\r\n}\r\n");
  
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
 * Loop and display the settings
 * 
 *-----------------------------------------------------*/

static void show_names(int v)
{
  unsigned int i;
  
  Serial.print("\r\n{\r\n");
  
  i=1;
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
 * return: None
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
   Serial.print("\n\rTrace: ");
   
   if ( trace == 0 )
   {
      json_dip_switch &= ~VERBOSE_TRACE;
      Serial.print("OFF");
   }
   else
   {
      json_dip_switch |= VERBOSE_TRACE;
      Serial.print("ON");
   }

   Serial.print("\n\r");
   
  /*
   * The DIP switch has been remotely set
   */
    return;   
 }

