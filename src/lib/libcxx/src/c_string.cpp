/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <compiler.h>
#include <stdlib.h>
#include <string.h>

#ifndef LIBCXX_NO_MEMCPY
__USED__ void* memcpy(void* __restrict dst, const void* __restrict src, size_t n)
{
   void* orig_dst = dst;

   asm("rep movsb"
       : "+S"(src), "+D"(dst), "+c"(n), "=m"(*(char(*)[n])dst)
       : "m"(*(const char(*)[n])src));

   return orig_dst;
}

__USED__ void* memset(void* dst, int c, size_t n)
{
   void* orig_dst = dst;

   asm("rep stosb"
       : "+D"(dst), "+c"(n), "=m"(*(char(*)[n])dst)
       : "a"(c));

   return orig_dst;
}
#endif

void* memcpy_reverse(void* dst, const void* src, size_t n)
{
   char* dst_end = (char*)dst + n - 1;
   const char* src_end = (const char*)src + n - 1;

   asm("std; rep movsb; cld"
       : "+S"(src_end), "+D"(dst_end), "+c"(n), "=m"(*(char(*)[n])dst)
       : "m"(*(const char(*)[n])src));

   return dst;
}

__USED__ void* memmove(void* dst, const void* src, size_t n)
{
   if (dst < src) {
      return memcpy(dst, src, n);
   }
   else {
      return memcpy_reverse(dst, src, n);
   }
}

__USED__ int memcmp(const void* s1, const void* s2, size_t n)
{
   const unsigned char* s1_ = (const unsigned char*)(s1);
   const unsigned char* s2_ = (const unsigned char*)(s2);

   for (size_t cnt = 0; cnt < n; cnt++, s1_++, s2_++) {
      unsigned char c1 = *s1_;
      unsigned char c2 = *s2_;

      if (c1 != c2) {
         return (c1 - c2);
      }
   }

   return 0;
}

__USED__ int bcmp(const void* __s1, const void* __s2, size_t __n)
{
   return memcmp(__s1, __s2, __n);
}

char* strcpy(char* __restrict dst, const char* __restrict src)
{
   char* ret = dst;

   do {
      *dst++ = *src;
   } while (*src++ != '\0');

   return ret;
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

void* memchr(const void* str, int c, size_t n)
{
   const unsigned char needle = (unsigned char)(c & 0xff);
   const unsigned char* cur = (const unsigned char*)str;

   for (size_t idx = 0; idx < n; idx++, cur++) {
      if (*cur == needle) {
         return (void*)cur;
      }
   }

   return NULL;
}

size_t strlen(const char* str)
{
   size_t len = 0;

   while (*(str++) != '\0') {
      len++;
   }

   return len;
}

bool isspace(char c)
{
   switch (c) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
         return true;
      default:
         return false;
   }
}

int ctoi_(char c)
{
   switch (c) {
      case '0' ... '9':
         return c - '0';
      case 'a' ... 'z':
         return c - 'a' + 10;
      case 'A' ... 'Z':
         return c - 'A' + 10;
      default:
         return -1;
   }
}

int ctoi(char c, int base)
{
   int i = ctoi_(c);
   return (0 <= i && i < base) ? i : -1;
}

unsigned long long strtoull(const char* str, char** str_end, int base)
{
   auto setend([str_end](const char* x) {
      if (str_end) {
         *str_end = const_cast<char*>(x);
      }
   });

   if (!str) {
      setend(nullptr);
      return 0;
   }

   if (*str == '\0') {
      setend(str);
      return 0;
   }

   const char* p{ str };

   while (isspace(*p)) {
      ++p;
   }

   if (*p == '+') {
      ++p;
   }

   if (*p == '0' && *(p + 1) == 'x' && base == 16) {
      p += 2;
   }

   unsigned long long accum{ 0 };

   while (*p) {
      int i{ ctoi(*p, base) };
      if (i < 0) {
         setend(p);
         return accum;
      }

      accum *= static_cast<unsigned long long>(base);
      accum += static_cast<unsigned long long>(i);

      ++p;
   }
   return accum;
}

unsigned long strtoul(const char* str, char** str_end, int base)
{
   auto setend([str_end](const char* x) {
      if (str_end) {
         *str_end = const_cast<char*>(x);
      }
   });

   if (!str) {
      setend(nullptr);
      return 0;
   }

   if (*str == '\0') {
      setend(str);
      return 0;
   }

   const char* p{ str };

   while (isspace(*p)) {
      ++p;
   }

   if (*p == '+') {
      ++p;
   }

   if (*p == '0' && *(p + 1) == 'x' && base == 16) {
      p += 2;
   }

   unsigned long long accum{ 0 };

   while (*p) {
      int i{ ctoi(*p, base) };
      if (i < 0) {
         setend(p);
         return accum;
      }

      accum *= static_cast<unsigned long long>(base);
      accum += static_cast<unsigned long long>(i);

      ++p;
   }
   return accum;
}

long long strtoll(const char* str, char** str_end, int base)
{
   if (!str) {
      if (str_end) {
         *str_end = nullptr;
      }
      return 0;
   }

   while (isspace(*str)) {
      ++str;
   }

   int pos{ 1 };

   if (*str == '-') {
      ++str;
      pos = -1;
   }

   return pos * static_cast<long long>(strtoull(str, str_end, base));
}

long strtol(const char* str, char** str_end, int base)
{
   return static_cast<long>(strtoll(str, str_end, base));
}
