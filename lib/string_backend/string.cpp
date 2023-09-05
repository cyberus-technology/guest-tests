/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <string_backend.hpp>

// We had integer conversion bugs here before. Tell the compiler to yell at us, if we do anything fishy.
#pragma GCC diagnostic error "-Wconversion"

namespace
{

void* memcpy_reverse(void* dst, const void* src, size_t n)
{
    char* dst_end = (char*) dst + n - 1;
    const char* src_end = (const char*) src + n - 1;

    asm("std; rep movsb; cld"
        : "+S"(src_end), "+D"(dst_end), "+c"(n), "=m"(*(char(*)[n]) dst)
        : "m"(*(const char(*)[n]) src));

    return dst;
}

} // anonymous namespace

namespace string_backend
{

void* memcpy(void* dst, const void* src, size_t n)
{
    void* orig_dst = dst;

    asm("rep movsb" : "+S"(src), "+D"(dst), "+c"(n), "=m"(*(char(*)[n]) dst) : "m"(*(const char(*)[n]) src));

    return orig_dst;
}

void* memset(void* dst, int c, size_t n)
{
    void* orig_dst = dst;

    asm("rep stosb" : "+D"(dst), "+c"(n), "=m"(*(char(*)[n]) dst) : "a"(c));

    return orig_dst;
}

void* memmove(void* dst, const void* src, size_t n)
{
    if (dst < src) {
        return memcpy(dst, src, n);
    } else {
        return memcpy_reverse(dst, src, n);
    }
}

char* strcpy(char* dst, const char* src)
{
    char* ret = dst;

    do {
        *dst++ = *src;
    } while (*src++ != '\0');

    return ret;
}

int strncmp(const char* str1, const char* str2, size_t num)
{
    for (size_t idx = 0; idx < num && !(str1[idx] == 0 && str2[idx] == 0); idx++) {
        int diff = str1[idx] - str2[idx];

        if (diff != 0) {
            return diff;
        }
    }

    return 0;
}

size_t strlen(const char* str)
{
    size_t len = 0;

    while (*(str++) != '\0') {
        len++;
    }

    return len;
}

int strcmp(const char* str1, const char* str2)
{
    for (size_t idx = 0; !(str1[idx] == 0 && str2[idx] == 0); idx++) {
        int diff = str1[idx] - str2[idx];

        if (diff != 0) {
            return diff;
        }
    }

    return 0;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* s1_ = (const unsigned char*) (s1);
    const unsigned char* s2_ = (const unsigned char*) (s2);

    for (size_t cnt = 0; cnt < n; cnt++, s1_++, s2_++) {
        unsigned char c1 = *s1_;
        unsigned char c2 = *s2_;

        if (c1 != c2) {
            return (c1 - c2);
        }
    }

    return 0;
}

void* memchr(const void* str, int c, size_t n)
{
    const unsigned char needle = (unsigned char) (c & 0xff);
    const unsigned char* cur = (const unsigned char*) str;

    for (size_t idx = 0; idx < n; idx++, cur++) {
        if (*cur == needle) {
            return (void*) cur;
        }
    }

    return NULL;
}

} // namespace string_backend
