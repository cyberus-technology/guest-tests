/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "stdint.h"
#include <compiler.h>

struct uncopyable
{
    uncopyable() = default;
    uncopyable(const uncopyable&) = delete;
    uncopyable& operator=(const uncopyable&) = delete;
};

struct unmovable
{
    unmovable() = default;
    unmovable(unmovable&&) = delete;
    unmovable& operator=(unmovable&&) = delete;
};

static constexpr inline int tzcnt(unsigned x)
{
    // FIXME: Throw exception for x == 0 when support is available.
    return __builtin_ctz(x);
}

// Necessary to resolve __COUNTER__ inside other macros.
#define CONCAT(x, y) x##y

// Can be used to mark an entire class non-final.
#define MARK_NON_FINAL_CLASS(c)                     \
    namespace                                       \
    {                                               \
        class [[maybe_unused]] final_impl final : c \
        {};                                         \
    }

// Helper for non-final markers on member functions.
#define MARK_NON_FINAL_METHOD_HELPER(c, ret, m, count, ...)          \
    namespace                                                        \
    {                                                                \
        struct [[maybe_unused]] CONCAT(final_impl, count) : public c \
        {                                                            \
            virtual ~CONCAT(final_impl, count)(){};                  \
            [[maybe_unused]] virtual ret m __VA_ARGS__ final override{};

// Can be used to mark a specific (const) function non-final.
#define MARK_NON_FINAL_METHOD(c, ret, m)                 \
    MARK_NON_FINAL_METHOD_HELPER(c, ret, m, __COUNTER__) \
    }                                                    \
    ;                                                    \
    }
#define MARK_NON_FINAL_METHOD_CONST(c, ret, m)                  \
    MARK_NON_FINAL_METHOD_HELPER(c, ret, m, __COUNTER__, const) \
    }                                                           \
    ;                                                           \
    }

// Same as MARK_NON_FINAL_METHOD, but allows to put additional definitions inside the class body, e.g. to prevent
// additional warning, such as hidden-function, ...
#define MARK_NON_FINAL_METHOD_WITH(c, ret, m) MARK_NON_FINAL_METHOD_HELPER(c, ret, m, __COUNTER__)
#define MARK_NON_FINAL_METHOD_WITH_END \
    }                                  \
    ;                                  \
    }
