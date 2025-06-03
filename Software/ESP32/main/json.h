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
#define IS_FIXED  (1 << 8)                           // The value cannot be changed
#define IS_FLOAT  (2 << 8)                           // Value is a floating point number
#define IS_INT32  (3 << 8)                           // Value is a 64 bit int
#define IS_SECRET (4 << 8)                           // Value is a string but hidden
#define IS_TEXT   (5 << 8)                           // Value is a string
#define IS_MFS    (6 << 8)                           // Value is a multifunction switch
#define IS_TEXT_1 (7 << 8)                           // Used only on first connection
#define IS_VOID   (8 << 8)                           // Value is a void

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
extern int    json_dip_switch;               // DIP switch overwritten by JSON message
extern double json_sensor_dia;               // Sensor radius overwitten by JSON message
extern int    json_sensor_angle;             // Angle sensors are rotated through
extern int    json_paper_time;               // Time to turn on paper backer motor
extern int    json_echo;                     // Value to ech
extern int    json_calibre_x10;              // Pellet Calibre
extern int    json_north_x;                  // North Adjustment
extern int    json_north_y;
extern int    json_east_x;                   // East Adjustment
extern int    json_east_y;
extern int    json_south_x;                  // South Adjustment
extern int    json_south_y;
extern int    json_west_x;                   // WestAdjustment
extern int    json_west_y;
extern int    json_spare_1;                  // Not used
extern int    json_name_id;                  // Name Identifier
extern int    json_LED_PWM;                  // PWM Setting (%)
extern int    json_power_save;               // How long to run target before turning off LEDs
extern int    json_send_miss;                // Sent the miss message when TRUE
extern int    json_serial_number;            // EIN
extern int    json_step_count;               // Number of times paper motor is stepped
extern int    json_step_ramp;                // Time interval between ramp cycles
extern int    json_step_start;               // Starting ramp inteval
extern int    json_step_time;                // Duration of step pulse
extern int    json_multifunction;            // Multifunction switch operation
extern double json_x_offset;                 // Offset added to horizontal to centre target in sensors
extern double json_y_offset;                 // Offset added to vertical to centre targetin sensors
extern int    json_z_offset;                 // Distance between paper and sensor plane (1mm / LSB)
extern int    json_paper_eco;                // Do not advance witness paper if shot is greater than json_paper_eco
extern int    json_target_type;              // Modify the location based on a target type (0 == regular 1 bull target)
#define FIVE_BULL_AIR_RIFLE_74 1             // Target is a five bull air rifle target 74mm centres
#define FIVE_BULL_AIR_RIFLE_79 2             // Target is a five bull air rifle target 79mm centres
#define TWELVE_BULL_AIR_RIFLE  3             // Target is a twelve bull air rifle target
extern int           json_tabata_enable;     // Enable the Tabata timer
extern int           json_tabata_on;         // Tabata ON timer
extern int           json_tabata_rest;       // Tabata OFF timer
extern int           json_tabata_warn_on;    // Time to turn on the warning
extern int           json_tabata_warn_off;   // Time to go dark until we start
extern int           json_rapid_enable;      // Rapid Fire enabled
extern unsigned long json_rapid_on;          // Rapid Fire ON timer
extern int           json_rapid_count;       // Number of expected shots
extern int           json_vset_PWM;          // Voltage PWM count
extern double        json_vset;              // Desired voltage setpont
extern int           json_follow_through;    // Follow through timer
extern int           json_keep_alive;        // Keepalive period
extern int           json_face_strike;       // Number of cycles to accept a face strike
extern int           json_rapid_time;        // When will the rapid fire event end
extern int           json_wifi_channel;      // Channel assigned to this SSID
extern int           json_rapid_wait;        // Delay applied to rapid fire
extern int           json_wifi_dhcp;         // TRUE if the DHCP server is enabled
extern char          json_wifi_static_ip[];  // Static IP assigned to the target
extern char          json_wifi_gateway[];    // Text of WiFI gateway mask
extern char          json_wifi_ssid[];       // Text of WiFI SSID
extern char          json_wifi_pwd[];        // Text of WiFI password
extern char          json_wifi_server_url[]; // Remote Server URL
extern char          json_wifi_server_key[]; // Remote Server key
extern int           json_wifi_hidden;       // Hide the SSID if enabled
extern int           json_min_ring_time;     // Time to wait for ringing to stop
extern int           json_token;             // Token ring setting
extern int           json_multifunction2;    // Multifunction Switch 2
extern double        json_vref_lo;           // Sensor Voltage Reference Low (V)
extern double        json_vref_hi;           // Sensor Voltage Reference High (V)
extern int           json_pcnt_latency;      // pcnt interrupt latancy
extern int           json_mfs_hold_12;       // Hold A and B
extern int           json_mfs_tap_2;         // Tap B
extern int           json_mfs_tap_1;         // Tap A
extern int           json_mfs_hold_2;        // Hold B
extern int           json_mfs_hold_1;        // Hold A
extern int           json_mfs_hold_d;        // Hold D
extern int           json_mfs_hold_c;        // Hold C
extern int           json_mfs_select_cd;     // Select C and D operation
extern int           json_wifi_reset_first;  // Reset the target on first WiFi connection
extern int           json_paper_shot;        // How many shots before advancing paper
extern int           json_aux_mode;          // Enable comms from the AUX port
extern char          json_remote_url[];      // Where are the messages going?
extern char          json_athlete[];         // Athelete shooting
extern char          json_event[];           // Event being shot
extern char          json_target_name[];     // Target being shot at
extern char          json_name_text[];       // Target name, ex (Target 54))
extern int           json_remote_modes;      // What modes are available to talk to a remote server
extern int           json_session_type;      // What kind of session is this?
extern char          json_ota_url[];         // OTA URL
#endif
