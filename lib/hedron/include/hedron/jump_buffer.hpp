/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#define JUMP_BUFFER_OFFSET_BASE 0x0
#define JUMP_BUFFER_RBP_OFFSET JUMP_BUFFER_OFFSET_BASE
#define JUMP_BUFFER_RSP_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x8
#define JUMP_BUFFER_RBX_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x10
#define JUMP_BUFFER_R12_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x18
#define JUMP_BUFFER_R13_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x20
#define JUMP_BUFFER_R14_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x28
#define JUMP_BUFFER_R15_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x30
#define JUMP_BUFFER_RIP_OFFSET JUMP_BUFFER_OFFSET_BASE + 0x38

#define JUMP_BUFFER_SIZE (JUMP_BUFFER_OFFSET_BASE + 0x8 + JUMP_BUFFER_RIP_OFFSET)

#ifndef __ASSEMBLER__
#    include <cstddef>
#    include <cstdint>

namespace hedron
{
struct jump_buffer
{
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
};

static_assert(offsetof(jump_buffer, rbp) == JUMP_BUFFER_RBP_OFFSET, "Mismatch between jump buffer offsets: rbp");
static_assert(offsetof(jump_buffer, rsp) == JUMP_BUFFER_RSP_OFFSET, "Mismatch between jump buffer offsets: rsp");
static_assert(offsetof(jump_buffer, rbx) == JUMP_BUFFER_RBX_OFFSET, "Mismatch between jump buffer offsets: rbx");
static_assert(offsetof(jump_buffer, r12) == JUMP_BUFFER_R12_OFFSET, "Mismatch between jump buffer offsets: r12");
static_assert(offsetof(jump_buffer, r13) == JUMP_BUFFER_R13_OFFSET, "Mismatch between jump buffer offsets: r13");
static_assert(offsetof(jump_buffer, r14) == JUMP_BUFFER_R14_OFFSET, "Mismatch between jump buffer offsets: r14");
static_assert(offsetof(jump_buffer, r15) == JUMP_BUFFER_R15_OFFSET, "Mismatch between jump buffer offsets: r15");
static_assert(offsetof(jump_buffer, rip) == JUMP_BUFFER_RIP_OFFSET, "Mismatch between jump buffer offsets: rip");
static_assert(sizeof(jump_buffer) == JUMP_BUFFER_SIZE,
              "Mismatch between assembly jump buffer size assumption and actual structure size!");

} // namespace hedron
#endif
