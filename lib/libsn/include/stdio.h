/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.h>
#include <stdarg.h>

#define EOF (-1)

typedef long int off_t;
typedef long int off64_t;

EXTERN_C int putc(char c);
EXTERN_C int putchar(int c);
EXTERN_C int puts(const char* str);

EXTERN_C int printf(const char* fmt, ...);
EXTERN_C int vprintf(const char* fmt, va_list ap);
EXTERN_C int sprintf(char* buff, const char* fmt, ...);
EXTERN_C int snprintf(char* buff, unsigned long n, const char* fmt, ...);
EXTERN_C int vsnprintf(char* buff, unsigned long n, const char* fmt, va_list ap);
