/*----------------------------------------------------------------
 *
 * html.h
 *
 * Header file to link up stuff in the EMBED_FILES section
 *
 *----------------------------------------------------------------
 *
 *
 */
#ifndef _HTML_H_
#define _HTML_H_

extern const unsigned char index_html[] asm("_binary_index_html_start");
extern const unsigned char index_end[] asm("_binary_index_html_end");
#define SIZEOF_INDEX_HTML (index_end - index_html)

extern const unsigned char issf_png[] asm("_binary_issf_png_start");
extern const unsigned char issf_end[] asm("_binary_issf_png_end");
#define SIZEOF_ISSF_PNG (issf_end - issf_png)

extern const unsigned char post_test_html[] asm("_binary_post_test_html_start");
extern const unsigned char post_test_end[] asm("_binary_post_test_html_end");
#define SIZEOF_POST_TEST_HTML (post_test_end - pot_test_html)

#endif
