/*----------------------------------------------------------------
 *
 * nonvol.h
 *
 * Header file for persistent storage functions
 *
 *---------------------------------------------------------------*/
#ifndef _NONVOL_H
#define _NONVOL_H

#define PS_VERSION   12                                   // Persistent storage version
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
void restore_nonvol(void);                        // Copyt the nonvol back
void nonvol_write_i32(char *name, int *value);    // Write a value to nonvol

/*
 * NON Vol Storage
 */
#define NONVOL_INIT             "NONVOL_INIT"     // Show when the INIT is done
#define NONVOL_CALIBRE_X10      "CALIBRE_X10"     // Pellet Calibre
#define NONVOL_SENSOR_DIA       "SENSOR_DIA"      // Sensor diameter
#define NONVOL_SENSOR_ANGLE     "SENSOR_ANGLE"    // Angular displacement of sensors
#define NONVOL_NORTH_X          "NORTH_X"         // Offset applied to North sensor
#define NONVOL_NORTH_Y          "NORTH_Y"
#define NONVOL_EAST_X           "EAST_X"          // Offset applied to East sensor
#define NONVOL_EAST_Y           "EAST_Y"
#define NONVOL_SOUTH_X          "SOUTH_X"         // Offset applied to South sensor
#define NONVOL_SOUTH_Y          "SOUTH_Y"
#define NONVOL_WEST_X           "WEST_X"          // Offset applied to West sensor
#define NONVOL_WEST_Y           "WEST_Y"
#define NONVOL_POWER_SAVE       "POWER_SAVE"      // Power saver time
#define NONVOL_NAME_ID          "NAME_ID"         // Name Identifier
#define NONVOL_1_RINGx10        "RINGx10"         // Size of the 1 ring in mm
#define NONVOL_LED_PWM          "LED_PWM"         // LED PWM value
#define NONVOL_SEND_MISS        "SEND_MISS"       // Send the MISS message when true
#define NONVOL_SERIAL_NO        "SERIAL_NO"       // EIN
#define NONVOL_STEP_COUNT       "STEP_COUNT"      // Number of paper pulse steps
#define NONVOL_STEP_RAMP        "STEP_RAMP"       // Amount to decreas every ramp cycle
#define NONVOL_STEP_START       "STEP_START"      // Starting ramp interval in ms
#define NONVOL_STEP_TIME        "STEP_TIME"       // Stepper motor pulse duration
#define NONVOL_MFS              "MFS"             // Multifunction switch operation
#define NONVOL_MFS2             "MFS2"            // Multifunction switch operation
#define NONVOL_PAPER_TIME       "PAPER_TIME"      // Paper advance time
#define NONVOL_PAPER_SHOT       "PAPER_SHOT"      // Number of shots before advancing paper
#define NONVOL_PAPER_ECO        "PAPER_ECO"       // Advance witness paper if the shot is less than paper_eco
#define NONVOL_TARGET_TYPE      "TARGET_TYPE"     // Modify the target processing (0 == Regular single bull)
#define NONVOL_PS_VERSION       "PS_VERSION"      // Persistent storage version
#define NONVOL_PCNT_LATENCY     "PCNT_LATENCY"    // Correction applied to PCNT readings
#define NONVOL_FOLLOW_THROUGH   "FOLLOW_THROUGH"  // Follow through timer
#define NONVOL_KEEP_ALIVE       "KEEP_ALIVE"      // Send out a keep alive at a r
#define NONVOL_FACE_STRIKE      "FACE_STRIKE"     // Number of cycles to accept a face strike
#define NONVOL_MIN_RING_TIME    "MIN_RING_TIME"   // Minimum time for ringing to stop
#define NONVOL_TOKEN            "TOKEN"           // Token ring state
#define NONVOL_VREF_LO          "VREF_LO"         // Sensor Reference Voltage low in V
#define NONVOL_VREF_HI          "VREF_HI"         // Sensor Reference Voltage high in V
#define NONVOL_WIFI_CHANNEL     "WIFI_CHANNEL"    // Channel to use for WiFI
#define NONVOL_WIFI_DHCP        "WIFI_DHCP"       //
#define NONVOL_WIFI_SSID        "WIFI_SSID"       // Storage for SSID
#define NONVOL_WIFI_PWD         "WIFI_PWD"        // Storage for SSID Password
#define NONVOL_WIFI_IP          "WIFI_IP"         // Storage forIP Address
#define NONVOL_WIFI_GATEWAY     "WIFI_GATEWAY"    // Storage for Gateway mask
#define NONVOL_WIFI_RESET_FIRST "WIFI_RESET"      // Reset the target on the first WiFi connction
#define NONVOL_X_OFFSET         "X_OFFSET"        // Offset added to sensors to adjust centre horizontally
#define NONVOL_Y_OFFSET         "Y_OFFSET"        // Offset added to sensors to adjust centre vertically
#define NONVOL_Z_OFFSET         "Z_OFFSET"        // Distance from sensor plane to paper plane
#define NONVOL_MFS_HOLD_AB      "MFS_HOLD_AB"     // Action to take place when A & B are held
#define NONVOL_MFS_TAP_B        "MFS_TAP_B"       // Action to take place when B is tapped
#define NONVOL_MFS_TAP_A        "MFS_TAP_A"       // Action to take place when A is tapped
#define NONVOL_MFS_HOLD_B       "MFS_HOLD_B"      // Action to take place when B is held
#define NONVOL_MFS_HOLD_A       "MFS_HOLD_A"      // Action to take place when A is held
#define NONVOL_MFS_HOLD_D       "MFS_HOLD_D"      // Action to take place when D is held
#define NONVOL_MFS_HOLD_C       "MFS_HOLD_C"      // Action to take place when C is held
#define NONVOL_MFS_SELECT_CD    "MFS_SELECT_CD"   // Hardware attached to CD
#define NONVOL_WIFI_HIDDEN      "WIFI_HIDDEN"     // Hide the SSID if set to 1
#define NONVOL_AUX_PORT_ENABLE  "AUX_PORT_ENABLE" // Enable comms throught the AUX port
#define NONVOL_NAME_TEXT        "NAME_TEXT"       // User supplied name for the target
#define NONVOL_OTA_URL          "OTA_URL"         // User supplied name for the target
#define NONVOL_REMOTE_ACTIVE    "REMOTE_ACTIVE"   // Send score to a remote server
#define NONVOL_REMOTE_URL       "REMOTE_URL"      // URL of the remote server
#define NONVOL_REMOTE_KEY       "REMOTE_KEY"      // Remote server access key
#define NONVOL_ATHELETE         "ATHELETE"        // Remember the athelete name
#define NONVOL_EVENT            "EVENT"           // Remember the shooting event
#define NONVOL_TARGET_NAME      "TARGET_NAME"     // Rememver the target name
#define NONVOL_LOCK             "LOCK"            // Password for the target

#endif
