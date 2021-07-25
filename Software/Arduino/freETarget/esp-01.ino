
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
 * Instead the incoming messages are tagged with a channel number
 * and outgoign messages are prepended with a channel number.
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

#include "freETarget.h"

/*
 * Function Prototypes
 */
static bool esp01_esp01_waitOK(void);   // Wait for an OK to come back
static void esp01_flush(void);          // Flush any pending messages

/*
 * Local Variables
 */
static bool esp01_present = false;      // TRUE if the esp 01 is installed

static char esp01_in_queue[32];         // Place to store incoming characters
static int  esp01_in_ptr;               // Queue in pointer
static int  esp01_out_ptr;              // Queue out pointer

static bool esp01_connect[] = {false, false, false}; // Set to true when a channel (0-2) connects

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
 *--------------------------------------------------------------*/
void esp01_init(void)
{
  if ( is_trace )
  {
    Serial.print("\r\nInitializing ESP-01");
  }
  
/*
 * Determine if the ESP-01 is attached to the Accessory Connector
 */
  if (esp01_is_present() == false )
  {
    if ( is_trace ) 
    {
      Serial.print("\r\nESP-01 Not Found");
    }
    return;                                     // No hardware installed, nothing to do
  }
  if ( is_trace )
  {
    Serial.print("\r\nESP-01 Present");
  }

  esp01_restart();
  delay(ONE_SECOND);
  esp01_flush();
  
/*
 * There is an ESP-01 on the freETarget.  We need to program it
 */
  AUX_SERIAL.print("ATE0\r\n");                   // Turn off echo (don't use it)
  if ( (esp01_waitOK() == false) && is_trace )
  {
    Serial.print("\r\nESP-01: Failed ATE0");
  }
  
  AUX_SERIAL.print("AT+RFPOWER=80\r\n");          // Set almost max power
  if ( (esp01_waitOK() == false) && ( is_trace ) )
  {
    Serial.print("\r\nESP-01: Failed AT+RFPOWER=80");  
  } 

  AUX_SERIAL.print("AT+CWMODE_DEF=2\r\n");        // We want to be an access point
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CWMODE_DEF=2");
  }
 
  AUX_SERIAL.print("AT+CWSAP_DEF="); AUX_SERIAL.print("\"FET-"); AUX_SERIAL.print(names[json_name_id]); AUX_SERIAL.print("\",\"NA\",5,0\r\n");
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: AT+CWSAP_DEF=FET-"); Serial.print(names[json_name_id]);
  }  
 
  AUX_SERIAL.print("AT+CWDHCP_DEF=0,1\r\n");      // DHCP turned on
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CWDHCP_DEF=0,1");
  }
  
  AUX_SERIAL.print("AT+CIPAP_DEF=\"192.168.10.9\",\"192.168.10.9\"\r\n"); // Set the freETarget IP to 192.168.10.9
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CIPAP_DEF=\"192.168.10.9\",\"192.168.10.9\"");
  }

  AUX_SERIAL.print("AT+CWDHCPS_DEF=1,2800,\"192.168.10.0\",\"192.168.10.8\"\r\n");          // Set the PC IP to 192.168.10.0.  Lease Time 2800 minutes
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CWDHCPS_DEF=1,2800,\"192.168.10.0\",\"192.168.10.8\"");
  }
    
  AUX_SERIAL.print("AT+CIPMUX=1\r\n");           // Allow a single connection
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CIPMUX=1");
  }
  
  AUX_SERIAL.print("AT+CIPSERVER=1,1090\r\n");   // Turn on the server and listen on port 1090
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CIPSERVER=1,1090");
  }
  
  AUX_SERIAL.print("AT+CIPSTO=7000\r\n");        // Set the server time out
  if ( (esp01_waitOK() == false) && (is_trace) )
  {
    Serial.print("\r\nESP-01: Failed AT+CIPSTO=7000");
  }
  
/*
 * All done, return
 */
  if ( is_trace )
  {
    Serial.print("\r\nESP-01 Initialization complete");
  }
  return;
}


