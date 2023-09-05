/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
#include <debugport_interface.h>
#include <segmentation.hpp>
#include <trace.hpp>
#include <x86asm.hpp>
#include <x86defs.hpp>

template <typename T>
void print(const T& val);

template <>
void print<x86::descriptor_ptr>(const x86::descriptor_ptr& val)
{
    pprintf("base {#x} limit {#x}\n", val.base, val.limit);
}

inline void lgdt(x86::descriptor_ptr& ptr, bool emulated = false)
{
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: lgdt %[ptr]" ::[emul] "r"(emulated),
                 [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [ptr] "m"(ptr));
}

inline x86::descriptor_ptr sgdt(bool emulated = false)
{
    x86::descriptor_ptr ptr;
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: sgdt %[ptr]"
                 : [ptr] "=m"(ptr)
                 : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
    return ptr;
}

inline void lidt(x86::descriptor_ptr& ptr, bool emulated = false)
{
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: lidt %[ptr]" ::[emul] "r"(emulated),
                 [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [ptr] "m"(ptr));
}

inline x86::descriptor_ptr sidt(bool emulated = false)
{
    x86::descriptor_ptr ptr;
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: sidt %[ptr]"
                 : [ptr] "=m"(ptr)
                 : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
    return ptr;
}

inline void ltr(uint16_t sel, bool emulated = false)
{
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: ltr %[tr]" ::[tr] "rm"(sel),
                 [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
}

inline uint16_t str(bool emulated = false)
{
    uint16_t tr;
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: str %[tr]"
                 : [tr] "=rm"(tr)
                 : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
    return tr;
}

inline void lldt(uint16_t sel, bool emulated = false)
{
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: lldt %[ldt]" ::[ldt] "rm"(sel),
                 [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
}

inline uint16_t sldt(bool emulated = false)
{
    uint16_t ldt;
    asm volatile("cmp $0, %[emul];"
                 "je 2f;"
                 "outb %[dbgport];"
                 "2: sldt %[ldt]"
                 : [ldt] "=rm"(ldt)
                 : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));
    return ldt;
}
