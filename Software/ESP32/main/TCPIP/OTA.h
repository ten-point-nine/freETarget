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
void OTA_load(void);       // Perform the OTA update
void OTA_partitions(void); // Display the partition data

/*
 * #defines
 */
#define OTA_URL     "targetbin.app/ota"
#define OTA_TIMEOUT 30 * 1000
#endif
