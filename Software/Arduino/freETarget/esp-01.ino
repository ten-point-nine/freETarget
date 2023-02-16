/*----------------------------------------------------------------
 *
 * esp-01.ino
 *
 * WiFi Driver for ESP-01
 * 
 *-----------------------------------------------------------------
 *
 * Refer to the command set which can be found at 
 * 
 * https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf
 * 
 * The WiFi software somewhat convoluted.
 * 
 * In order to simplify operation in the field, each freETarget 
 * is a WiFi station.  The target is assigned an SSID corresponding 
 * to the name of the target, ex FET-WODEN.  This helps to link up 
 * the WiFI to the target.
 * 
 * Each ESP-01 is a server so that the PC connects to it over 
 * IP address 192.168.10.9:1090
 * 
 * Unfortunatly as a server, each ESP expects to see more than one
 * connection, so a transparent or passthrought mode is not possible.
 * Instead the incoming messages are tagged with a connection number
 * and outgoign messages are prepended with a connection number.
 * 
 * Thus the function esp01_receive constantly listens on the serial
 * port, parses the input and saves only the message to a 
 * circular buffer that is used later on by the application.
 * 
 * On output, the function esp01_send tells the ESP-01 to expect a 
 * very long null-terminated message.  The regular Serial.send()
 * messages are then used to send the body of the message and at 
 * the end esp01_send() inserts the null and kicks off the 
 * transmission.
 * 
 *---------------------------------------------------------------*/

/*
 * Function Prototypes
 */
static bool esp01_esp01_waitOK(unsigned int trace_on,unsigned int max_wait);   // Wait for an OK to come back
static void esp01_flush(void);          // Flush any pending messages

/*
 * Local Variables
 */
static bool esp01_present = false;      // TRUE if the esp 01 is installed

static char esp01_in_queue[32];         // Place to store incoming characters
static int  esp01_in_ptr;               // Queue in pointer
static int  esp01_out_ptr;              // Queue out pointer

static bool esp01_connect[esp01_N_CONNECT]; // Set to true when a client (0-3) connects
#define esp01_MAX_WAITOK  (ONE_SECOND * 2)  // 2000 milliseconds
/*----------------------------------------------------------------
 *
 * function: void esp01_init
 *
 * brief: Initialize the ESP-01
 * 
 * return: esp_present - set to TRUE if an ESP-01 is on the board
 *
 *----------------------------------------------------------------
 *   
 * The function outputs a series of AT commands to the port to 
 * 
 * Set the part as a WiFi Server
 *    The SSID is set up as FET-<name>
 *    The freETarget IP address is 192.169.10.9
 *    The freETarget port is 1090
 *    The connecting PC will be assigned address 192.168.10.8
 *    
 * Open a port for connection
 * 
 * IMPORTANT
 * 
 * The init function does not set the pass through mode.  This
 * is done by a separate call to esp01_passthrough();
 * 
 * Network Settings
 * 
 * json_wifi_ssid given:
 *    Connect to this SSID
 *    use it's DHCP server
 *    Use json_wifi_ip if given
 *    
 * json_wifi_ssid empty:
 *    I am the SSID
 *    I am the DHCP server
 *    Use 192.168.10.9 as my IP address
 *   
 *--------------------------------------------------------------*/
void esp01_init(void)
{  
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("Initializing ESP-01 - Waiting for Power Up"));
  }
  
  esp01_reset();
  esp01_flush();
  
/*
 * Determine if the ESP-01 is attached to the Accessory Connector
 */
  if (esp01_is_present() == false )
  {
    if ( DLT(INIT_TRACE) ) 
    {
      Serial.print(T("ESP-01: Not Found"));
    }
    return;                                     // No hardware installed, nothing to do
  }
  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("ESP-01 Present"));
  }

/*
 * There is an ESP-01 on the freETarget.  We need to program it
 */
  WIFI_SERIAL.print(T("ATE0\r\n"));                   // Turn off echo (don't use it)
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed ATE0"));
  }

  WIFI_SERIAL.print(T("AT+RFPOWER=80\r\n"));          // Set max power
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE))
  {
    Serial.print(T("ESP-01: Failed AT+RFPOWER=80"));  
  } 

  esp01_flush();
