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
void OTA_rollback(void);                                                  // Go back to the previous version of software
void OTA_load(void);                                                      // Perform the OTA update
void OTA_load_json(int dummy);                                            // Perform the OTA update from the JSON function
void OTA_partitions(void);                                                // Display the partition data
void OTA_compare_versions(void);                                          // Compare working against OTA versions
void OTA_get_versions(char *running_version, char *OTA_compare_versions); // Version watiting in OTA

/*
 * #defines
 */
#define OTA_URL          "http://targetbin.app/OTA"
#define OTA_TEST1        {"OTA_URL" : "http://targetbin.app/OTA"}
#define OTA_TEST2        {"TRACE" : 256, "TEST" : 84}
#define OTA_TIMEOUT      30 * 1000
#define OTA_FETCH_HEADER false // Fetch the header data from the server
#endif
