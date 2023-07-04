/*-------------------------------------------------------
 * 
 * token.ino
 * 
 * token ring driver
 * 
 * ------------------------------------------------------
 *
 *  The token ring driver for FreeETarget is intended to be
 *  used when chaining a number of targets together to report
 *  as one target, for example five bay rapid fire.
 *  
 *  The token ring driver has a number of modes of operation
 *  
 *  1 - TOKEN_WIFI -    The auxilary port is used for the ESP01 
 *                      WiFi adapter, andno token ring operations
 *                      are supported
 *
 *  2 - TOKEN_MASTER -  The token ring is connected to the PC via
 *                      USB and messages on the ring are passed
 *                      to and from the PC
 *                      
 *  3 - TOKEN_SLAVE -   These are subordinate devices that report
 *                      scores to the PC via the master
 *                      
 *  How It Works
 *  
 *  Enumeration
 *  
 *  The auxilary ports of the FreETarget shields are connected
 *  to each other in a daisy chain.  On power up the new device
 *  requests a token enumberation and if the ring is complete
 *  the master receives the request and then sends out an enumberatoin
 *  which is incremented by 1 for each node it passes through,
 *  When the enueration comes back to the master the enumeeration
 *  is taken off of the bus
 *  
 *  Buws Request and Bus Give
 *  
 *  At any one time, the buw belongs to nobody or a particular
 *  node.  Nodes can only put messages on the bus if they own 
 *  the bus.  To do so, the node checks if the bus is in use and
 *  then asserts a bus request which arrives at the master.  If
 *  no node owns the bus then the master sends out a bus take
 *  message identifying the new owner.  From then on the node
 *  can send messages over the ring to the master and then onto
 *  the PC.
 *  
 *  When finished, the node sends a bus give message and the
 *  master tells the other nodes that nobody owns the bus.
 *  
 *  Broadcast
 *  
 *  Messages arriving from the PC (for example RAPID_ENABLE)
 *  are picked up by the master over the USB and then sent
 *  down the network as-is.  Each node will receive the message
 *  for it's own use and pass it along to the next node until
 *  it arrives back at the master and removed
 *  
 *******************************************************************/

#include "freETarget.h"
#include "token.h"
#include "esp-01.h"

int my_ring = TOKEN_UNDEF;          // Token ring address
int whos_ring = TOKEN_UNDEF;        // Who owns the ring right now?

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

/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_WIFI)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  if ( DLT(DLT_CRITICAL) ) 
  {
    Serial.print(T("token_init()"));  
  }
  
/*
 * Send out the token initializaation request
 */
  if ( json_token == TOKEN_MASTER )
  {
    AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_ENUM | 1 + 1) );                           // Master, send out an enum
  }
  else
  {
    AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_ENUM_REQUEST | (my_ring & TOKEN_RING)));   // Slave, Send out the request
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
void token_poll(void)
 {
  char token;                                         // Token read from serial port
  unsigned long now;
  
/*
 * If we are not in token ring mode, then copy the aux spool to the json spool
 */
  switch ( json_token )
  {
    case TOKEN_WIFI:                                  // No token ring installed
      while ( aux_spool_available() )                  // Is there something waiting for us?
      {
          token = aux_spool_read();                    //  Pick it up
          json_spool_put(token);                       //  and save it for our own use
      }  
      break;
 
/*
 * Handle stuff coming from the PC for broadcast
 */
    case TOKEN_MASTER:                                // Am I the master connected to the PC?  
      while ( Serial.available() )                    // Is there something waiting for us?
      {
        token = Serial.read();                        //  Pick it up
        AUX_SERIAL.print(token);                      //  Pass it along
        json_spool_put(token);                        //  and save it for our own use
      }     
                                  
/*
 * Handle local token ring traffic
 */
    
    while ( aux_spool_available() )
    {
      token = aux_spool_read();
      if (DLT(DLT_INFO) )                             // and not in trace mode (DIAG jumper installed)
      {
        Serial.print("Master Rx: 0x"); Serial.print((char)token, HEX); Serial.print(T(" ")); Serial.print((char)token);
      }

      if ( token & TOKEN_BYTE )
      {
        switch ( token & (TOKEN_CONTROL) )                  // Yes, act on it
        {
          case (TOKEN_ENUM_REQUEST):                          // A new device has requested an enum
            AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_ENUM | 1 + 1) ); // Yes, start the enumeration at 2 
            if (DLT(DLT_INFO) )                              // and not in trace mode (DIAG jumper installed)
            {
              Serial.print(T("{\"TOKEN_ENUM\":")); Serial.print((int)(token & TOKEN_RING)); Serial.print(T("}"));
            }
            break;
          
          case TOKEN_ENUM:                                    // An enumeration byte is passing around
            my_ring = 1;                                      // Master is alwayshot_mm 0
            whos_ring = TOKEN_UNDEF;                          // Nobody owns the ring right now
            if (DLT(DLT_INFO) )
            {
              Serial.print(T("{\"TOKEN\":")); Serial.print(millis()); Serial.print(T("}"));
            }
            break;
        
          case TOKEN_TAKE:                                    // A take is passing aroound
            if ( (token & TOKEN_RING) == my_ring )            // Is it me?
            {
              whos_ring = my_ring;                            // Yes, Grab it
            }
            break;                                            // 
                           
          case TOKEN_RELEASE:                                 // A release is passing around
              whos_ring = TOKEN_UNDEF;                        // Yes, Release it
              break;                                          // 
            
          case TOKEN_TAKE_REQUEST:                            // Request ownership of the bus
            if ( whos_ring == TOKEN_UNDEF )                   // Is the ring available?
            {
              whos_ring = token & TOKEN_RING;                 // Yes, give ownership
              AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_TAKE | (token & TOKEN_RING)));   // and pass it along
            }
            break;                                            // Otherwise throw it away

          case TOKEN_RELEASE_REQUEST:                         // Give up ownership of the bus
            if ( whos_ring == (token & TOKEN_RING) )          // Is the person asking the owner too?
            {
              whos_ring = TOKEN_UNDEF;                        // Yes, the ring is now undefined
              AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_RELEASE | (token & TOKEN_RING)));  // and pass it along
            }
            break;
             
          default:                                            // Not a control byte
            Serial.print(token);                              // Send it out the serial port
            break;    
          }
      }
      else
      {  
        if ( whos_ring != TOKEN_UNDEF )                       // Is the ring undefined?
        {
          Serial.print(token);                                // Then don't send anything onwards
        }
      }
    }    
    break;
  