/*
 * If an SSID is defined, then we are connecting to an existing access point
 */
  if ( *json_wifi_ssid == 0 )                           // If the SSID is empty, then we are an SSID and a DHCP server and our IP address is 192.168.10.9
  {                                                     // ******************************
    if ( DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Configuring as an SSID"));
    }
      
    WIFI_SERIAL.print(T("AT+CWMODE_DEF=2\r\n"));        // We want to be a soft access point
    if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Failed AT+CWMODE_DEF=2"));
    }
    
    WIFI_SERIAL.print(T("AT+CWSAP_DEF=\"FET-")); WIFI_SERIAL.print(names[json_name_id]); WIFI_SERIAL.print(T("\",\"NA\",")); WIFI_SERIAL.print(json_wifi_channel); WIFI_SERIAL.print(T(",0\r\n"));
    if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Failed AT+CWSAP_DEF=\"FET-")); Serial.print(names[json_name_id]); Serial.print(T("\",\"NA\",")); Serial.print(json_wifi_channel); Serial.print(T(",0\r\n"));
    }  

    WIFI_SERIAL.print(T("AT+CWDHCP_DEF=0,1\r\n"));      // DHCP turned on
    if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false ) && DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Failed AT+CWDHCP_DEF=0,1"));
    }
  
    WIFI_SERIAL.print(T("AT+CIPAP_DEF=\"192.168.10.9\",\"192.168.10.9\"\r\n")); // Set the freETarget IP to 192.168.10.9
    if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
    {
      Serial.print(T("\r\nESP-01: Failed AT+CIPAP_DEF=\"192.168.10.9\",\"192.168.10.9\""));
    }
  }                                                     // ******************************
  else                                                  // Connect to an SSID, let it define the DHCP if needed
  {
    if ( DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Connecting to an access point: ")); Serial.print(json_wifi_ssid); Serial.print(T(",")); Serial.print(json_wifi_pwd);
    }
    
    WIFI_SERIAL.print(T("AT+CWQAP\r\n"));        // Disconnect
    if ( (esp01_waitOK(1,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Failed AT+CWQAP"));
    } 
    
    WIFI_SERIAL.print(T("AT+CWMODE_DEF=1\r\n"));        // We want to be an in Station Mode
    if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
    {
      Serial.print(T("ESP-01: Failed AT+CWMODE_DEF=1"));
    }
    
    WIFI_SERIAL.print(T("AT+CWJAP_DEF=\"")); WIFI_SERIAL.print(json_wifi_ssid); WIFI_SERIAL.print(T("\",\"")); WIFI_SERIAL.print(json_wifi_pwd); WIFI_SERIAL.print(T("\""));
    if ( (esp01_waitOK(1, esp01_MAX_WAITOK*10) == false) )               // Didn't connect to the SSID Try again
    { 
      if ( DLT(INIT_TRACE))
      {
        Serial.print(T("ESP-01: Failed AT+CWJAP_DEF=\"")); Serial.print(json_wifi_ssid); Serial.print(T("\",\"")); Serial.print(json_wifi_pwd); Serial.print(T("\""));
      }
    }
  }

/*
 * Other operating settings
 */
/*
  WIFI_SERIAL.print(T("AT+CIPMODE=0\r\n"));           // Normal Transmission Mode
  if ( (esp01_waitOK(1,esp01_MAX_WAITOK) == false ) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed AT+CIPMODE=0"));
  }
*/  
  WIFI_SERIAL.print(T("AT+CIPMUX=1\r\n"));           // Allow multiple connections
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK * 10) == false ) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed AT+CIPMUX=1"));
  }

  WIFI_SERIAL.print(T("AT+CIPSERVER=1,1090\r\n"));   // Turn on the server and listen on port 1090
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false ) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed AT+CIPSERVER=1,1090"));
  }
  
  WIFI_SERIAL.print(T("AT+CIPSTO=7000\r\n"));        // Set the server time out
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false ) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed AT+CIPSTO=7000"));
  }
  
/*
 * All done, return
 */
  if ( DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Initialization complete"));
  }

  return;
}


/*----------------------------------------------------------------
 *
 * function: void esp01_reset
 *
 * brief:    Take control of the ESP and issue a reset
 * 
 * return:   TRUE if the module acknowleges the command and restarts
 *
 *----------------------------------------------------------------
 *   
 * We don't know what state the ESP is in, so this function
 * isses +++AT to get contol of the device and reset it to 
 * factory defaults. 
 *   
 *--------------------------------------------------------------*/
static bool esp01_reset(void)
{
  unsigned int i;

  if ( DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Resetting ESP-01"));
  }

/*
 * Send out the break then restart the module
 */
  WIFI_SERIAL.print(T("+++"));
  delay(ONE_SECOND);
  WIFI_SERIAL.print(T("AT+RST\r\n"));
  if ( (esp01_waitOK(0,esp01_MAX_WAITOK) == false) && DLT(INIT_TRACE) )
  {
    Serial.print(T("ESP-01: Failed to reset ESP-01"));
    return false;
  }

/*
 * Flush the stuff from the self test
 */
  delay(ONE_SECOND);
  esp01_flush();
  
/*
 * All done, 
 */
  for (i=0; i != esp01_N_CONNECT; i++)
  {
    esp01_connect[i] = false;
  }
  
  return true;
}


