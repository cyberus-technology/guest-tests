/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

// These have to be #defines, because they will be used by assembly files later.

#define DEBUGPORT_NUMBER 0x80

#define DEBUGPORT_GROUP_MASK 0xF0

#define DEBUGPORT_MISC_GROUP 0x00
#define DEBUGPORT_QUERY_HV (DEBUGPORT_MISC_GROUP + 0)
#define DEBUGPORT_HV_PRESENT 0xD34DB33F

#define DEBUGPORT_EMUL_GROUP 0x10
#define DEBUGPORT_EMUL_ONCE (DEBUGPORT_EMUL_GROUP + 0)
#define DEBUGPORT_EMUL_START (DEBUGPORT_EMUL_GROUP + 1)
#define DEBUGPORT_EMUL_END (DEBUGPORT_EMUL_GROUP + 2)

#define DEBUGPORT_EXC_GROUP 0x20
#define DEBUGPORT_ENABLE_EXC (DEBUGPORT_EXC_GROUP + 0)
#define DEBUGPORT_DISABLE_EXC (DEBUGPORT_EXC_GROUP + 1)

#define DEBUGPORT_WIN_GROUP 0x30
#define DEBUGPORT_INTERCEPT_CUR (DEBUGPORT_WIN_GROUP + 0)
#define DEBUGPORT_SET_BP (DEBUGPORT_WIN_GROUP + 1)
#define DEBUGPORT_REMOVE_BP (DEBUGPORT_WIN_GROUP + 2)

#define DEBUGPORT_BP_GROUP 0x40
#define DEBUGPORT_QUERY_BP (DEBUGPORT_BP_GROUP + 0)
#define DEBUGPORT_EMULATE_REPLACEMENT (DEBUGPORT_BP_GROUP + 1)

#define DEBUGPORT_FPU_GROUP 0x50
#define DEBUGPORT_FPU_CLEAR_HOST_REGS (DEBUGPORT_FPU_GROUP + 0)
#define DEBUGPORT_FPU_TOUCH_AVX (DEBUGPORT_FPU_GROUP + 1)

#define DEBUGPORT_PARAM1_REG_ASM "rcx"
#define DEBUGPORT_PARAM1_REG_CONSTRAINT "c"
#define DEBUGPORT_PARAM1_REG_GPR x86::gpr::RCX

#define DEBUGPORT_PARAM2_REG_ASM "rbx"
#define DEBUGPORT_PARAM2_REG_CONSTRAINT "b"
#define DEBUGPORT_PARAM2_REG_GPR x86::gpr::RBX

#define DEBUGPORT_RETVAL_REG_ASM "rdx"
#define DEBUGPORT_RETVAL_REG_CONSTRAINT "+&d"
#define DEBUGPORT_RETVAL_REG_GPR x86::gpr::RDX

#ifndef __ASSEMBLER__

#    include <toyos/util/interval.hpp>
#    include <toyos/x86/x86defs.hpp>

static constexpr const cbl::interval DEBUG_PORTS {DEBUGPORT_NUMBER, DEBUGPORT_NUMBER + 1};

inline uint64_t debugport_call(uint8_t function)
{
    uint64_t ret = 0;
    asm volatile("outb %[dbgport];"
                 : DEBUGPORT_RETVAL_REG_CONSTRAINT(ret)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(function));
    return ret;
}

inline uint64_t debugport_param1(uint8_t function, uint64_t param)
{
    uint64_t ret = 0;
    asm volatile("outb %[dbgport];"
                 : DEBUGPORT_RETVAL_REG_CONSTRAINT(ret)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(function), DEBUGPORT_PARAM1_REG_CONSTRAINT(param));
    return ret;
}

inline uint64_t debugport_param2(uint8_t function, uint64_t param1, uint64_t param2)
{
    uint64_t ret = 0;
    asm volatile("outb %[dbgport];"
                 : DEBUGPORT_RETVAL_REG_CONSTRAINT(ret)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(function), DEBUGPORT_PARAM1_REG_CONSTRAINT(param1),
                   DEBUGPORT_PARAM2_REG_CONSTRAINT(param2));
    return ret;
}

inline bool debugport_present()
{
    return debugport_call(DEBUGPORT_QUERY_HV) == DEBUGPORT_HV_PRESENT;
}

inline void enable_exc_exit(x86::exception exc)
{
    debugport_param1(DEBUGPORT_ENABLE_EXC, unsigned(exc));
}

inline void disable_exc_exit(x86::exception exc)
{
    debugport_param1(DEBUGPORT_DISABLE_EXC, unsigned(exc));
}

#endif
