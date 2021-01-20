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
int     json_trip_point;            // Detection trip point in mV
int     json_name_id;               // Name identifier
int     json_1_ring_x10;            // Size of the 1 ring in mm

#define IS_VOID    0
#define IS_INT16   1
#define IS_FLOAT   2
#define IS_DOUBLE  3
#define JSON_DEBUG false                    // TRUE to echo DEBUG messages

       void show_echo(void);                // Display the current settings
static void show_test(void);                // Execute the self test once
static void show_test0(void);               // Help Menu
static void show_names(void);               // Display the names

static void nop(void);

typedef struct  {
  char*           token;  // JSON token string, ex "RADIUS": 
  int*            value;  // Where value is stored 
  double*       d_value;  // Where value is stored 
  unsigned int  convert;  // Conversion type
  void       (*f)(void);  // Function to execute with message
  unsigned int  non_vol;  // Storage in NON-VOL
} json_message;

  
static json_message JSON[] = {
  {"\"ANGLE\":",          &json_sensor_angle,                0,                IS_INT16,  0,             NONVOL_SENSOR_ANGLE},    //  0
  {"\"CALIBREx10\":",     &json_calibre_x10,                 0,                IS_INT16,  0,             NONVOL_CALIBRE_X10 },    //  1
  {"\"DIP\":",            &json_dip_switch,                  0,                IS_INT16,  0,             NONVOL_DIP_SWITCH  },    //  2
  {"\"ECHO\":",           &json_echo,                        0,                IS_INT16,  &show_echo,                    0  },    //  3
  {"\"INIT\":",           0,                                 0,                IS_VOID,   &reinit_nonvol,                0  },    //  4  
  {"\"PAPER\":",          &json_paper_time,                  0,                IS_INT16,  0,             NONVOL_PAPER_TIME  },    //  5
  {"\"SENSOR\":",         0,                                 &json_sensor_dia, IS_FLOAT,  &gen_position, NONVOL_SENSOR_DIA  },    //  6
  {"\"TEST\":",           &json_test,                        0,                IS_INT16,  &show_test,    NONVOL_TEST_MODE   },    //  7
  {"\"NORTH_X\":",        &json_north_x,                     0,                IS_INT16,  0,             NONVOL_NORTH_X     },    //  8
  {"\"NORTH_Y\":",        &json_north_y,                     0,                IS_INT16,  0,             NONVOL_NORTH_Y     },    //  9
  {"\"EAST_X\":",         &json_east_x,                      0,                IS_INT16,  0,             NONVOL_EAST_X      },    // 10
  {"\"EAST_Y\":",         &json_east_y,                      0,                IS_INT16,  0,             NONVOL_EAST_Y      },    // 11
  {"\"SOUTH_X\":",        &json_south_x,                     0,                IS_INT16,  0,             NONVOL_SOUTH_X     },    // 12
  {"\"SOUTH_Y\":",        &json_south_y,                     0,                IS_INT16,  0,             NONVOL_SOUTH_Y     },    // 13
  {"\"WEST_X\":",         &json_west_x,                      0,                IS_INT16,  0,             NONVOL_WEST_X      },    // 14
  {"\"WEST_Y\":",         &json_west_y,                      0,                IS_INT16,  0,             NONVOL_WEST_Y      },    // 15
  {"\"TRIP_POINT\":",     &json_trip_point,                  0,                IS_INT16,  &set_trip_pt,  NONVOL_TRIP_POINT  },    // 16
  {"\"NAME_ID\":",        &json_name_id,                     0,                IS_INT16,  &show_names,   NONVOL_NAME_ID     },    // 17
  {"\"TRGT_1_RINGx10\":", &json_1_ring_x10,                  0,                IS_INT16,  0,             NONVOL_1_RINGx10   },    // 18
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

void read_JSON(void)
{
static int16_t got_right;
unsigned int  i, j, x;
int     k;
char    ch;
double  y;

/*
 * See if anything is waiting and if so, add it in
 */
  while (Serial.available() != 0)
  {
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
    return;
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

      if ( k > 0 )
      {
        not_found = false;
        k++;
        switch ( JSON[j].convert )
        {
          default:
          case IS_VOID:
          break;
            
          case IS_INT16:
            x = atoi(&input_JSON[i+k-1]);
            *JSON[j].value = x;                              // Save the value
            if ( JSON[j].non_vol != 0 )
            {
              EEPROM.put(JSON[j].non_vol, x);               // Store into NON-VOL
            }
            break;
  
          case IS_FLOAT:
          case IS_DOUBLE:
            y = atoi(&input_JSON[i+k-1]);
            *JSON[j].d_value = y;                           // Save the value
            if ( JSON[j].non_vol != 0 )
            {
              EEPROM.put(JSON[j].non_vol, y);               // Store into NON-VOL
            }
            break;
        }
        if ( JSON[j].f != 0 )                              // Call the handler if it is available
        {
          JSON[j].f();
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
  return;
}

// Compare two strings.  Return -1 if not equal, end of string if equal
// S1 Long String, S2 Short String . if ( instr("CAT Sam", "CAT") == 3)
int instr(char* s1, char* s2)
{
  int return_value = -1;
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
 * Reached the end of the comparison string
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

void show_echo(void)
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
  Serial.print("\"INIT\":");     Serial.print(i);                Serial.print(", \r\n");
  Serial.print("\"V_REF\":");    Serial.print(TO_VOLTS(analogRead(V_REFERENCE))); Serial.print(", \r\n");
  Serial.print("\"VERSION\":");  Serial.print(SOFTWARE_VERSION); Serial.print(", \r\n");
  Serial.print("\"BRD_REV\":");  Serial.print(revision()); 
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

static void show_names(void)
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

static void show_test(void)
 {
  Serial.print("\r\nSelf Test:"); Serial.print(json_test); Serial.print("\r\n");
  
  self_test(json_test);
  return;
 }