/*----------------------------------------------------------------
 *
 * function: bool esp01_is_present
 *
 * brief:    Determines if an ESP-01 is installed
 * 
 * return:   TRUE if the ESO-01 is installed
 *
 *----------------------------------------------------------------
 *   
 * The function outputs an AT<entere> to the auxilarry port and
 * waits to see if an OK is returned within one second.
 * 
 * This function can only be executed once after a reset.  After 
 * that time, the device may be in pass through mode and will not
 * do anything with the AT command
 *   
 *--------------------------------------------------------------*/
bool esp01_is_present(void)
{
  static bool esp01_first = true;    // Remember if we have results
  
/*
 * If the function has been exectued once, return the old results 
 */
  if ( esp01_first == false )
  {
    return esp01_present;
  }

/*
 * Determine if the ESP-01 is attached to the Accessory Connector
 */
  esp01_present = false;                  // Assume that the ESP-01 is not installed
  
  esp01_flush();                          // Eat any garbage that might be on the port

  WIFI_SERIAL.print(T("+++"));
  delay(ONE_SECOND);
  WIFI_SERIAL.print(T("AT\r\n"));            // Send out an AT command to the port
  esp01_present = esp01_waitOK(0,esp01_MAX_WAITOK);         // and wait for the OK to come back

/*
 * All done, return the esp-01 present state
 */
  esp01_first = false;                   // Done it once.
  return esp01_present;
}


/*----------------------------------------------------------------
 *
 * function: bool esp01_test
 *
 * brief:    Tests to see if the WiFi responds to ATs
 * 
 * return:   Displayed on screen
 *
 *----------------------------------------------------------------
 *   
 * The function outputs an AT<entere> to the auxilarry port and
 * waits to see if an OK is returned within one second.
 * 
 * This function can only be executed once after a reset.  After 
 * that time, the device may be in pass through mode and will not
 * do anything with the AT command
 *   
 *--------------------------------------------------------------*/
void esp01_echo(void)
{
   char ch;                                // Character read from ESP-01
  long unsigned int start;                // Start time

  start = millis();
  while ( (millis() - start) < esp01_MAX_WAITOK )
  {
    if ( WIFI_SERIAL.available() != 0 ) 
    {
      ch = WIFI_SERIAL.read();
      Serial.print(ch);
      start = millis();
    }
  }
  return;
}

void esp01_test(void)
{

  esp01_flush();                          // Eat any garbage that might be on the port

  if ( esp01_is_present() == false )
  {
    Serial.print(T("\n\rESP-01 not present. Continuing test anyway")); 
  }

/*
 * Send out status commands
 */
 
  Serial.print(T("\n\rResetting ESP-01\n\r"));
  WIFI_SERIAL.print(T("+++"));
  delay(ONE_SECOND);
  WIFI_SERIAL.print(T("AT+RST\r\n"));
  esp01_echo();
    
  Serial.print(T("\r\nSoftware Version\r\n"));
  WIFI_SERIAL.print(T("AT+GMR\r\n"));         // Send out an AT command to the port
  esp01_echo();

  Serial.print(T("\r\nList of SSIDs\r\n"));
  WIFI_SERIAL.print(T("AT+CWLAP\r\n"));       // Send out an AT command to the port
  esp01_echo();
    
/*
 * Initialize the WiFi if available
 */
   set_LED('*', '*', '.');                 // Hello World
   Serial.print(T("\r\nReinitializing ESP-01\r\n")); 
   esp01_init();                           // Prepare the WiFi channel if installed
   
/*
 * All done, return the esp-01 present state
 */
  Serial.print(T("\r\nTest Over"));
  return;
}


/*----------------------------------------------------------------
 *
 * function: bool esp01_status
 *
 * brief:    Request and display stauts
 * 
 * return:   Displayed on screen
 *
 *----------------------------------------------------------------
 *   
 * The function issues a status command and sees what comes back
 * then it issues a PING on the four IP addressees that should
 * be attached to is.
 *--------------------------------------------------------------*/

static char* status_list[] = {"CWMODE_DEF",  "CWSAP_DEF", "CWJAP_DEF",
                              "CWDHCP_DEF",  "CIPAP_DEF", "CIPSTA_CUR",
                              "CWDHCPS_DEF", "CIPMODE",    "CIPSERVER",
                              "CIPMUX",      "CIPSTO", 0};        // Status List

