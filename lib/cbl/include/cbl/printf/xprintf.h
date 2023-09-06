/*------------------------------------------------------------------------*/
/* Universal string handler for user console interface  (C)ChaN, 2011     */
/*------------------------------------------------------------------------*/

#include "stdarg.h"

#ifndef _STRFUNC
#    define _STRFUNC

#    define _USE_XFUNC_OUT 1 /* 1: Use output functions */
#    define _CR_CRLF 1       /* 1: Convert \n ==> \r\n in the output char */

#    define _USE_XFUNC_IN 0 /* 1: Use input function */
#    define _LINE_ECHO 0    /* 1: Echo back input chars in xgets function */

#    ifdef __cplusplus
extern "C"
{
#    endif

#    if _USE_XFUNC_OUT
#        define xdev_out(func) xfunc_out = func
extern void (*xfunc_out)(unsigned char);

#        ifndef LIB_TYPE_HOSTED
int putc(char c);
int putchar(int c);
int puts(const char* str);

int printf(const char* fmt, ...);
int vprintf(const char* fmt, va_list arp);
int sprintf(char* buff, const char* fmt, ...);
int snprintf(char* buff, unsigned long n, const char* fmt, ...);
int vsnprintf(char* buff, unsigned long n, const char* fmt, va_list arp);

#            define DW_CHAR sizeof(char)
#            define DW_SHORT sizeof(short)
#            define DW_LONG sizeof(long)
#        endif

#    endif // ifndef LIB_TYPE_HOSTED

#    ifdef __cplusplus
}
#    endif

#endif
