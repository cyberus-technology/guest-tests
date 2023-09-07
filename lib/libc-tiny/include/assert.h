/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#if defined(__cplusplus) && !defined(NDEBUG)
#    include <toyos/util/trace.hpp>
#    define assert(x) ASSERT(x, "Assertion failed")
#else
#    define assert(x)                                                                                                  \
        do {                                                                                                           \
        } while (0)
#endif
