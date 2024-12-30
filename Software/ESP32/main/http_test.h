/*----------------------------------------------------------------
 *
 * http_test.h
 *
 * Header file for HTTP test functions
 *
 *---------------------------------------------------------------*/
#ifndef _HTTP_TEST_H_
#define _HTTP_TEST_H_

/*
 * Global functions
 */
#if ( BUILD_HTTP || BUILD_HTTPS || BUILD_SIMPLE )
void http_DNS_test(void); // Exercise the DNS lookup
#endif

/*
 * #defines
 */

#endif
