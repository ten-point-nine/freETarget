/*-------------------------------------------------------
 * 
 * token.ino
 * 
 * token ring driver
 * 
 * ----------------------------------------------------*/

#include "freETarget.h"
#include "token.h"
#include "esp-01.h"

static int my_ring;             // Token ring address
static int whos_ring;           // Who owns the ring right now?
static long my_time = 0;        // Starting time reference

/*-----------------------------------------------------
 * 
 * function: token_init
 * 
 * brief:    Prepare the token ring
 * 
 * return:   Token ring address
 * 
 *-----------------------------------------------------
 *
 * Issue a token ring enumeration request an wait for 
 * an enumeration.  
 * 
 * If nothing happens after TOKEN time seconds,then
 * return and wait for someone else to trigger the
 * enumeration
 *-----------------------------------------------------*/

void token_init(void)
{
  unsigned long now;
  unsigned char token;      // Token

/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_NONE)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  if ( DLT(INIT_TRACE) ) 
  {
    Serial.print(T("token_init()"));  
  }
  else if ( DLT(DLT_CRITICAL) ) 
  {
    Serial.print(T("token_init()"));  
  }

  my_ring = TOKEN_UNDEF;                      // The ring is unowned
  whos_ring = TOKEN_UNDEF;                    // The ring is available
  
/*
 * Send out the token initializaation request
 */
  now = millis();                             // Remember the start time
  AUX_SERIAL.print(TOKEN_ENUM_REQUEST);       // Send out the request

  while ( (millis() - now) <= TOKEN_TIME_OUT )
  {
    if ( token_poll(TOKEN_ENUM) )             // Wait for something to come back
    {
      break;
    }
  }

  
/*
 * All done, return
 */  
  sys_time_reset();                           // Put the clock to 0
  return;
}


/*-----------------------------------------------------
 * 
 * function: token_poll
 * 
 * brief:    Look for something on the token ring
 * 
 * return:   TRUE if the action has been completed
 * 
 *-----------------------------------------------------
 *
 * This listens for token ring comands to come along
 * and if available then carry out the action.  Otherwise
 * just send the character along to the next processor on
 * the ring
 * 
 *-----------------------------------------------------*/
 int token_poll
 (
    unsigned int token_wait                           // Action needed to exit poll
 )
 {
  int token;                                          // Token read from serial port

/*
 * Test to see if this function needs to do anything 
 */
  if ( json_token == TOKEN_NONE )                     // No token ring installed
  {
    return 0;
  }
  if ( AUX_SERIAL.available() == 0 )                  // Got something?
  {
     return 0;                                        // No, return nothing
  }

/*
 * There is something in the AUX port waiting for us
 */
   token = AUX_SERIAL.read();
   if ( (token & TOKEN_BYTE) == 0  )                  // Is it a token?
   {
      AUX_SERIAL.print(token);                        // No, send it along
      return 0;                                       // and pretend nothing happened
   }

 /*
  * It's a token control, act on the token
  */
  if ( json_token == TOKEN_MASTER )                   // Am I the master connected to the PC?
  {
    switch ( token & (TOKEN_CONTROL) )                // Yes, act on it
    {
      case (TOKEN_ENUM_REQUEST):                      // A new device has requested an enum
        AUX_SERIAL.print( TOKEN_BYTE | TOKEN_ENUM + 1 ); // Yes, start the enumeration at 1
        break;
          
    case TOKEN_ENUM:                                  // An enumeration byte is passing around
        my_ring = token & TOKEN_RING;                 // Extract the node number
        whos_ring = TOKEN_UNDEF;                      // Nobody owns the ring right now?
        break;
        
    case TOKEN_TAKE:                                  // A take is passing aroound
        if ( (token & TOKEN_RING) == my_ring )        // Is it me?
        {
          whos_ring = my_ring;                        // Yes, Grab it
        }
        break;                                        // 
                           
    case TOKEN_RELEASE:                               // A release is passing around
        if ( (token & TOKEN_RING) == my_ring )        // Is it me?
        {
          whos_ring = TOKEN_UNDEF;                    // Yes, Release it
        }
        break;                                        // 
        
    case TOKEN_TAKE_REQUEST:                          // Request ownership of the bus
        if ( whos_ring == TOKEN_UNDEF )               // Is the ring available?
        {
          whos_ring = token & TOKEN_RING;             // Yes, give ownership
          AUX_SERIAL.print(TOKEN_TAKE + whos_ring);   // and pass it along
        }
        break;                                        // Otherwise throw it away

            
    case TOKEN_RELEASE_REQUEST:                       // Give up ownership of the bus
        if ( whos_ring == (token & TOKEN_RING) )      // Is the ring owned the node requesting it?
        {
          AUX_SERIAL.print(TOKEN_TAKE + (token & TOKEN_RING));  // and pass it along
          whos_ring = TOKEN_UNDEF;                    // Yes, the ring is now undefined
         }
         break;
             
    default:                                          // Not a control byte
      break;      
    }    
  }
  
/*
 * Regular node
 */
  else
  {
    switch ( token & (TOKEN_CONTROL) )                // Yes, act on it
    {
      case TOKEN_ENUM_REQUEST:                        // A new device has requested an enum
      case TOKEN_TAKE_REQUEST:                        // Request ownership of the bus     
      case TOKEN_RELEASE_REQUEST:                     // Give up ownership of the bus      
        AUX_SERIAL.print( token );                    // Just pass it along
        break;

    case TOKEN_ENUM:                                  // An enumeration byte is passing around
        my_ring = token & TOKEN_RING;                 // Extract the node number
        whos_ring = TOKEN_UNDEF;                      // Nobody owns the ring right now?
        sys_time_reset();                             // Synchronize the clocks
        AUX_SERIAL.print( TOKEN_ENUM | (my_ring+1) ); // Add 1 and send it along
        break;
        
    case TOKEN_TAKE:                                  // A take is passing aroound
        whos_ring = token & TOKEN_RING;
        AUX_SERIAL.print( token);                      // Pass it along to the master
        break;
        
    case TOKEN_RELEASE:                               // A release is passing around
        if ( (token & TOKEN_RING) == my_ring )        // Is it me?
        {
          whos_ring = TOKEN_UNDEF;                    // Yes, Release it
        }
        AUX_SERIAL.print(token);                      // Pass it along to the master
        break;                                        // 
                           
    default:                                          // Not a control byte
      Serial.print(token);                            // Send it out the USB port
      break;
      
    }
  }

/*
 * See if the waaiting ation has been completed
 */
    if ( (token & TOKEN_CONTROL) == token_wait )
    {
      return 1;
    }
    else
    {
      return 0;
    }
 }

  