/*----------------------------------------------------------------
 *
 * function: void esp01_restart
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
bool esp01_restart(void)
{
  char ch;
  
/*
 * Determine if the ESP-01 is attached to the Accessory Connector
 */
  if (esp01_is_present() == false )
  {
    return false;                               // No hardware installed, nothing to do
  }
  
/*
 * Send out the break then restart the module
 */
  AUX_SERIAL.print("+++");
  delay(ONE_SECOND);
  AUX_SERIAL.print("AT+RST\r\n");

/*
 * All done, 
 */
  esp01_connect[0] = false;
  esp01_connect[1] = false;
  esp01_connect[2] = false;
  
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

  AUX_SERIAL.print("AT\r\n");             // Send out an AT command to the port
  esp01_present = esp01_waitOK();         // and wait for the OK to come back

/*
 * All done, return the esp-01 present state
 */
  esp01_first = false;                   // Done it once.
  return esp01_present;
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
   while ( AUX_SERIAL.available() != 0 ) 
   {
    AUX_SERIAL.read();
   }

 /*
  * The buffer is empty, go home
  */
   return;
 }


/*----------------------------------------------------------------
 *
 * function: bool esp01_waitOK()
 *
 * brief:    Wait for the OK to come back
 * 
 * return:   FALSE if no OK came back
 *
 *----------------------------------------------------------------
 *   
 *   Loop for a second to see if OK is returned.
 *   
 *   The state machine ensures that OK is parsed
 *   
 *--------------------------------------------------------------*/
#define MAX_esp01_waitOK  1000000       // 1M microseconds

#define GOT_NUTHN 0                     // Decoding states
#define GOT_O     1

