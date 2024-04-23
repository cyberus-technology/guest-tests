// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <compiler.h>
#include <stddef.h>

EXTERN_C void* memcpy(void* __restrict __dest, const void* __restrict __src, size_t __n);
EXTERN_C void* memmove(void* __dest, const void* __src, size_t __n);
EXTERN_C void* memccpy(void* __restrict __dest, const void* __restrict __src, int __c, size_t __n);
EXTERN_C void* memset(void* __s, int __c, size_t __n);
EXTERN_C int memcmp(const void* __s1, const void* __s2, size_t __n);

EXTERN_C char* strcpy(char* __restrict __dest, const char* __restrict __src);
EXTERN_C char* strncpy(char* __restrict __dest, const char* __restrict __src, size_t __n);
EXTERN_C char* strcat(char* __restrict __dest, const char* __restrict __src);
EXTERN_C char* strncat(char* __restrict __dest, const char* __restrict __src, size_t __n);
EXTERN_C int strcmp(const char* __s1, const char* __s2);
EXTERN_C int strncmp(const char* __s1, const char* __s2, size_t __n);
EXTERN_C int strcoll(const char* __s1, const char* __s2);
EXTERN_C size_t strxfrm(char* __restrict __dest, const char* __restrict __src, size_t __n);

EXTERN_C void* memchr(const void* __s, int __c, size_t __n);
EXTERN_C char* strchr(const char* __s, int __c);
EXTERN_C size_t strcspn(const char* __s, const char* __reject);
EXTERN_C size_t strspn(const char* __s, const char* __accept);
EXTERN_C char* strpbrk(const char* __s, const char* __accept);
EXTERN_C char* strstr(const char* __haystack, const char* __needle);
EXTERN_C char* strrchr(const char* __s, int __c);

EXTERN_C char* strerror(int __errnum);
EXTERN_C char* strtok(char* __restrict __s, const char* __restrict __delim);
EXTERN_C size_t strlen(const char* __s);
