/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <stddef.h>

namespace string_backend
{
void* memcpy(void* __restrict __dest, const void* __restrict __src, size_t __n);
void* memmove(void* __dest, const void* __src, size_t __n);
void* memset(void* __s, int __c, size_t __n);
int memcmp(const void* __s1, const void* __s2, size_t __n);
char* strcpy(char* __restrict __dest, const char* __restrict __src);
int strcmp(const char* __s1, const char* __s2);
int strncmp(const char* str1, const char* str2, size_t num);
void* memchr(const void* __s, int __c, size_t __n);
size_t strlen(const char* __s);

unsigned long long strtoull(const char* str, char** str_end, int base);
unsigned long strtoul(const char* str, char** str_end, int base);
long long strtoll(const char* str, char** str_end, int base);
long strtol(const char* str, char** str_end, int base);

} // namespace string_backend
