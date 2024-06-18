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
void reset_JSON(void);              // Clear the JSON input buffer
void freeETarget_json(void*);       // Task to scan the serial port looking for JSON input
void show_echo(unsigned int level); // Display the settings

/* 
 * JSON message typedefs
 */
typedef struct  {
  int               level;    // Display level
  char*             token;    // JSON token string, ex "RADIUS": 
  int*              value;    // Where value is stored 
  double*         d_value;    // Where value is stored 
  int             convert;    // Conversion type
  void        (*f)(int x);    // Function to execute with message
  char*           non_vol;    // Storage in NON-VOL
  int          init_value;    // Initial Value
} json_message_t;

extern const json_message_t JSON[];

/*
 * Definitioins
 */
#define IS_VOID       (7<<8)   // Value is a void
#define IS_MFS        (6<<8)   // Value is a multifunction switch
#define IS_TEXT       (5<<8)   // Value is a string
#define IS_SECRET     (4<<8)   // Value is a string but hidden
#define IS_INT32      (3<<8)   // Value is a 64 bit int
#define IS_FLOAT      (2<<8)   // Value is a floating point number
#define IS_FIXED      (1<<8)   // The value cannot be changed
#define IS_MASK       (IS_VOID | IS_TEXT | IS_SECRET | IS_INT32 | IS_FLOAT | IS_FIXED | IS_MFS)
#define FLOAT_MASK    ((~IS_MASK) & 0xFF)    // Scaling factor 8 bits

#define SSID_SIZE     31        // Reserve 30+1 bytes for SSID
#define PWD_SIZE      31        // Reserve 30+1 bytes for Password
#define URL_SIZE      33        // Reserve 32+1 bytes for remote URL
#define SMALL_STRING  63        // Reserver 64 bytes for a short string

#define EA             0        // Echo everything
#define EX             0        // Echo Never
#define ES             1        // Echo the things we most care about
#define EN             2        // Echo network settings 
#define EC             3        // Echo the things that affect calculations 
#define EO             4        // Echo operational settings
#define EI             9        // Echo information settings 

/*
 * Global JSON variables and settings
 */
extern int    json_dip_switch;    // DIP switch overwritten by JSON message
extern double json_sensor_dia;    // Sensor radius overwitten by JSON message
extern int    json_sensor_angle;  // Angle sensors are rotated through
extern int    json_paper_time;    // Time to turn on paper backer motor
extern int    json_echo;          // Value to ech
extern int    json_calibre_x10;   // Pellet Calibre
extern int    json_north_x;       // North Adjustment
extern int    json_north_y;
extern int    json_east_x;        // East Adjustment
extern int    json_east_y;
extern int    json_south_x;       // South Adjustment
extern int    json_south_y;
extern int    json_west_x;        // WestAdjustment
extern int    json_west_y;
extern int    json_spare_1;       // Not used
extern int    json_name_id;       // Name Identifier
extern int    json_LED_PWM;       // PWM Setting (%)
extern int    json_power_save;    // How long to run target before turning off LEDs
extern int    json_send_miss;     // Sent the miss message when TRUE
extern int    json_serial_number; // EIN 
extern int    json_step_count;    // Number of times paper motor is stepped
extern int    json_step_time;     // Duration of step pulse
extern int    json_multifunction; // Multifunction switch operation
extern int    json_z_offset;      // Distance between paper and sensor plane (1mm / LSB)
extern int    json_paper_eco;     // Do not advance witness paper if shot is greater than json_paper_eco
extern int    json_target_type;   // Modify the location based on a target type (0 == regular 1 bull target)
#define FIVE_BULL_AIR_RIFLE_74 1  // Target is a five bull air rifle target 74mm centres
#define FIVE_BULL_AIR_RIFLE_79 2  // Target is a five bull air rifle target 79mm centres
#define TWELVE_BULL_AIR_RIFLE  3  // Target is a twelve bull air rifle target
extern int    json_tabata_enable; // Enable the Tabata timer
extern int    json_tabata_on;     // Tabata ON timer
extern int    json_tabata_rest;   // Tabata OFF timer
extern int    json_tabata_warn_on;  // Time to turn on the warning
extern int    json_tabata_warn_off; // Time to go dark until we start
extern int    json_rapid_enable;  // Rapid Fire enabled
extern unsigned long   json_rapid_on; // Rapid Fire ON timer
extern int    json_rapid_count;   // Number of expected shots
extern int    json_vset_PWM;      // Voltage PWM count
extern double json_vset;          // Desired voltage setpont
extern int    json_follow_through;// Follow through timer
extern int    json_keep_alive;    // Keepalive period
extern int    json_face_strike;   // Number of cycles to accept a face strike
extern int    json_rapid_time;    // When will the rapid fire event end
extern int    json_rapid_count;   // Number of allowable shots
extern int    json_wifi_channel;  // Channel assigned to this SSID
extern int    json_rapid_wait;    // Delay applied to rapid fire
extern int    json_wifi_dhcp;     // TRUE if the DHCP server is enabled
extern char   json_wifi_ssid[];   // Text of WiFI SSID
extern char   json_wifi_pwd[];    // Text of WiFI password
extern char   json_wifi_ip[];     // Text of IP address
extern int    json_min_ring_time; // Time to wait for ringing to stop
extern int    json_token;         // Token ring setting
extern int    json_multifunction2;// Multifunction Switch 2
extern double json_vref_lo;       // Sensor Voltage Reference Low (V)
extern double json_vref_hi;       // Sensor Voltage Reference High (V)
extern int    json_pcnt_latency;  // pcnt interrupt latancy
extern int    json_mfs_hold_ab;   // Hold A and B
extern int    json_mfs_tap_b;     // Tap B
extern int    json_mfs_tap_a;     // Tap A
extern int    json_mfs_hold_b;    // Hold B
extern int    json_mfs_hold_a;    // Hold A
extern int    json_mfs_hold_d;    // Hold D
extern int    json_mfs_hold_c;    // Hold C
extern int    json_mfs_select_cd; // Select C and D operation
extern char   json_remote_url[];  // Text string of remote URL ex "http://google.com"
extern int    json_remote_active; // Set to 1 if the remote server is used to display score 
extern char   json_athlete[SMALL_STRING];    // Shooter name
extern char   json_event[SMALL_STRING];       // Shooting event
extern char   json_target_name[SMALL_STRING]; // Target name
#endif
