// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <compiler.h>
#include <stddef.h>

struct div_t;
struct ldiv_t;
struct lldiv_t;

#ifndef __cplusplus
typedef int wchar_t;
#endif

double atof(const char* __nptr);
int atoi(const char* __nptr);
long int atol(const char* __nptr);
long long int atoll(const char* __nptr);

double strtod(const char* __restrict __nptr, char** __restrict __endptr);
float strtof(const char* __restrict __nptr, char** __restrict __endptr);
long double strtold(const char* __restrict __nptr, char** __restrict __endptr);
long int strtol(const char* __restrict __nptr, char** __restrict __endptr, int __base);
unsigned long int strtoul(const char* __restrict __nptr, char** __restrict __endptr, int __base);
unsigned long int strtoul(const char* __restrict __nptr, char** __restrict __endptr, int __base);
long long int strtoll(const char* __restrict __nptr, char** __restrict __endptr, int __base);
unsigned long long int strtoull(const char* __restrict __nptr, char** __restrict __endptr, int __base);

int rand(void);
void srand(unsigned int __seed);

EXTERN_C void* malloc(size_t __size);
EXTERN_C void* calloc(size_t __nmemb, size_t __size);
EXTERN_C void* realloc(void* __ptr, size_t __size);
EXTERN_C void free(void* __ptr);

void abort(void) __attribute__((__noreturn__));
int atexit(void (*__func)(void));
void exit(int __status) __attribute__((__noreturn__));
void _Exit(int __status) __attribute__((__noreturn__));

char* getenv(const char* __name);
int system(const char* __command);
void* bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, void* __compar);
void qsort(void* __base, size_t __nmemb, size_t __size, void* __compar);

int abs(int __x) __attribute__((__const__));
long int labs(long int __x) __attribute__((__const__));
long long int llabs(long long int __x) __attribute__((__const__));

struct div_t div(int __numer, int __denom) __attribute__((__const__));
struct ldiv_t ldiv(long int __numer, long int __denom) __attribute__((__const__));
struct lldiv_t lldiv(long long int __numer, long long int __denom) __attribute__((__const__));

int mblen(const char* __s, size_t __n);
int mbtowc(wchar_t* __restrict __pwc, const char* __restrict __s, size_t __n);
int wctomb(char* __s, wchar_t __wchar);
size_t mbstowcs(wchar_t* __restrict __pwcs, const char* __restrict __s, size_t __n);
size_t wcstombs(char* __restrict __s, const wchar_t* __restrict __pwcs, size_t __n);
