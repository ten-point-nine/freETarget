/*----------------------------------------------------------------
 *
 * json.h
 *
 * Header file for JSON functions and called routines
 *
 *---------------------------------------------------------------*/
#ifndef _JSON_H_
#define _JSON_H_

#include "trace.h"
/*
 * Public Functions
 */
void reset_JSON(void);                           // Clear the JSON input buffer
void trace_json(void *);                         // Task to scan the serial port looking for JSON input
void show_echo(void);                            // Display the settings
bool json_find_first(void);                      // Find the start of the input stream
bool json_get_array_next(int type, void *value); // Pull in the next array value
void json_tabata(bool tabata_enable);            // Start or stop a Tabata session
bool json_get_next_string(char *str, int size);  // Get the next string from the JSON input

/*
 * JSON message typedefs
 */
typedef struct
{
  int   show;       // Display attributes
  char *token;      // JSON token string, ex "RADIUS":
  int  *value;      // Where value is stored (cast to (real_t*) if needed)
  int   convert;    // Conversion type
  void (*f)(int x); // Function to execute with message
  char *non_vol;    // Storage in NON-VOL
  int   init_value; // Initial Value
  int   ps_version; // What persistent storage version was this introduced
} json_message_t;

extern const json_message_t JSON[];

/*
 * Definitioins
 */
#ifdef JSON_C
#define EXTERN
#else
#define EXTERN extern
extern const json_message_t JSON[];
extern char                 input_JSON[];
#endif

#define SHOW        0x01                               // Show the value
#define HIDE        0x00                               // Hide the value
#define LOCK        0x02                               // The value is a secret
#define MFS_DISPLAY 0x04                               // Display as MFS
#define IS_FIXED    (1 << 8)                           // The value cannot be changed
#define IS_FLOAT    (2 << 8)                           // Value is a real_ting point number
#define IS_INT32    (3 << 8)                           // Value is a 64 bit int
#define IS_SECRET   (4 << 8)                           // Value is a string but hidden
#define IS_TEXT     (5 << 8)                           // Value is a string
#define IS_MFS      (6 << 8)                           // Value is a multifunction switch
#define IS_TEXT_1   (7 << 8)                           // Used only on first connection
#define IS_VOID     (8 << 8)                           // Value is a void
#define IS_TIME     (9 << 8)                           // Value is time
#define IS_ARRAY    (1 << (8 + 4))                     // Value is part of an array
#define IS_FIRST    (999)                              // Reset the pointers
#define FLOAT_SCALE 1000.0                             // Floats are stored as 1000x integer

#define IS_MASK     (IS_VOID | IS_TEXT | IS_SECRET | IS_INT32 | IS_FLOAT | IS_FIXED | IS_MFS | IS_TIME)
#define real_t_MASK ((~IS_MASK) & 0xFF)                // Scaling factor 8 bits

#define SSID_SIZE          31                          // Reserve 30+1 bytes for SSID
#define PWD_SIZE           63                          // Reserve 63+1 bytes for Password
#define URL_SIZE           128                         // Reserve 129 bytes for server URL
#define KEY_SIZE           31                          // Key size for remote access
#define SMALL_STRING       32                          // Small strings are 32 bytes long
#define LARGE_STRING       128                         // Large strings are 128 bytes long
#define EXTRA_LARGE_STRING 1024                        // Really long string for files
#define IP_SIZE            sizeof("192.168.100.100\0") // Reserved space of IP address
#define JSON_NAME_TEXT     99                          // Name ID = User defined
#define JSON_NAME_CLIENT   100                         // Name ID = Client defined
#define JSON_NAME_SN       101                         // Name ID = Serial Number

/*
 * Global JSON variables and settings
 */


EXTERN int           json_name_id;                   // Name Identifier
EXTERN int           json_serial_number;             // EIN

EXTERN int           json_keep_alive;                // Keepalive period
EXTERN int           json_wifi_channel;              // Channel assigned to this SSID
EXTERN int           json_wifi_dhcp;                 // TRUE if the DHCP server is enabled
EXTERN char          json_wifi_static_ip[IP_SIZE];   // Static IP assigned to the target
EXTERN char          json_wifi_gateway[IP_SIZE];     // Text of WiFI gateway mask
EXTERN char          json_wifi_ssid[SSID_SIZE];      // Text of WiFI SSID
EXTERN char          json_wifi_pwd[PWD_SIZE];        // Text of WiFI password
EXTERN char          json_wifi_server_url[URL_SIZE]; // Remote Server URL
EXTERN char          json_wifi_server_key[IP_SIZE];  // Remote Server key
EXTERN int           json_wifi_hidden;               // Hide the SSID if enabled
EXTERN char          json_remote_url[URL_SIZE];      // Where are the messages going?
EXTERN int           json_remote_active[URL_SIZE];   // Is there a remote present
EXTERN char          json_remote_key[URL_SIZE];      // Security key if nessary
EXTERN int           json_remote_modes;              // What modes are available to talk to a remote server
#endif
