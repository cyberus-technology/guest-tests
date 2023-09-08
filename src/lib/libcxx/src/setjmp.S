/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

.global setjmp, longjmp

/// INPUT:
/// RDI: env buffer
///
/// RETURN: 0 on direct call, != 0 when called via longjmp
/// We only need to save the registers
/// that are owned by the callee. All other registers
/// are automatically saved by the caller of setjump as
/// dictated by the calling x86_64 conventions and of course
/// the current return RIP.
setjmp:
    mov  %rbp,    (%rdi)
    mov  %rsp,   8(%rdi)
    mov  %rbx,  16(%rdi)
    mov  %r12,  24(%rdi)
    mov  %r13,  32(%rdi)
    mov  %r14,  40(%rdi)
    mov  %r15,  48(%rdi)
    mov (%rsp),    %rax
    mov  %rax,  56(%rdi)
    xor  %eax,     %eax
    ret

/// INPUT:
/// RDI: env buffer in
/// RSI: return parameter
longjmp:
    mov   (%rdi), %rbp
    mov  8(%rdi), %rsp
    mov 16(%rdi), %rbx
    mov 24(%rdi), %r12
    mov 32(%rdi), %r13
    mov 40(%rdi), %r14
    mov 48(%rdi), %r15
    mov 56(%rdi), %rax

    mov %rax, (%rsp) // set new return RIP
    mov %esi,  %eax  // return value
    cmp $0,    %eax
    jne .exit
    inc %eax
.exit:
    ret

.section .note.GNU-stack, "", %progbits
