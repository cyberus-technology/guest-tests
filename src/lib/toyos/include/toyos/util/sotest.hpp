/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/util/trace.hpp>

namespace test_protocol
{

    constexpr size_t SOTEST_VERSION{ 1u };

    inline void begin(unsigned test_count)
    {
        pprintf("SOTEST VERSION {} BEGIN {}\n", SOTEST_VERSION, test_count);
    }

    inline void success(const char* name)
    {
        pprintf("SOTEST SUCCESS \"%s\"\n", name);
    }
    inline void skip()
    {
        pprintf("SOTEST SKIP\n");
    }
    inline void fail(const char* name)
    {
        pprintf("SOTEST FAIL \"%s\"\n", name);
    }
    inline void end()
    {
        pprintf("SOTEST END\n");
    }

    inline void benchmark(const char* name, long int value, const char* unit)
    {
        pprintf("SOTEST BENCHMARK:{s}:{s}:{}\n", name, unit, value);
    }

}  // namespace test_protocol
