/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.h>
#include <stdint.h>

typedef struct __PACKED__ jmp_buf_internal
{
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t ip;
} jmp_buf[1];

EXTERN_C int setjmp(jmp_buf env);
EXTERN_C __attribute__((noreturn)) void longjmp(jmp_buf env, int val);
