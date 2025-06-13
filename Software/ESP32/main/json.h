/*----------------------------------------------------------------
 *
 * json.h
 *
 * Header file for JSON functions and called routines
 *
 *---------------------------------------------------------------*/
#ifndef _JSON_H_
#define _JSON_H_

#include "freETarget.h"
/*
 * Public Functions
 */
void reset_JSON(void);         // Clear the JSON input buffer
void freeETarget_json(void *); // Task to scan the serial port looking for JSON input
void show_echo(void);          // Display the settings

/*
 * JSON message typedefs
 */
typedef struct
{
  int   show;       // Display attributes
  char *token;      // JSON token string, ex "RADIUS":
  int  *value;      // Where value is stored (cast to (float*) if needed)
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
#endif

#define SHOW 0x01                                  // Show the value
#define HIDE 0x00                                  // Hide the value
#define LOCK 0x02                                  // The value is a secret

#define IS_FIXED  (1 << 8)                         // The value cannot be changed
#define IS_FLOAT  (2 << 8)                         // Value is a floating point number
#define IS_INT32  (3 << 8)                         // Value is a 64 bit int
#define IS_SECRET (4 << 8)                         // Value is a string but hidden
#define IS_TEXT   (5 << 8)                         // Value is a string
#define IS_MFS    (6 << 8)                         // Value is a multifunction switch
#define IS_TEXT_1 (7 << 8)                         // Used only on first connection
#define IS_VOID   (8 << 8)                         // Value is a void

#define IS_MASK    (IS_VOID | IS_TEXT | IS_SECRET | IS_INT32 | IS_FLOAT | IS_FIXED | IS_MFS)
#define FLOAT_MASK ((~IS_MASK) & 0xFF)               // Scaling factor 8 bits

#define SSID_SIZE        31                          // Reserve 30+1 bytes for SSID
#define PWD_SIZE         31                          // Reserve 30+1 bytes for Password
#define URL_SIZE         128                         // Reserve 129 bytes for server URL
#define KEY_SIZE         31                          // Key size for remote access
#define SMALL_STRING     32                          // Small strings are 32 bytes long
#define LARGE_STRING     128                         // Large strings are 128 bytes long
#define IP_SIZE          sizeof("192.168.100.100\0") // Reserved space of IP address
#define JSON_NAME_TEXT   99                          // Name ID = User defined
#define JSON_NAME_CLIENT 100                         // Name ID = Client defined

/*
 * Global JSON variables and settings
 */

EXTERN int           json_is_locked;                 // Set to TRUE if the JSON is locked
EXTERN int           json_aux_mode;                  // Enable comms from the AUX port
EXTERN int           json_calibre_x10;               // Pellet Calibre
EXTERN int           json_dip_switch;                // DIP switch overwritten by JSON message
EXTERN int           json_dip_switch;                // DIP switch overwritten by JSON message
EXTERN double        json_sensor_dia;                // Sensor radius overwitten by JSON message
EXTERN int           json_sensor_angle;              // Angle sensors are rotated through
EXTERN int           json_paper_time;                // Time to turn on paper backer motor
EXTERN int           json_echo;                      // Value to ech
EXTERN int           json_calibre_x10;               // Pellet Calibre
EXTERN int           json_north_x;                   // North Adjustment
EXTERN int           json_north_y;
EXTERN int           json_east_x;                    // East Adjustment
EXTERN int           json_east_y;
EXTERN int           json_south_x;                   // South Adjustment
EXTERN int           json_south_y;
EXTERN int           json_west_x;                    // WestAdjustment
EXTERN int           json_west_y;
EXTERN int           json_spare_1;                   // Not used
EXTERN int           json_name_id;                   // Name Identifier
EXTERN int           json_LED_PWM;                   // PWM Setting (%)
EXTERN int           json_power_save;                // How long to run target before turning off LEDs
EXTERN int           json_send_miss;                 // Sent the miss message when TRUE
EXTERN int           json_serial_number;             // EIN
EXTERN int           json_step_count;                // Number of times paper motor is stepped
EXTERN int           json_step_ramp;                 // Time interval between ramp cycles
EXTERN int           json_step_start;                // Starting ramp inteval
EXTERN int           json_step_time;                 // Duration of step pulse
EXTERN int           json_multifunction;             // Multifunction switch operation
EXTERN double        json_x_offset;                  // Offset added to horizontal to centre target in sensors
EXTERN double        json_y_offset;                  // Offset added to vertical to centre targetin sensors
EXTERN int           json_z_offset;                  // Distance between paper and sensor plane (1mm / LSB)
EXTERN int           json_paper_eco;                 // Do not advance witness paper if shot is greater than json_paper_eco
EXTERN int           json_target_type;               // Modify the location based on a target type (0 == regular 1 bull target)
EXTERN int           json_tabata_enable;             // Enable the Tabata timer
EXTERN int           json_tabata_on;                 // Tabata ON timer
EXTERN int           json_tabata_rest;               // Tabata OFF timer
EXTERN int           json_tabata_warn_on;            // Time to turn on the warning
EXTERN int           json_tabata_warn_off;           // Time to go dark until we start
EXTERN int           json_rapid_enable;              // Rapid Fire enabled
EXTERN unsigned long json_rapid_on;                  // Rapid Fire ON timer
EXTERN int           json_rapid_count;               // Number of expected shots
EXTERN int           json_vset_PWM;                  // Voltage PWM count
EXTERN double        json_vset;                      // Desired voltage setpont
EXTERN int           json_follow_through;            // Follow through timer
EXTERN int           json_keep_alive;                // Keepalive period
EXTERN int           json_face_strike;               // Number of cycles to accept a face strike
EXTERN int           json_rapid_time;                // When will the rapid fire event end
EXTERN int           json_wifi_channel;              // Channel assigned to this SSID
EXTERN int           json_rapid_wait;                // Delay applied to rapid fire
EXTERN int           json_wifi_dhcp;                 // TRUE if the DHCP server is enabled
EXTERN char          json_wifi_static_ip[IP_SIZE];   // Static IP assigned to the target
EXTERN char          json_wifi_gateway[IP_SIZE];     // Text of WiFI gateway mask
EXTERN char          json_wifi_ssid[SSID_SIZE];      // Text of WiFI SSID
EXTERN char          json_wifi_pwd[PWD_SIZE];        // Text of WiFI password
EXTERN char          json_wifi_server_url[URL_SIZE]; // Remote Server URL
EXTERN char          json_wifi_server_key[IP_SIZE];  // Remote Server key
EXTERN int           json_wifi_hidden;               // Hide the SSID if enabled
EXTERN int           json_min_ring_time;             // Time to wait for ringing to stop
EXTERN int           json_token;                     // Token ring setting
EXTERN int           json_multifunction2;            // Multifunction Switch 2
EXTERN double        json_vref_lo;                   // Sensor Voltage Reference Low (V)
EXTERN double        json_vref_hi;                   // Sensor Voltage Reference High (V)
EXTERN int           json_pcnt_latency;              // pcnt interrupt latancy
EXTERN int           json_mfs_hold_12;               // Hold A and B
EXTERN int           json_mfs_tap_2;                 // Tap B
EXTERN int           json_mfs_tap_1;                 // Tap A
EXTERN int           json_mfs_hold_2;                // Hold B
EXTERN int           json_mfs_hold_1;                // Hold A
EXTERN int           json_mfs_hold_d;                // Hold D
EXTERN int           json_mfs_hold_c;                // Hold C
EXTERN int           json_mfs_select_cd;             // Select C and D operation
EXTERN int           json_wifi_reset_first;          // Reset the target on first WiFi connection
EXTERN int           json_paper_shot;                // How many shots before advancing paper
EXTERN int           json_aux_mode;                  // Enable comms from the AUX port
EXTERN char          json_remote_url[URL_SIZE];      // Where are the messages going?
EXTERN int           json_remote_active[URL_SIZE];   // Is there a remote present
EXTERN char          json_remote_key[URL_SIZE];      // Security key if nessary
EXTERN char          json_athlete[SMALL_STRING];     // Athelete shooting
EXTERN char          json_event[SMALL_STRING];       // Event being shot
EXTERN char          json_target_name[SMALL_STRING]; // Target being shot at
EXTERN char          json_name_text[SMALL_STRING];   // Target name, ex (Target 54))
EXTERN int           json_remote_modes;              // What modes are available to talk to a remote server
EXTERN int           json_session_type;              // What kind of session is this?
EXTERN char          json_ota_url[URL_SIZE];         // OTA URL
EXTERN int           json_lock;                      // Lock the JSON message so it cannot be changed
EXTERN int           json_is_locked;                 // JSON lock state
#endif