/*-----------------------------------------------------
 * 
 * function: token_take
 * 
 * brief:    Request to take the token ring
 * 
 * return:   who owns the ring
 * 
 *-----------------------------------------------------
 *
 * Issue a token ring take request and wait for 
 * the reply to come back
 * 
 *-----------------------------------------------------*/
int token_take(void)
{
  unsigned long now;
  
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_NONE)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  else if ( DLT(DLT_CRITICAL) ) 
  {
    Serial.print(T("token_init()"));  
  }

/*
 * Check to see if the token ring is alreay used
 */
   if ( (whos_ring != TOKEN_UNDEF) )               // The ring belongs to somebody
   {
    return 1;
   }
   
/*
 * Send out the token initializaation request
 */
  now = millis();                                   // Remember the start time
  AUX_SERIAL.print(TOKEN_TAKE_REQUEST + my_ring);   // Send out the request

  while ( (millis() - now) <= TOKEN_TIME_OUT )
  {
    if ( token_poll(TOKEN_TAKE) )                  // Wait for something to come back
    {
      break;
    }
  }

/*
 * All done, return
 */  
  return 1;
}

/*-----------------------------------------------------
 * 
 * function: token_give
 * 
 * brief:    Return the token ring
 * 
 * return:   who owns the ring
 * 
 *-----------------------------------------------------
 *
 * Issue a token ring give request an wait for 
 * the reply to come back
 * 
 *-----------------------------------------------------*/
int token_give(void)
{
  unsigned long now;
  
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_NONE)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  else if ( DLT(DLT_CRITICAL) ) 
  {
    Serial.print(T("token_give()"));  
  }

/*
 * Check to see if the token ring is alreay used
 */
   if ( whos_ring != my_ring )                    // The ring belongs to somebody
   {
    return 0;
   }
   
/*
 * Send out the token initializaation request
 */
  now = millis();                                   // Remember the start time
  AUX_SERIAL.print(TOKEN_RELEASE_REQUEST + my_ring);// Send out the request

  while ( (millis() - now) <= TOKEN_TIME_OUT )
  {
    if ( token_poll(TOKEN_RELEASE) )                // Wait for something to come back
    {
      break;
    }
  }

/*
 * All done, return
 */  
  return 1;
}



/*-----------------------------------------------------
 * 
 * function: token_available
 * 
 * brief:    Test to see if the token ring is available
 * 
 * return:   TRUE if the ring is available
 * 
 *-----------------------------------------------------
 *
 * The token ring outptu is available if 
 * 
 *     We are not in token ring mode
 *     The WiFi is connected
 *     We have requested and received the ring
 * 
 *-----------------------------------------------------*/
int token_available(void)
{
  unsigned long now;
  
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_NONE)                 // Not in token ring mode
    || (esp01_is_present() == 1)                  // In WiFi mode
    || (my_ring == whos_ring ) )                  // Or the ring belong to me
  {
    return 1;
  }

/*
 * The ring is not available to me
 */  
  return 0;
}


/*-----------------------------------------------------
 * 
 * function: sys_time()
 * 
 * brief:    Return the system time across all token ring
 * 
 * return:   Time in milliseconds
 * 
 *-----------------------------------------------------
 *
 * All members of the token ring share a common time
 * base.  This function returns the time in milliseconds
 * 
 *-----------------------------------------------------*/
unsigned long sys_time(void)
{
  if ( my_time == 0 )
  {
    my_time = millis();
  }

  return millis() - my_time;
}


/*-----------------------------------------------------
 * 
 * function: sys_time_reset
 * 
 * brief:    Reset the system time to zero
 * 
 * return:   None
 * 
 *-----------------------------------------------------
 *
 * All members of the token ring share a common time
 * base.  This function returns the time in milliseconds
 * 
 *-----------------------------------------------------*/
void sys_time_reset(void)
{
  my_time = millis();
  return;

}
