/*----------------------------------------------------------------
 *
 * nonvol.h
 *
 * Header file for persistent storage functions
 *
 *---------------------------------------------------------------*/
#ifndef _NONVOL_H
#define _NONVOL_H

#define PS_VERSION   0                                    // Persistent storage version
#define PS_UNINIT(x) (((x) == 0xABAB) || ((x) == 0xFFFF)) // Uninitilized value

#define NAME_SPACE "freETarget"

extern nvs_handle_t my_handle;                            // Handle to NVS space

/*
 * @function prototypes
 */
void factory_nonvol(bool new_serial_number);      // Factory reset nonvol
void init_nonvol(int v);                          // Reset to defaults
void read_nonvol(void);                           // Read in the locations
void update_nonvol(unsigned int current_version); // Update the database if needed
void nonvol_write_i32(char *name, int *value);    // Write a value to nonvol

/*
 * NON Vol Storage
 */
#define NONVOL_INIT                "NONVOL_INIT"         // Show when the INIT is done
#define NONVOL_NAME_ID             "NAME_ID"             // Name Identifier
#define NONVOL_SERIAL_NO           "SERIAL_NO"           // EIN
#define NONVOL_PS_VERSION          "PS_VERSION"          // Persistent storage version
#define NONVOL_KEEP_ALIVE          "KEEP_ALIVE"          // Send out a keep alive at a r
#define NONVOL_FACE_STRIKE         "FACE_STRIKE"         // Number of cycles to accept a face strike
#define NONVOL_MIN_RING_TIME       "MIN_RING_TIME"       // Minimum time for ringing to stop
#define NONVOL_TOKEN               "TOKEN"               // Token ring state
#define NONVOL_VREF_LO             "VREF_LO"             // Sensor Reference Voltage low in V
#define NONVOL_VREF_HI             "VREF_HI"             // Sensor Reference Voltage high in V
#define NONVOL_WIFI_CHANNEL        "WIFI_CHANNEL"        // Channel to use for WiFI
#define NONVOL_WIFI_DHCP           "WIFI_DHCP"           //
#define NONVOL_WIFI_SSID           "WIFI_SSID"           // Storage for SSID
#define NONVOL_WIFI_PWD            "WIFI_PWD"            // Storage for SSID Password
#define NONVOL_WIFI_IP             "WIFI_IP"             // Storage forIP Address
#define NONVOL_WIFI_GATEWAY        "WIFI_GATEWAY"        // Storage for Gateway mask
#define NONVOL_WIFI_RESET_FIRST    "WIFI_RESET"          // Reset the target on the first WiFi connction
#define NONVOL_X_OFFSET            "X_OFFSET"            // Offset added to sensors to adjust centre horizontally
#define NONVOL_Y_OFFSET            "Y_OFFSET"            // Offset added to sensors to adjust centre vertically
#define NONVOL_Z_OFFSET            "Z_OFFSET"            // Distance from sensor plane to paper plane
#define NONVOL_MFS_HOLD_AB         "MFS_HOLD_AB"         // Action to take place when A & B are held
#define NONVOL_MFS_TAP_B           "MFS_TAP_B"           // Action to take place when B is tapped
#define NONVOL_MFS_TAP_A           "MFS_TAP_A"           // Action to take place when A is tapped
#define NONVOL_MFS_HOLD_B          "MFS_HOLD_B"          // Action to take place when B is held
#define NONVOL_MFS_HOLD_A          "MFS_HOLD_A"          // Action to take place when A is held
#define NONVOL_MFS_HOLD_D          "MFS_HOLD_D"          // Action to take place when D is held
#define NONVOL_MFS_HOLD_C          "MFS_HOLD_C"          // Action to take place when C is held
#define NONVOL_MFS_SELECT_CD       "MFS_SELECT_CD"       // Hardware attached to CD
#define NONVOL_WIFI_HIDDEN         "WIFI_HIDDEN"         // Hide the SSID if set to 1
#define NONVOL_NAME_TEXT           "NAME_TEXT"           // User supplied name for the target
#define NONVOL_OTA_URL             "OTA_URL"             // User supplied name for the target
#define NONVOL_REMOTE_ACTIVE       "REMOTE_ACTIVE"       // Send score to a remote server
#define NONVOL_REMOTE_URL          "REMOTE_URL"          // URL of the remote server
#define NONVOL_REMOTE_KEY          "REMOTE_KEY"          // Remote server access key
#define NONVOL_ATHELETE            "ATHELETE"            // Remember the athelete name
#define NONVOL_EVENT               "EVENT"               // Remember the shooting event
#define NONVOL_LOCK                "LOCK"                // Password for the target
#define NONVOL_RADIUS_ADJUST       "RADIUS_ADJUST"       // Compensate for the diameter of the target
#define NONVOL_CALIBRATION_DATA    "CD"                  // Calibration data block
#define NONVOL_SENSOR_ANGLE_OFFSET "SENSOR_ANGLE_OFFSET" // Correction to sensor angle
#define NONVOL_TABATA_WARN_ON      "TABATA_WARN_ON"      // Tabata warning on time
#define NONVOL_TABATA_REST         "TABATA_REST"         // Tabata rest time
#define NONVOL_TABATA_ON           "TABATA_ON"           // Tabata on time
#endif