void esp01_status(void)
{
  char ch;                                // Character read from ESP-01
  long unsigned int start;                // Start time
  unsigned int i;
  char str[32];
  
/*
 * Get the status of the connection
 */
  Serial.print(T("\n\resp01_status\r\n"));

  WIFI_SERIAL.print(T("+++"));
  delay(ONE_SECOND + ONE_SECOND/10);
  WIFI_SERIAL.print(T("AT+CIPSTATUS\r\n"));

  start = millis();
  while ( (millis() - start) < esp01_MAX_WAITOK )
  {
    if ( WIFI_SERIAL.available() != 0 ) 
    {
      ch = WIFI_SERIAL.read();                  // Echo what comes back
      Serial.print(ch);
      start = millis();
    }
  }

/*
 * All of the settings
 */
  i=0;
  while (status_list[i] != 0)
  {
    Serial.print(T("\r\nStatus: ")); Serial.print(status_list[i]); Serial.print(T("\r\n"));
    WIFI_SERIAL.print(T("AT+")); WIFI_SERIAL.print(status_list[i]); WIFI_SERIAL.print(T("?\r\n"));
    start = millis();
    while ( (millis() - start) < esp01_MAX_WAITOK )
    {
      if ( WIFI_SERIAL.available() != 0 ) 
      {
        ch = WIFI_SERIAL.read();                  // Echo what comes back
        Serial.print(ch);
        start = millis();
      }
    }
    i++;
  }
  
/*
 * Ping and see what comes back
 */
  Serial.print(T("\n\resp01_ping\r\n"));

  for (i=0; i != esp01_N_CONNECT; i++)
  {
    WIFI_SERIAL.print(T("+++"));
    delay(ONE_SECOND + ONE_SECOND/10);
    sprintf(str, "AT+PING=\"192.168.10.%d\"\r\n", i);
    WIFI_SERIAL.print(str);
    Serial.print(str);
    start = millis();
    while ( (millis() - start) < esp01_MAX_WAITOK )
    {
      if ( WIFI_SERIAL.available() != 0 ) 
      {
        ch = WIFI_SERIAL.read();
        Serial.print(ch);
        start = millis();
      }
    }
  }
  
/*
 * All done, return the esp-01 present state
 */
  Serial.print(T("\r\nDone\r\n"));
  return;
}


/*----------------------------------------------------------------
 *
 * function: bool esp01_myIP
 *
 * brief:    Obtain the current IP address
 * 
 * return:   Returned to a pointer
 *
 *----------------------------------------------------------------
 *   
 * The function issues a status command and sees what comes back
 * then it issues a PING on the four IP addressees that should
 * be attached to is.
 *--------------------------------------------------------------*/
#define myIP_IDLE   0
#define myIP_QUOTE1 1
#define myIP_QUOTE2 2

char ip[] = "AAA.BBB.CCC.DDD";

void esp01_myIP
  (
  char* return_value          // Place to return the results
  )
{
  unsigned long start;
  char ch;
  unsigned int myIP_state;

  *return_value = 0;
  
  if ( *json_wifi_ssid == 0 )
  {
    WIFI_SERIAL.print(T("AT+CIPAP_CUR?\r\n"));        // Request the IP address
  }
  else
  {
    WIFI_SERIAL.print(T("AT+CIPSTA_CUR?\r\n"));       // Request the IP address
  }

  myIP_state = myIP_IDLE;
  
  start = millis();
  while ( (millis() - start) < esp01_MAX_WAITOK )
  {
    if ( WIFI_SERIAL.available() != 0 ) 
    {
      ch = WIFI_SERIAL.read();                      // Echo what comes back

      switch (myIP_state)
      {
        case myIP_IDLE: 
          if ( ch == '"' )
          {
            myIP_state = myIP_QUOTE1;
          }
          start = millis();
          break;
          
      case myIP_QUOTE1:                             // Accumulate the IP addres
          if ( ch == '"' )
          {
            return;
          }
          *return_value = ch;
          return_value++;
          *return_value = 0;
          start = millis();
          break;

     case myIP_QUOTE2: 
          break;
      }
    }
  }
/*
 * All done, return
 */
  return;
}

