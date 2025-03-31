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

void OTA_rollback(void); // Go back to the previous version of software
void OTA_load(void);     // Get a new version of software from the server

/*
 * #defines
 */

#endif
