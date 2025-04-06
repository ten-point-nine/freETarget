/*----------------------------------------------------------------
 *
 * OTA.h
 *
 * Over The Air Header file
 *
 *---------------------------------------------------------------*/
#ifndef _OTA_H_
#define _OTA_H_

/*
 * Global functions
 */
void OTA_rollback(void);   // Go back to the previous version of software
void OTA_load(void);       // Get a new version of software from the server
void OTA_start(void);      // Start loading but check the status first
void OTA_partitions(void); // Display the partition data
void OTA_init(void);       // Initialize the OTA process

/*
 * #defines
 */
#define OTA_URL     "targetbin.app/ota"
#define OTA_TIMEOUT 30 * 1000
#endif