/*----------------------------------------------------------------
 *
 * function: void esp01_flush
 *
 * brief:    Flush any characters waiting in the buffer
 * 
 * return:   None
 *
 *----------------------------------------------------------------
 *   
 *   Just read the port and return when nothing is left 
 *   
 *--------------------------------------------------------------*/

 static void esp01_flush(void)
 {
   while ( WIFI_SERIAL.available() != 0 ) 
   {
    WIFI_SERIAL.read();
   }

 /*
  * The buffer is empty, go home
  */
   return;
 }


/*----------------------------------------------------------------
 *
 * function: bool esp01_waitOK(0,esp01_MAX_WAITOK)
 *
 * brief:    Wait for the OK to come back
 * 
 * return:   FALSE if no OK came back
 *
 *----------------------------------------------------------------
 *   
 *   The function has a list of expected messsages and responses
 *   
 *   As each character is received, it is compared to the list and
 *   when it reaches the end of the list the value is returned
 *   
 *--------------------------------------------------------------*/

typedef struct
{
  unsigned int index;           // Where are we in the comparison
  char*        ptr;             // Pointer to the message
  bool         return_value;    // What to return if a match is made
} wait_message_t;

static wait_message_t wait_message[] = {
  { 0, "OK",               true },      // Command executed successfully
  { 0, "ERROR",           false },      // Command failed
  { 0, "WIFI GOT IP",      true },      // Connected to SSID and received IP
  { 0, "WIFI DISCONNECT", false },      // No connection to SSID
  { 0, "ready",            true },      // Finished reset and ready to go
  { 0, 0, 0}
  };
  
static bool esp01_waitOK
  (
  unsigned int trace_on,                // Force the trace to be on for diagnostics
  unsigned int wait_time                // How long to wait before failing
  )
{
  char         ch;                      // Character from port
  unsigned int i;
  long         start;                   // Timer start
  
  start = millis();                     // Remember the starting time
  
  i = 0;                                // Clear the indexes
  while (wait_message[i].ptr != 0 )
  {
    wait_message[i].index = 0;
    i++;
  }

  if ( DLT(DLT_DIAG) )
  {
    Serial.print(T("esp01_waitOK(x,")); Serial.print(wait_time); Serial.print(T(")"));
  }
  
/*
 * Loop for a second and see if OK comes back
 */
  while ( (millis() - start) < wait_time )
  {
    if ( WIFI_SERIAL.available() != 0 ) 
    {
      start = millis();                   // Reset the timeout
      ch = WIFI_SERIAL.read();
      if ( (trace_on != 0) || (is_trace >= DLT_DIAG) )
      {
        Serial.print(ch);
      }

      i = 0;                                // Look through the list of messages
      while (wait_message[i].ptr != 0 )
      {
        if ( wait_message[i].ptr[wait_message[i].index] != ch ) // No match
        {
          wait_message[i].index = 0;         // Start over
        }
        else                                // Match
        {
          wait_message[i].index++;          // Look for the next entry
          if ( wait_message[i].ptr[wait_message[i].index] == 0 ) // Reached the end of the comparison
          {
            return wait_message[i].return_value;
          }
        }
        i++;
      }
    }
  }

/*
 * Time ran out without getting an OK.
 */
  return false;
}


/*----------------------------------------------------------------
 *
 * function: char esp01_read()
 *
 * brief:    Return a character from the Accessory port
 * 
 * return:   Next character waiting
 *
 *----------------------------------------------------------------
 *   
 *   The function checks to see if an ESP-01 is attached to the
 *   board.  
 *   
 *   Not attached - Perform regular WIFI_SERIAL.read()
 *   Attached     - Take a character from the IP queue
 *   
 *--------------------------------------------------------------*/

char esp01_read(void)
{
  char ch;                              // Working character
  
/*
 * Determine which source will be used to get the next character
 */
  if ( esp01_is_present() == false )
  {
    return WIFI_SERIAL.read();           // Just do a regular read from the aux port
  }

/*  
 *   We have an ESP-01 attached,  Get the character from the queue
 */
  if ( esp01_in_ptr == esp01_out_ptr )  // Is the queue empty?
  {
    ch = -1;                            // Return an error
  }
  else
  {
    ch = esp01_in_queue[esp01_out_ptr]; // Get the character out of the queueu
    esp01_out_ptr = (esp01_out_ptr + 1) % sizeof(esp01_in_queue);// increment and wrap around
  }

/*
 * All done, return the next character
 */
  return ch;
}

/*----------------------------------------------------------------
 *
 * function: unsigned int esp01_available()
 *
 * brief:    Determine ihow many characters are available
 * 
 * return:   Number of characters in the input queue
 *
 *----------------------------------------------------------------
 *   
 *   The function checks to see if an ESP-01 is attached to the
 *   board.  
 *   
 *   Not attached - Perform regular WIFI_SERIAL.gavailable()
 *   Attached     - Return the queue size
 *   
 *--------------------------------------------------------------*/

