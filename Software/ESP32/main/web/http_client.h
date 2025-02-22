/*----------------------------------------------------------------
 *
 * http_client.h
 *
 * HTTP Client Interface File
 *
 *----------------------------------------------------------------
 *
 * This is the interface and configuration file for HTTP/HTTPS
 * client requests to a server
 *
 * See
 * https://github.com/espressif/esp-idf/blob/8760e6d2a7e19913bc40675dd71f374bcd51b0ae/examples/protocols/esp_http_client/main/esp_http_client_example.c
 *
 */
#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

/*
 *  Definitions
 */
#define BUILD_HTTP   1        // Build HTTP transfers
#define BUILD_SIMPLE 1        // Build for simple HTTP transfers
#define BUILD_HTTPS  0        // BUild HTTPS transfers

#define INCLUDE_GET     0     // Include GET methods
#define INCLUDE_POST    1     // Include POST methods
#define INCLUDE_PUT     0     // Include PUT methods
#define INCLUDE_PATCH   0     // Include PATCH methods
#define INCLUDE_DELETE  0     // Include DELETE methods
#define INCLUDE_HEAD    0     // Include HEAD methods
#define INCLUDE_API_KEY 1     // Include an API key
#define API_KEY         "cpe-1704-tks"

#define METHOD_GET    0       // enum used to identify a get method
#define METHOD_POST   1       // enum used to identify a post method
#define METHOD_PUT    2       // enum used to identify a put method
#define METHOD_PATCH  3       // enum used to identify a patch method
#define METHOD_DELETE 4       // enum used to identify a delete method
#define METHOD_HEAD   5       // enum used to identify a head method

#define DNS_TRIES 100         // Try for 10 seconds to get a DNS

#define REMOTE_MODE_NONE    0 // No remote mode supported
#define REMOTE_MODE_WEBPAGE 1 // The target sends a web page
#define REMOTE_MODE_CLIENT  2 // The target sends JSON to a server

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
#if ( BUILD_HTTPS )
extern const char howsmyssl_com_root_cert_pem_start[];
extern const char howsmyssl_com_root_cert_pem_end[];

extern const char postman_root_cert_pem_start[];
extern const char postman_root_cert_pem_end[];
#endif

void http_client_init(void); // Initialize the HTTP/HTTPS stack

#if BUILD_HTTP
// esp_err_t _http_event_handler(esp_http_client_event_t *evt);    // HTTP Event Handler
void http_rest_with_url(char *url, int method, char *payload); // Send the payload to the URL
#endif

#if BUILD_HTTPS
void https_with_url(char *url, int method, char *payload);     // HTTPS transfer to URL
void https_async(char *url, char *payload);
#endif

#if ( 0 )
void http_encoded_query(void);
void http_relative_redirect(void);
void http_absolute_redirect(void);
void http_absolute_redirect_manual(void);
#endif

void http_download_chunk(char *url, char *payload);
void http_perform_as_stream_reader(char *url, char *payload);

void http_native_request(char *url, int method, char *payload, int payload_length); // Low level API
void http_partial_download(char *url, char *range_string);

#endif
