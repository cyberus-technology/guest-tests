/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <string_backend.hpp>

namespace
{

bool isspace(char c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n': return true;
    default: return false;
    }
}

int ctoi_(char c)
{
    switch (c) {
    case '0' ... '9': return c - '0';
    case 'a' ... 'z': return c - 'a' + 10;
    case 'A' ... 'Z': return c - 'A' + 10;
    default: return -1;
    }
}

int ctoi(char c, int base)
{
    int i = ctoi_(c);
    return (0 <= i && i < base) ? i : -1;
}

} // anonymous namespace

namespace string_backend
{

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

    const char* p {str};

    while (isspace(*p)) {
        ++p;
    }

    if (*p == '+') {
        ++p;
    }

    if (*p == '0' && *(p + 1) == 'x' && base == 16) {
        p += 2;
    }

    unsigned long long accum {0};

    while (*p) {
        int i {ctoi(*p, base)};
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
    return static_cast<unsigned long>(strtoull(str, str_end, base));
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

    int pos {1};

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

} // namespace string_backend
