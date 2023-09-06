/*------------------------------------------------------------------------/
/  Universal string handler for user console interface
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include <compiler.h>
#include <stdint.h>

#include "cbl/printf/xprintf.h"

static char* const max_ptr = (char*) -1;

#if _USE_XFUNC_OUT
void (*xfunc_out)(unsigned char); /* Pointer to the output stream */

#    ifndef LIB_TYPE_HOSTED
/*----------------------------------------------*/
/* Put a character                              */
/*----------------------------------------------*/

// Clang UBSAN complains about incrementing *outptr when it is NULL. This is
// actually undefined behavior.
//
// See #946.
#        if defined(__clang__)
__attribute__((no_sanitize("undefined")))
#        endif
static int
putc_internal(char c, char** outptr, char* outptr_limit)
{
    if (_CR_CRLF && c == '\n')
        putc_internal('\r', outptr, outptr_limit); /* CR -> CRLF */

    if (outptr) {
        if (*outptr && *outptr < outptr_limit)
            **outptr = (unsigned char) c;
        (*outptr)++;
        return 0;
    }

    if (xfunc_out)
        xfunc_out((unsigned char) c);

    return 0;
}

int putc(char c)
{
    return putc_internal(c, 0, max_ptr);
}

int putchar(int c)
{
    return putc(c);
}

/*----------------------------------------------*/
/* Put a null-terminated string                 */
/*----------------------------------------------*/

int puts_internal(const char* str, char** outptr, char* outptr_limit)
{
    while (*str) {
        putc_internal(*str++, outptr, outptr_limit);
    }
    return 0;
}

/*------------------------------------------------------*/
/* Put a null-terminated string with a trailing newline */
/*------------------------------------------------------*/

int puts(const char* str)
{
    int ret = puts_internal(str, 0, max_ptr);
    putc_internal('\n', 0, max_ptr);
    return ret;
}

/*----------------------------------------------*/
/* Formatted string output                      */
/*----------------------------------------------*/
/*  xprintf("%d", 1234);            "1234"
    xprintf("%6d,%3d%%", -200, 5);  "  -200,  5%"
    xprintf("%-6u", 100);           "100   "
    xprintf("%ld", 12345678L);      "12345678"
    xprintf("%04x", 0xA3);          "00a3"
    xprintf("%08LX", 0x123ABC);     "00123ABC"
    xprintf("%016b", 0x550F);       "0101010100001111"
    xprintf("%s", "String");        "String"
    xprintf("%-4s", "abc");         "abc "
    xprintf("%4s", "abc");          " abc"
    xprintf("%c", 'a');             "a"
    xprintf("%f", 10.0);            <xprintf lacks floating point support>
*/