unsigned int esp01_available(void)
{
  unsigned int x;                       // Working character
  
/*
 * Determine which source will be used to get return the input size
 */
  if ( esp01_is_present() == false )
  {
    return WIFI_SERIAL.available();      // Just do a regular read fromthe aux port
  }

/*  
 *   We have an ESP-01 attached,  Figure out the bytes in the queueu
 */
  if ( esp01_in_ptr < esp01_out_ptr )   // Have we wrapped around?
  {
    x = sizeof(esp01_in_queue) + esp01_in_ptr;  // add in the size offset
  }
  else
  {
    x = esp01_in_ptr;
  }

  x -= esp01_out_ptr;                   // Subtract the output pointer


/*
 * All done, return
 */
  return x;
}


/*----------------------------------------------------------------
 *
 * function: char esp01_send()
 *
 * brief:    Send a string over the IP port
 * 
 * return:   TRUE if the connection is active
 *
 *----------------------------------------------------------------
 *   
 *   The function checks to see if an ESP-01 is attached to the
 *   board.  
 *   
 *   Not attached - Do nothing
 *   Attached     - Output the control bytes to begin IP transmission
 *   
 *   The function operates on a bit of a kluge.  The ESP-01 has a 
 *   command AT+CIPSENDEX which will transmit the next N bytes 
 *   over the IP connection. The ESP-01 will also stop collecting 
 *   bytes and send the data if a NULL is received.
 *   
 *   The function uses a state machine
 *   
 *   Not ready to send out characters
 *   Filling the transmit buffer
 *   Kicking out the transmit buffer.
 *   
 *   This lets the application fill the tranmsit buffer as 
 *   quickly as possible until the buffer is full.
 *   
 *   The appplication must send a null to trigger the sending
 *   operation.
 *--------------------------------------------------------------*/
#define MAX_RETRY   10                  // Number of retry attempts to start

#define SEND_OFF    0                   // The send function is off
#define SEND_BUSY   1                   // The buffer is being sent.
#define SEND_WAIT   2                   // Wait for a long delay to expire
#define SEND_ERROR  3                   // Recover from an error

static unsigned int send_state[esp01_N_CONNECT]; // State of each connection

bool esp01_send_ch                      // Send a character
  (
    char ch,                            // Character to send
    int  connection                     // Which connection to send on
  )
{
  char str[2];

  str[0] = ch;
  str[1] = 0;
  esp01_send(str, connection);
}

bool esp01_send
  (
    char* str,                          // String to send
    int  connection                     // Which connection to send on
  )
{
  long         timer;                   // Timer start
  unsigned int retry_count;             // Number of times to try again
  char AT_command[64];                  // Place to store a string
  char ch;                              // Character read back
  unsigned int len;                     // Length of string to send
  
/*
 * Determine if we actually have to do anything
 */
  if ( esp01_connect[connection] == false )  // No connection on this connection
  {
    return false;
  }

  len = 0;                                    // Figure out the size of the string
  while (*(str+len) )
  {
    len++;
  }
  if ( len > esp01_BUFFER_SIZE )
  {
    len = esp01_BUFFER_SIZE;
  }

  while ( WIFI_SERIAL.available() != 0 )      // Eat any availble characters
  {
    ch = WIFI_SERIAL.read();
    delay(2);
  }
  
/*  
 *   Loop here until the string has been output
 */
  retry_count = MAX_RETRY;
  while ( 1 )
  {
    switch ( send_state[connection] )
    {
      case SEND_OFF:                                      // The ESP is not ready to send, 
        sprintf(AT_command, "AT+CIPSEND=%d,%d\r\n", connection, len); // Start and lie that we will send 2K of data
        WIFI_SERIAL.print(AT_command);
        timer = millis();                               // Remember the starting time
        while ( (millis() - timer) < esp01_MAX_WAITOK ) // Wait for the > to come back within a second
        {
          if ( WIFI_SERIAL.available() != 0 )           // Something available?
          {
            ch = WIFI_SERIAL.read();
            if ( ch == '>' )                            // Is it the prompt?
            {
              set_LED(LED_WIFI_SEND);
              WIFI_SERIAL.print(str);                   // Yes, ready to send
              send_state[connection] = SEND_BUSY;       // Wait for the buffer to be sent
              break;                                    // Break from timer while
            }
          }
        }
        if ( send_state[connection] == SEND_BUSY)
        {
          break;                                        // All good, break from SEND_OFF state
        }
        retry_count--;
        if ( retry_count == 0 )
        {
          send_state[connection] = SEND_ERROR;          // Fatal error, nothing more we can do
          Serial.print("send error");
        }
        else
        {
          send_state[connection] = SEND_WAIT;
          Serial.print("send wait");
        }
        break;

    case SEND_WAIT:
        WIFI_SERIAL.print(T("+++"));                    // Command not acted on
        delay(ONE_SECOND + ONE_SECOND/10);
        send_state[connection] = SEND_OFF;
        break;

    case SEND_BUSY:                                       // Wait for the buffer to be sent
       if ( esp01_waitOK(0,esp01_MAX_WAITOK) )
       {
        send_state[connection] = SEND_OFF;                // Ready to next time
        set_LED(LED_READY);
        return true;
       }
       else
       {
        send_state[connection] = SEND_ERROR;
       }
       break;

    case SEND_ERROR:
        Serial.print(T("{\"SEND_ERROR\":")); Serial.print(connection); Serial.print(T(", \"SENDING\": \"")); Serial.print(str); Serial.print(T("\"}"));
        esp01_status();
        esp01_close(connection);                       // and discard this connection
        send_state[connection] = SEND_OFF;             // Go back to the off state
        return false;                                  // And bail out                    
      }
  }

/*
 * All done, return
 */
  return true;
}