static bool esp01_waitOK(void)
{
  char         ch;                      // Character from port
  unsigned int state;                   // OK decoding state
  long         start;                   // Timer start

  start = micros();                     // Remember the starting time
  state = GOT_NUTHN;                    // Start off empty
  
/*
 * Loop for a second and see if OK comes back
 */
  while ( (micros() - start) < MAX_esp01_waitOK )
  {
    if ( AUX_SERIAL.available() != 0 ) 
    {
      ch = AUX_SERIAL.read();

      switch (state)
      {
        default:
        case GOT_NUTHN:
          if ( ch == 'O' )                // Got an O, wait for the K
          {
            state = GOT_O;
          }
          break;

        case GOT_O:
          if ( ch == 'K' )                // Got O then K
          {
            return true;                  // Done
          }
          else                            // Got O but no K
          {
            state = GOT_NUTHN;            // Start over
          }
          break;          
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
 *   Not attached - Perform regular AUX_SERIAL.read()
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
    return AUX_SERIAL.read();           // Just do a regular read from the aux port
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
    esp01_out_ptr++;                    // Bump up the output pointer
    esp01_out_ptr %= sizeof(esp01_in_queue);// and wrap around
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
 *   Not attached - Perform regular AUX_SERIAL.gavailable()
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
    return AUX_SERIAL.available();      // Just do a regular read fromthe aux port
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
 * return:   TRUE if the channel is active
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
 *   over the IP channel. The ESP-01 will also stop collecting 
 *   bytes and send the data if a NULL is received.
 *   
 *   So the freETarget implements the send function by triggering
 *   the IP channel with the AT+CIPSENDEX command and then lets 
 *   the application send out bytes over the AUX port as usual 
 *   then the application sends out a NULL to kick off the 
 *   sending process.
 *   
 *   It is possible for the ESP-01 to be attached, but nothing 
 *   connected via the server.  This function will not get the >
 *   prompt and hang indefinitly.  For this reason the waiting
 *   loop will us a timer to exit if nothing comes back
 *   
 *--------------------------------------------------------------*/

bool esp01_send
  (
    bool start,                         // TRUE if starting a transmission
    int  index                          // Which index (connection) to send on
  )
{
  unsigned int x;                       // Working character
  long         timer;                   // Timer start
  
/*
 * Determine if we actually have to do anything
 */
  if ( (esp01_is_present() == false)    // No ESP at all
      || ( esp01_connect[index] == false )) // No connection on this channel
  {
    return false;
  }

/*  
 *   We have an ESP-01 attached,  Figure out the bytes in the queueu
 */
  if ( start )
  {
    AUX_SERIAL.print("AT+CIPSENDEX="); AUX_SERIAL.print(index); AUX_SERIAL.print(",2047\r\n");   // Start and lie that we will send 2K of data

    timer = micros();                               // Remember the starting time
    while ( (micros() - timer) < MAX_esp01_waitOK ) // Wait for the > to come back within a second
    {
      if ( AUX_SERIAL.available() != 0 )            // Something available
      {
        if ( AUX_SERIAL.read() == '>' )             // Is it the prompt?
        {
          return true;                              // Yes, Ready to send out 
        }
      }
    }
  }
  else
  {
    AUX_SERIAL.print("\\0");                       // End by sending a null out the port
  }

/*
 * All done, return
 */
  return false;
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
 *   The ESP-01 receives incoming characters on the IP channel, 
 *   parses the frame, and informs the application of what 
 *   channel received data, the length, and the contents
 *   
 *   ex
 *   
 *   +IPD,0,1:b         // Data comes in from the target
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
 *--------------------------------------------------------------*/

#define WAIT_IDLE    0          // Wait for the + or channel,CONNECT to come along
#define WAIT_MESSAGE 1          // Wait for the a message to come along
#define WAIT_CONNECT 2          // The for the CONNECT message to appear
#define WAIT_CHANNEL 3          // Throw away the channel information
#define WAIT_SIZE    4          // Pull in the size buffer
#define WAIT_DATA    5          // Pull in the rest of the buffer

#define IS_UNKNOWN   0          // As yet the message is unknown
#define IS_CONNECT   1          // Message appears to be CONNECT
#define IS_CLOSED    2          // The message appears to be CLOSED
#define IS_IPD       4          // The message appears to be IPD

static char s_ipd[]     = "IPD,";       // IPC message
static char s_connect[] = "CONNECT\r\n";    // Connect message
static char s_closed[]  = "CLOSED\r\n";     // Disconnect message

void esp01_receive(void)
{
  char                ch;               // Working character
  static unsigned int state = WAIT_IDLE;// Receiver State
  static unsigned int count;            // Expected number of characters
  static unsigned int i;                // Itration counter
  static unsigned int channel;          // Channel contained in CONNECT or CLOSED message
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
  while ( AUX_SERIAL.available() != 0 ) // Nothing is waiting for us
  {
    ch = AUX_SERIAL.read();             // Pull in the next byte

    switch (state)                      // What do we do with it?
    {
      case WAIT_IDLE:                   // Stay here until we see a + or ,
        if ( (ch == '+') || (ch == ',' ) )// Synchronized and ready for the next state
        {
          i = 0;
          message_type = IS_CONNECT + IS_CLOSED + IS_IPD;  // Message could be anything
          state = WAIT_CONNECT;
        }
        else
        {
          channel = ch - '0';            // Nothing, pretend it is a CONNECT channel identifier (ex 0,CONNECT)  
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
          esp01_connect[channel] = true;// Record the channel                
          POST_version(PORT_AUX);       // Send out the software version to keep the PC happy
          state = WAIT_IDLE;            // and go back to waiting
        }
        if ( (message_type & IS_CLOSED) && (s_closed[i] == 0) )         // Reached the end of CLOSED?
        {                 
          esp01_connect[channel] = false;// No longer a valid channel
          state = WAIT_IDLE;            // Go back to waiting       
        }
        if ( (message_type & IS_IPD) && (s_ipd[i] == 0) )               // Reached the end of IPD?
        {
          state = WAIT_CHANNEL;         // Go and pick up the channel number
        }
        break;
        
      case WAIT_CHANNEL:                // Throw away the CHANNEL
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
          count *= 10;
          count += ch -'0';             // Accumulate the count
        }
        break;

      case WAIT_DATA:
        esp01_in_queue[esp01_in_ptr] = ch;  // Save the data
        esp01_in_ptr++;                     // and bump up the pointer
        esp01_in_ptr %= sizeof(esp01_in_queue);
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
