// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later.code64

.globl syscall_entry, irq_handlers;
.weak syscall_handler;

syscall_handler:
    ret

syscall_entry:
    // Syscall param0 == 0 means we should stay in kernel mode and jump to the location in RSI
    cmp $0, %rdi
    je return_to_kernel

    // Save return IP and flags
    push %rcx
    push %r11

    // Call the C/C++ handler function
    call syscall_handler

    // Restore return IP and flags, return to user-mode
    pop %r11
    pop %rcx
    sysretq

return_to_kernel:
    jmp *%rsi

irq_common:
    /*
     * Our stack looks like this:
     *
     * SS
     * SP
     * FLAGS
     * CS
     * IP
     * ERROR_CODE
     * VECTOR <-- current SP
     */

    push %rax
    push %rbx
    push %rcx
    push %rdx

    // RSP is already in IRET frame
    push %rbp
    push %rsi
    push %rdi

    push %r8
    push %r9
    push %r10
    push %r11

    push %r12
    push %r13
    push %r14
    push %r15

    // Pass a pointer to the intr_regs structure as parameter
    mov %rsp, %rdi
    call irq_entry

    pop %r15
    pop %r14
    pop %r13
    pop %r12

    pop %r11
    pop %r10
    pop %r9
    pop %r8

    pop %rdi
    pop %rsi
    pop %rbp
    // RSP will be handled by IRET

    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    // Get rid of error code and vector
    add $16, %rsp

    iretq

.data

irq_handlers:

.text

.macro INTR VEC, ERR
1:
.data
    .quad 1b
.previous

.if \ERR == 0
    push $0
.endif
    push $\VEC
    jmp irq_common
.endm

INTR  0, 0
INTR  1, 0
INTR  2, 0
INTR  3, 0
INTR  4, 0
INTR  5, 0
INTR  6, 0
INTR  7, 0
INTR  8, 1
INTR  9, 0
INTR 10, 1
INTR 11, 1
INTR 12, 1
INTR 13, 1
INTR 14, 1
INTR 15, 0
INTR 16, 0
INTR 17, 1
INTR 18, 0
INTR 19, 0
INTR 20, 0
.set VEC, 21
.rept 256 - 21
INTR VEC, 0
.set VEC, VEC + 1
.endr

.section .note.GNU-stack, "", %progbits