/*----------------------------------------------------------------
 *
 * function: char esp01_receive()
 *
 * brief:    Receive a string over the IP port
 * 
 * return:   Incoming characters saved to a queueu
 *
 *----------------------------------------------------------------
 *   
 *   The ESP-01 receives incoming characters on the IP connection, 
 *   parses the frame, and informs the application of what 
 *   connection received data, the length, and the contents
 *   
 *   ex
 *   
 *   +IPD,l,p:d         // IP-Data comes in from the target
 *                      // l - length
 *                      // p - port
 *                      // d - data
 *   0,CONNECT          // The target connects to the PC
 *   
 *   As with any IP type communications, there is a disconnect
 *   between what is sent and what is received.  In many cases 
 *   the sent data may be broken up over several received frames
 *   
 *   This function is a state machine that parses the incoming
 *   message and then buffers only the 'real' data so that it 
 *   can be extracted later on.
 *   
 *   While this ESP software keeps track of what connection is
 *   connected, esp01_receive() merges all of the connections into
 *   a single input stream.
 *   
 *--------------------------------------------------------------*/

#define WAIT_IDLE    0          // Wait for the + or connection,CONNECT to come along
#define WAIT_MESSAGE 1          // Wait for the a message to come along
#define WAIT_CONNECT 2          // The for the CONNECT message to appear
#define WAIT_CONNECTION 3       // Throw away the connection information
#define WAIT_SIZE    4          // Pull in the size buffer
#define WAIT_DATA    5          // Pull in the rest of the buffer

#define IS_UNKNOWN   0          // As yet the message is unknown
#define IS_CONNECT   1          // Message appears to be CONNECT
#define IS_CLOSED    2          // The message appears to be CLOSED
#define IS_IPD       4          // The message appears to be IPD

static char s_ipd[]     = "IPD,";       // IPD message
static char s_connect[] = "CONNECT\r\n";    // Connect message
static char s_closed[]  = "CLOSED\r\n";     // Disconnect message

