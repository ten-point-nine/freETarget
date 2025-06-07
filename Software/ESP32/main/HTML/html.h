/*----------------------------------------------------------------
 *
 * html.h
 *
 * Header file to link up stuff in the EMBED_FILES section
 *
 *----------------------------------------------------------------
 *
 * Remember to add the files to the CMakeLists.txt file
 * in the EMBED_FILES section.
 *
 */
#ifndef _HTML_H_
#define _HTML_H_

extern const unsigned char FreeETarget_html[] asm("_binary_FreeETarget_html_start");
extern const unsigned char FreeETarget_end[] asm("_binary_FreeETarget_html_end");
#define SIZEOF_FreeETarget_HTML (FreeETarget_end - FreeETarget_html)

extern const unsigned char issf_png[] asm("_binary_issf_png_start");
extern const unsigned char issf_end[] asm("_binary_issf_png_end");
#define SIZEOF_ISSF_PNG (issf_end - issf_png)

extern const unsigned char help_html[] asm("_binary_help_html_start");
extern const unsigned char help_end[] asm("_binary_help_html_end");
#define SIZEOF_HELP_HTML (help_end - help_html)

extern const unsigned char menu_html[] asm("_binary_menu_html_start");
extern const unsigned char menu_end[] asm("_binary_menu_html_end");
#define SIZEOF_MENU_HTML (help_end - help_html)
#endif
