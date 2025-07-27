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
 *---------------------------------------------------------------*/
#ifndef _HTML_H_
#define _HTML_H_

extern const unsigned char FreeETarget_html_start[] asm("_binary_FreeETarget_html_start");
extern const unsigned char FreeETarget_html_end[] asm("_binary_FreeETarget_html_end");
#define SIZEOF_FreeETarget_HTML (FreeETarget_html_end - FreeETarget_html_start)

extern const unsigned char FreeETarget_png_start[] asm("_binary_FreeETarget_png_start");
extern const unsigned char FreeETarget_png_end[] asm("_binary_FreeETarget_png_end");
#define SIZEOF_FREEETARGET_PNG (FreeETarget_png_end - FreeETarget_png_start)

extern const unsigned char help_html_start[] asm("_binary_help_html_start");
extern const unsigned char help_html_end[] asm("_binary_help_html_end");
#define SIZEOF_help_HTML (help_html_end - help_html_start)

extern const unsigned char menu_html_start[] asm("_binary_menu_html_start");
extern const unsigned char menu_html_end[] asm("_binary_menu_html_end");
#define SIZEOF_menu_HTML (menu_html_end - menu_html_start)

#endif
