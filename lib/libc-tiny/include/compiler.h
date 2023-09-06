/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#define __PACKED__ __attribute__((packed))
#define __NOINLINE__ __attribute__((noinline))
#define __NO_RETURN__ __attribute__((noreturn))
#define __USED__ __attribute__((used))
#define __SECTION__(x) __attribute__((section(x)))
#define __WEAK__ __attribute__((weak))

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

#define __UNREACHED__                                                                                                  \
    do {                                                                                                               \
        __builtin_trap();                                                                                              \
        __builtin_unreachable();                                                                                       \
    } while (0);

#ifdef __cplusplus
#    define EXTERN_C extern "C"
#else
#    define EXTERN_C ;
#endif

#if defined(__clang__)
#    define COMPILER_NAME "clang"
#    define COMPILER_MAJOR __clang_major__
#    define COMPILER_MINOR __clang_minor__
#    define COMPILER_PATCH __clang_patchlevel__
#    ifdef __cplusplus
#        define FALL_THROUGH [[clang::fallthrough]]
#    else
#        define FALL_THROUGH
#    endif
#    define __UBSAN_NO_UNSIGNED_OVERFLOW__ __attribute__((no_sanitize("unsigned-integer-overflow")))
#elif defined(__GNUC__)
#    define COMPILER_NAME "gcc"
#    define COMPILER_MAJOR __GNUC__
#    define COMPILER_MINOR __GNUC_MINOR__
#    define COMPILER_PATCH __GNUC_PATCHLEVEL__
#    if COMPILER_MAJOR > 5
#        define FALL_THROUGH __attribute__((fallthrough))
#    else
#        define FALL_THROUGH
#    endif
// unlike clang, gcc does not offer "unsigned-integer-overflow" checks
#    define __UBSAN_NO_UNSIGNED_OVERFLOW__
#else
#    define COMPILER_NAME "unknown compiler"
#    define COMPILER_MAJOR 0
#    define COMPILER_MINOR 0
#    define COMPILER_PATCH 0
#    define FALL_THROUGH
#endif

#ifdef RELEASE
#    define DEBUG_ONLY(x)
#else
#    define DEBUG_ONLY(x)                                                                                              \
        do {                                                                                                           \
            x                                                                                                          \
        } while (0);
#endif
