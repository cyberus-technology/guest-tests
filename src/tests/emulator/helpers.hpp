/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
#include <toyos/testhelper/debugport_interface.h>

static constexpr const uint16_t SEL_DATA{ 0x10 };

#define MOV_FROM_SEG(seg)                                                                                                \
    /**                                                                                                                \
     * Load segment register seg                                                                                       \
     * \param emulated Force emulation                                                                                 \
     * \return The segment selector                                                                                    \
     */ \
    inline uint16_t mov_from_##seg(bool emulated)                                                                        \
    {                                                                                                                    \
        uint16_t ret{ 0 };                                                                                               \
        asm volatile("cmp $0, %[emul];"                                                                                  \
                     "je 2f;"                                                                                            \
                     "outb %[dbgport];"                                                                                  \
                     "2: mov %%" #seg ", %[ret];"                                                                        \
                     : [ret] "=rm"(ret)                                                                                  \
                     : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));                    \
        return ret;                                                                                                      \
    }

#define MOV_TO_SEG(seg)                                                                                                  \
    /**                                                                                                                \
     * Store segment register seg                                                                                      \
     * \param sel Segment Selector to load into seg                                                                    \
     * \param emulated Force emulation                                                                                 \
     */ \
    inline void mov_to_##seg(uint16_t sel, bool emulated)                                                                \
    {                                                                                                                    \
        asm volatile("cmp $0, %[emul];"                                                                                  \
                     "je 2f;"                                                                                            \
                     "outb %[dbgport];"                                                                                  \
                     "2: mov %[sel], %%" #seg ";" ::[emul] "r"(emulated),                                                \
                     [dbgport] "i"(DEBUG_PORTS.a),                                                                       \
                     "a"(DEBUGPORT_EMUL_ONCE),                                                                           \
                     [sel] "r"(sel));                                                                                    \
    }

MOV_FROM_SEG(cs)
MOV_FROM_SEG(ds)
MOV_FROM_SEG(es)
MOV_FROM_SEG(fs)
MOV_FROM_SEG(gs)
MOV_FROM_SEG(ss)

MOV_TO_SEG(ds)
MOV_TO_SEG(es)
MOV_TO_SEG(fs)
MOV_TO_SEG(gs)
MOV_TO_SEG(ss)
