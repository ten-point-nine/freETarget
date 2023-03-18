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

static int my_token;            // Token ring address
static int my_ring;             // Set to 1 if I own the ring
static int whos_ring;           // Who owns the ring right now?

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
 
  if ( (json_token != TOKEN_NONE)                 // Not in token ring mode
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

  if ( json_token == TOKEN_MASTER )           // Is this the token master?
  {
    my_token = 1;                             // Yes, set myself to 1
  }
  else
  {
    my_token = TOKEN_UNDEF;                   // No, the token is unassigned
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
    if ( token_poll() )                       // Wait for something to come back
    {
      break;
    }
  }


  
/*
 * All done, return
 */  
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
 int token_poll(void)
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
    case TOKEN_TAKE:                                  // A take is passing aroound
    case TOKEN_RELEASE:                               // A release is passing around
        break;                                        // Take it off of the ring
                           
          
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
          return 0;
         }
         return 0;
             

    default:                                          // Not a control byte
      Serial.print(token);                            // Send it out the USB port
      return 0;
      
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
        return 0;

          
    case TOKEN_ENUM:                                  // An enumeration byte is passing around
        my_token = token & TOKEN_RING;                // Extract the node number
        AUX_SERIAL.print( TOKEN_ENUM | (my_token+1) ); // Add 1 and send it along
        return 0;
        
    case TOKEN_TAKE:                                  // A take is passing aroound
        whos_ring = token & TOKEN_RING;
        if ( whos_ring == my_token )                  // Is it me?
        {
          my_ring = true;
        }
        else
        {
          my_ring = false;
        }
        AUX_SERIAL.print( token);                      // Pass it along to the master
        return 0;
        
    case TOKEN_RELEASE:                               // A release is passing around
        if ( (token & TOKEN_RING) == my_token )       // Is it me?
        {
          whos_ring = TOKEN_UNDEF;                    // Yes, Release it
        }
        AUX_SERIAL.print(token);                      // Pass it along to the master
        return 0;                                     // 
                           
    default:                                          // Not a control byte
      Serial.print(token);                            // Send it out the USB port
      return 0;
      
    }
  }

   
 /*
  * All done, return
  */
  return 0;
 }
  