/*
 * Regular node
 */
  case TOKEN_SLAVE:
    while( aux_spool_available() )
    {
      token = aux_spool_read();
      if (DLT(DLT_INFO) )                                   // and not in trace mode (DIAG jumper installed)
      {
        Serial.print("Slave Rx: 0x"); Serial.print((char)token, HEX); Serial.print(T(" ")); Serial.print((char)token);
      }

      if ( token & TOKEN_BYTE )
      {
        switch ( token & (TOKEN_CONTROL) )                  // Yes, act on it
        {
          case TOKEN_ENUM_REQUEST:                          // A new device has requested an enum
          case TOKEN_TAKE_REQUEST:                          // Request ownership of the bus     
          case TOKEN_RELEASE_REQUEST:                       // Give up ownership of the bus      
              AUX_SERIAL.print( token );                    // Just pass it along
              break;
  
          case TOKEN_ENUM:                                  // An enumeration byte is passing around
              my_ring = token & TOKEN_RING;                 // Extract the node number
              whos_ring = TOKEN_UNDEF;                      // Nobody owns the ring right now?
              if (DLT(DLT_INFO) )                           // and not in trace mode (DIAG jumper installed)
              { 
                Serial.print(T("{\"TOKEN\":")); Serial.print(millis()); Serial.print(T("}"));
              }
              AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_ENUM | (my_ring+1)) ); // Add 1 and send it along
              break;
        
          case TOKEN_TAKE:                                  // A take is passing aroound
              whos_ring = token & TOKEN_RING;
              if ( whos_ring == my_ring )
              {
                set_LED(LED_WIFI_SEND);
              }
              AUX_SERIAL.print(token);                      // Pass it along to the master
              break;
          
          case TOKEN_RELEASE:                               // A release is passing around
              whos_ring = TOKEN_UNDEF;                      // Yes, Release it
              set_LED(LED_READY);                           // And show it is ready
              AUX_SERIAL.print(token);                      // Pass it along to the master
              break;                                        // 
               
          default:                                          // Not a control byte
             AUX_SERIAL.print(token);                       // Send it on to the next node
             break;  
        }
      }
      else
      {
        AUX_SERIAL.print(token);                            // Send it to the next guy
        if ( whos_ring == TOKEN_UNDEF)                      // and the ring is not in use
        {      
          json_spool_put(token);                            // Ring in broadcast
        }                                
      }
    }
    break;
  }

/*
 * See if the waaiting ation has been completed
 */

   return;
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
  
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_WIFI)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  else if ( DLT(DLT_INFO) ) 
  {
    Serial.print(T("token_take()"));  
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
  AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_TAKE_REQUEST | my_ring));   // Send out the request

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
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_WIFI)                 // Not in token ring mode
    || (esp01_is_present() == 1) )                // In WiFi mode
  {
    return 0;
  }
  
  else if ( DLT(DLT_INFO) ) 
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
  AUX_SERIAL.print((char)(TOKEN_BYTE | TOKEN_RELEASE_REQUEST | my_ring));// Send out the request
  
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
/*
 * If not in token ring mode or WiFi is present,do nothing
 */
  if ( (json_token == TOKEN_WIFI)                 // Not in token ring mode
    || (esp01_is_present() == 1)                  // In WiFi mode
    || ((whos_ring == my_ring ) && (my_ring != TOKEN_UNDEF)))  // Or the ring belong to me
  {
    return 1;
  }

/*
 * The ring is not available to me
 */  
  return 0;
}
