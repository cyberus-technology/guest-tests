/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <compiler.h>
#include <stdlib.h>
#include <string.h>
#include <string_backend.hpp>

#ifndef LIBCXX_NO_MEMCPY
__USED__ void* memcpy(void* __restrict __dest, const void* __restrict __src, size_t __n)
{
    return string_backend::memcpy(__dest, __src, __n);
}

__USED__ void* memset(void* __s, int __c, size_t __n)
{
    return string_backend::memset(__s, __c, __n);
}
#endif

__USED__ void* memmove(void* __dest, const void* __src, size_t __n)
{
    return string_backend::memmove(__dest, __src, __n);
}

__USED__ int memcmp(const void* __s1, const void* __s2, size_t __n)
{
    return string_backend::memcmp(__s1, __s2, __n);
}

__USED__ int bcmp(const void* __s1, const void* __s2, size_t __n)
{
    return string_backend::memcmp(__s1, __s2, __n);
}

char* strcpy(char* __restrict __dest, const char* __restrict __src)
{
    return string_backend::strcpy(__dest, __src);
}

int strcmp(const char* __s1, const char* __s2)
{
    return string_backend::strcmp(__s1, __s2);
}

int strncmp(const char* __s1, const char* __s2, size_t __n)
{
    return string_backend::strncmp(__s1, __s2, __n);
}

void* memchr(const void* __s, int __c, size_t __n)
{
    return string_backend::memchr(__s, __c, __n);
}

size_t strlen(const char* __s)
{
    return string_backend::strlen(__s);
}

unsigned long long strtoull(const char* str, char** str_end, int base)
{
    return string_backend::strtoull(str, str_end, base);
}

unsigned long strtoul(const char* str, char** str_end, int base)
{
    return string_backend::strtoul(str, str_end, base);
}

long long strtoll(const char* str, char** str_end, int base)
{
    return string_backend::strtoll(str, str_end, base);
}

long strtol(const char* str, char** str_end, int base)
{
    return string_backend::strtol(str, str_end, base);
}
