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
#define OTA_URL          "free-e-target.com/htdocs/wp-content/uploads/OTA"
#define OTA_TEST         {"OTA_URL" : "http://free-e-target.com/htdocs/wp-content/uploads/OTA"}
#define OTA_TEST1        {"OTA_URL" : "http://targetbin.app/OTA"}
#define OTA_TEST2        {"TRACE" : 256, "TEST" : 84}
#define OTA_TIMEOUT      30 * 1000
#define OTA_FETCH_HEADER false // Fetch the header data from the server
#endif