void esp01_receive(void)
{
  char                ch;               // Working character
  static unsigned int state = WAIT_IDLE;// Receiver State
  static unsigned int count;            // Expected number of characters
  static unsigned int i;                // Itration counter
  static unsigned int connection;          // connection contained in CONNECT or CLOSED message
  static unsigned int message_type;     // Mesages is one of CONNECT, CLOSED, or IPD
  
/*
 * Determine if we actually have to do anything
 */
  if ( esp01_is_present() == false )
  {
    return;                             // Nope
  }

/*
 * Loop here while we have characters waiting for us
 */
  while ( WIFI_SERIAL.available() != 0 ) // Nothing is waiting for us
  {
    ch = WIFI_SERIAL.read();             // Pull in the next byte
    switch (state)                       // What do we do with it?
    {
      case WAIT_IDLE:                   // Stay here until we see a + or ,
        if ( (ch == '+') || (ch == ',' ) )// Synchronized and ready for the next state
        {
          i = 0;
          message_type = IS_CONNECT + IS_CLOSED + IS_IPD;  // Message could be anything
          state = WAIT_CONNECT;
        }
        
        if ( (ch >= '0') && (ch <= (esp01_N_CONNECT + '0')) ) // Allow only legal connections
        {
          connection = ch - '0';            // Nothing, pretend it is a CONNECT connection identifier (ex 0,CONNECT)  
        }
        break;

      case WAIT_CONNECT:                // If the next character is NOT
        if ( ch != s_connect[i] )       // from CONNECT
        {
          message_type &= ~IS_CONNECT;
        }
        if ( ch != s_closed[i] )        // from CLOSED
        {
          message_type &= ~IS_CLOSED;
        }
        if ( ch != s_ipd[i] )           // from IPD
        {
          message_type &= ~IS_IPD;
        }
          
        if ( message_type == IS_UNKNOWN ) // not from IPD
        {
          state = WAIT_IDLE;            // then, go back to the idle state
        }
        
        i++;                            // Yes, wait for the next character
        if ( (message_type & IS_CONNECT) && (s_connect[i] == 0) )        // Reached the end of CONNECT?
        { 
          Serial.print(T("{\"CONNECTION_START\": ")); Serial.print(connection); Serial.print(T("}"));
          esp01_connect[connection] = true;// Record the connection       
          POST_version();               // Send out the software version to keep the PC happy
          show_echo();                  // Send out the settings
          state = WAIT_IDLE;            // and go back to waiting
        }
        if ( (message_type & IS_CLOSED) && (s_closed[i] == 0) )         // Reached the end of CLOSED?
        {                 
          Serial.print(T("{\"CONNECTION_CLOSE\":")); Serial.print(connection); Serial.print(T("}"));
          esp01_connect[connection] = false;// No longer a valid connection
          state = WAIT_IDLE;            // Go back to waiting       
        }
        if ( (message_type & IS_IPD) && (s_ipd[i] == 0) )               // Reached the end of IPD?
        {
          state = WAIT_CONNECTION;         // Go and pick up the connection number
        }
        break;
        
      case WAIT_CONNECTION:             // Throw away the connection since all input goes into one stream
        if ( ch == ',' )                // Don't do anything until you see a comma
        {
          state = WAIT_SIZE;
          count = 0;                    // Reset the count field
        }
        break;
        
      case WAIT_SIZE:                   // Scoop up the message size
        if ( ch == ':' )                // Stay here until you see a colin
        {
          state = WAIT_DATA;            // Then jump to the next state
        }
        else
        {
          count = (count*10) + (ch -'0');  // Accumulate the count
        }
        break;

      case WAIT_DATA:
        esp01_in_queue[esp01_in_ptr] = ch;  // Save the data
        esp01_in_ptr = (esp01_in_ptr+1) % sizeof(esp01_in_queue);
        count--;                        // One less to read
        if ( count == 0 )               // No more?
        {
          state = WAIT_IDLE;            // Wait for the next message
        }
        break;
    }
  }

 /*
  * That's it for this call
  */
  return;
}
/*----------------------------------------------------------------
 *
 * function: bool esp01_connected()
 *
 * brief:    Return the connection status
 * 
 * return:   TRUE if any connection is connected
 *
 *----------------------------------------------------------------
 *   
 *   Loops throught the available connections and ORs together the
 *   status of each connection
 *   
 *--------------------------------------------------------------*/
bool esp01_connected(void)
{ 
  unsigned int i;
  bool return_value;

  return_value = false;

  for (i=0; i != esp01_N_CONNECT; i++)
  {
    return_value |= esp01_connect[i];
  }
  
  return return_value;
}

/*----------------------------------------------------------------
 *
 * function: bool esp01_close()
 *
 * brief:    Close the open connections
 * 
 * return:   Nothing
 *
 *----------------------------------------------------------------
 *   
 *   Close the connection associated with this connection (connection)
 *   
 *   
 *--------------------------------------------------------------*/
void esp01_close
  (
  unsigned int connection                  // connection of connection to close
  )
{ 
  unsigned int i;
  char str[32];
  
/*
 * Send out the AT break if needed
 */
  if ( esp01_is_present() );
  {
    WIFI_SERIAL.print(T("+++"));
    delay(ONE_SECOND);
      
    esp01_connect[connection] = false;                                 // and discard this connection
    sprintf(str, "AT+CIPCLOSE=%d\r\n", connection);                    // Close this connection
    WIFI_SERIAL.print(str);
    sprintf(str, "{\"CONNECTION_CLOSE\": %d}\r\n", connection);   // Close this connection
    Serial.print(str);
          
/*
 * Wait and eat any junk that might be coming back
 */
    delay(ONE_SECOND);
    while ( WIFI_SERIAL.available() )
    {
      WIFI_SERIAL.read();
    }
  }
  
/*
 * All done, return
 */
  return;
}