enum
{
    FLAG_ZERO_PADDING = 1u << 0,
    FLAG_LEFT_JUSTIFIED = 1u << 1,
    FLAG_SIZE_LONG = 1u << 2,
    FLAG_SIZE_LONG_LONG = 1u << 3,
    FLAG_SIGNED_NEGATIVE = 1u << 4,
    FLAG_SIZE_POINTER = 1u << 5,
};
static void xvprintf_internal(const char* fmt, va_list arp, char** outptr, char* outptr_limit)
{
    unsigned int r, i, j, w, f;
    unsigned long long v;
    char s[32], c, d, *p;

    for (;;) {
        c = *fmt++;
        if (!c) {
            // End of format
            break;
        }

        if (c != '%') {
            /* Pass through it if not a % sequence */
            putc_internal(c, outptr, outptr_limit);
            continue;
        }

        f = 0;
        c = *fmt++;

        if (c == '#') {
            puts_internal("0x", outptr, outptr_limit);
            c = *fmt++;
        }

        if (c == '0') {
            f = FLAG_ZERO_PADDING;
            c = *fmt++;
        } else if (c == '-') {
            f = FLAG_LEFT_JUSTIFIED;
            c = *fmt++;
        }

        for (w = 0; c >= '0' && c <= '9'; c = *fmt++) {
            /* Minimum width */
            w = w * 10 + c - '0';
        }

        // First long: Set long flag
        if (c == 'l' || c == 'L') {
            f |= FLAG_SIZE_LONG;
            c = *fmt++;
        }

        // Second long: Set long long flag, keep long
        if (c == 'l' || c == 'L') {
            f |= FLAG_SIZE_LONG_LONG;
            c = *fmt++;
        }

        if (!c) {
            break;
        }

        d = c;
        if (d >= 'a') {
            d -= 0x20;
        }

        switch (d) { /* Type is... */
        case 'S':    /* String */
            p = va_arg(arp, char*);
            for (j = 0; p[j]; j++)
                ;
            while (!(f & FLAG_LEFT_JUSTIFIED) && j++ < w) {
                putc_internal(' ', outptr, outptr_limit);
            }
            puts_internal(p, outptr, outptr_limit);
            while (j++ < w) {
                putc_internal(' ', outptr, outptr_limit);
            }
            continue;
        case 'C': /* Character */ putc_internal((char) va_arg(arp, int), outptr, outptr_limit); continue;
        case 'B': /* Binary */ r = 2; break;
        case 'O': /* Octal */ r = 8; break;
        case 'D': /* Signed decimal */
        case 'U': /* Unsigned decimal */ r = 10; break;
        case 'P':
            f |= FLAG_SIZE_POINTER;
            puts_internal("0x", outptr, outptr_limit);
            FALL_THROUGH;
        case 'X': /* Hexadecimal */ r = 16; break;
        default: /* Unknown type (passthrough) */ putc_internal(c, outptr, outptr_limit); continue;
        }

        /* Get an argument and put it in numeral */
        if (f & FLAG_SIZE_POINTER) {
            v = va_arg(arp, uintptr_t);
        } else if (f & FLAG_SIZE_LONG_LONG) {
            v = va_arg(arp, long long);
        } else if (f & FLAG_SIZE_LONG) {
            v = va_arg(arp, long);
        } else {
            v = (d == 'D') ? (long long) va_arg(arp, int) : (long long) va_arg(arp, unsigned int);
        }

        if (d == 'D' && (v & 0x8000000000000000)) {
            v = 0 - v;
            f |= FLAG_SIGNED_NEGATIVE;
        }

        i = 0;
        do {
            d = (char) (v % r);
            v /= r;
            if (d > 9)
                d += (c == 'x') ? 0x27 : 0x07;
            s[i++] = d + '0';
        } while (v && i < sizeof(s));

        if (f & FLAG_SIGNED_NEGATIVE) {
            s[i++] = '-';
        }

        j = i;
        d = (f & FLAG_ZERO_PADDING) ? '0' : ' ';

        while (!(f & FLAG_LEFT_JUSTIFIED) && j++ < w) {
            putc_internal(d, outptr, outptr_limit);
        }

        do {
            putc_internal(s[--i], outptr, outptr_limit);
        } while (i);

        while (j++ < w) {
            putc_internal(' ', outptr, outptr_limit);
        }
    }
}

int vprintf(const char* fmt, va_list arp)
{
    xvprintf_internal(fmt, arp, 0, max_ptr);
    return 0;
}

int printf(const char* fmt, ...)
{
    va_list arp;

    va_start(arp, fmt);
    vprintf(fmt, arp);
    va_end(arp);

    return 0;
}

int sprintf(char* buff, const char* fmt, ...)
{
    va_list arp;

    char* outptr = buff;

    va_start(arp, fmt);
    xvprintf_internal(fmt, arp, &outptr, max_ptr);
    va_end(arp);

    *outptr = 0;

    return outptr - buff;
}

int snprintf(char* buff, unsigned long n, const char* fmt, ...)
{
    va_list arp;

    va_start(arp, fmt);
    int ret = vsnprintf(buff, n, fmt, arp);
    va_end(arp);

    return ret;
}

#        if defined(__clang__)
__attribute__((no_sanitize("undefined")))
#        endif
int vsnprintf(char* buff, unsigned long n, const char* fmt, va_list arp)
{
    char* outptr = buff;
    char* snprintf_limit = buff + n - (n ? 1 : 0);

    if ((!buff && n) || outptr > snprintf_limit) {
        __builtin_trap();
        __UNREACHED__;
    }

    xvprintf_internal(fmt, arp, &outptr, snprintf_limit);

    int distance = outptr - buff;

    if (n == 0) {
        return distance;
    }

    if (outptr > snprintf_limit - 1) {
        *snprintf_limit = 0;
    } else {
        *outptr = 0;
    }

    return distance;
}
#    endif

#endif /* _USE_XFUNC_OUT */
