/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */
#pragma once

#ifdef __cplusplus
#define NULL nullptr
#else
#define NULL ((void*)0)
#endif

#define offsetof(t, x) __builtin_offsetof(t, x)

#ifdef __x86_64__

typedef unsigned long size_t;
typedef long ssize_t;
typedef long ptrdiff_t;

#else
#error Implement me!
#endif
