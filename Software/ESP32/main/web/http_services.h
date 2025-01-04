/*----------------------------------------------------------------
 *
 * http_services.h
 *
 * HTTP services supported by the target
 *
 *---------------------------------------------------------------*/
#ifndef _HTTP_SERVICES_H_
#define _HTTP_SERVICES_H_

/*
 * Global functions
 */
void register_services(httpd_handle_t server); // Pointer to active server

/*
 * #defines
 */
extern esp_err_t service_get_target(httpd_req_t *req);   // Respond to a request for a target display
extern esp_err_t service_get_issf_png(httpd_req_t *req); // Respond to a request for a target display
extern esp_err_t service_get_post(httpd_req_t *req);
extern esp_err_t service_post_post(httpd_req_t *req);    // Sample POST handler
extern esp_err_t service_get_post2(httpd_req_t *req);
extern esp_err_t service_post_post2(httpd_req_t *req);   // Sample POST handler
#endif
