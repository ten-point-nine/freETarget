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
#define NONVOL_DISTANCE_TO_TARGET "DISTANCE_TO_TARGET" // Distance to target in meters
#define NONVOL_MUZZLE_VELOCITY    "MUZZLE_VELOCITY"    // Pellet speed
#define NONVOL_TRACE_SIZE         "TRACE_SIZE"         // Exclusion radius of the target in mm.
#define NONVOL_INIT               "NONVOL_INIT"        // Show when the INIT is done
#define NONVOL_SERIAL_NO          "SERIAL_NO"          // EIN
#define NONVOL_PS_VERSION         "PS_VERSION"         // Persistent storage version
#define NONVOL_KEEP_ALIVE         "KEEP_ALIVE"         // Send out a keep alive at a r
#define NONVOL_WIFI_SSID          "WIFI_SSID"          // Storage for SSID
#define NONVOL_WIFI_PWD           "WIFI_PWD"           // Storage for SSID Password
#define NONVOL_WIFI_GATEWAY       "WIFI_GATEWAY_IP"    // Gateway
#define NONVOL_WIFI_STATIC_IP     "WIFI_STATIC_IP"     // Storage for IP Address
#define NONVOL_WIFI_REMOTE_IP     "WIFI_REMOTE_IP"     // Address of the target server
#define NONVOL_WIFI_RESET_FIRST   "WIFI_RESET"         // Reset the target on the first WiFi connction
#define NONVOL_WIFI_HIDDEN        "WIFI_HIDDEN"        // Hide the SSID if set to 1
#define NONVOL_OTA_URL            "OTA_URL"            // User supplied name for the target
#define NONVOL_REMOTE_ACTIVE      "REMOTE_ACTIVE"      // Send score to a remote server
#define NONVOL_REMOTE_URL         "REMOTE_URL"         // URL of the remote server
#define NONVOL_REMOTE_KEY         "REMOTE_KEY"         // Remote server access key
#define NONVOL_ATHELETE           "ATHELETE"           // Remember the athelete name
#define NONVOL_EVENT              "EVENT"              // Remember the shooting event
#define NONVOL_LOCK               "LOCK"               // Password for the target
#define NONVOL_X_DOTDOT_OFFSET    "X_DOTDOT_OFFSET"    // Correction for X acceleration
#define NONVOL_Y_DOTDOT_OFFSET    "Y_DOTDOT_OFFSET"    // Correction for Y acceleration
#define NONVOL_Z_DOTDOT_OFFSET    "Z_DOTDOT_OFFSET"    // Correction for Z acceleration
#define NONVOL_RHO_DOT_OFFSET     "RHO_DOT_OFFSET"     // Correction for angular velocity
#define NONVOL_THETA_DOT_OFFSET   "THETA_DOT_OFFSET"   // Correction for angular velocity
#define NONVOL_PHI_DOT_OFFSET     "PHI_DOT_OFFSET"     // Correction for angular velocity

#endif
