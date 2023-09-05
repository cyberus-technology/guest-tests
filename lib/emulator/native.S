/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */


#define GENERATE_JUMPTABLE
#include <emulator/macros.hpp>
#undef GENERATE_JUMPTABLE

INSN_2OP(bt, r16, %cx,  r16, %bx)
INSN_2OP(bt, r32, %ecx, r32, %ebx)
INSN_2OP(bt, r64, %rcx, r64, %rbx)

INSN_2OP(bt, r16, %cx,  m16, (%rbx))
INSN_2OP(bt, r32, %ecx, m32, (%rbx))
INSN_2OP(bt, r64, %rcx, m64, (%rbx))

INSN_2OP(movbe, r16,  %cx, m16, (%rbx))
INSN_2OP(movbe, r32, %ecx, m32, (%rbx))
INSN_2OP(movbe, r64, %rcx, m64, (%rbx))

INSN_2OP(movbe, m16, (%rcx), r16,  %bx)
INSN_2OP(movbe, m32, (%rcx), r32, %ebx)
INSN_2OP(movbe, m64, (%rcx), r64, %rbx)

.section .note.GNU-stack, "", %progbits
