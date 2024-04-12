/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

.text

// void exit_handler_low_level();
//
// Low-level entry point for vm exits.
// Saves guest state (which is not included in the VMCS), hands control
// to the high level handler. Guest state is restored after the high level
// handler returns.
.globl exit_handler_low_level
exit_handler_low_level:
    // align stack pointer
    and $~0xF, %rsp

    // We are about to push 15 8 byte values on the stack. To preserve the 16 byte alignment, push an 8 byte dummy.
    push %rax

    push %rax
    push %rbx
    push %rcx
    push %rdx
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

    // prepare parameter for vmx_handler
    mov %rsp, %rdi
    call vmx_handler

    // restore state
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
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    // No need to remove dummy or revert alignment.
    // We are leaving the exit handler and the next one starts with a fresh stack pointer.

    vmresume

.extern disable_vmx

// void restore_arch_state_and_jump(guest_regs& regs, uint64_t rsp);
// @param rsp stack pointer value
//
// Restored saved arch state and sets the stack pointer.
// This function is used on the shut down path when tinivisor::stop is called.
// According to calling conventions, only callee saved registers have to be restored.
// However, with compiler optimizations, that does not always seem to be true.
// To be safe, we restore everything.
.globl restore_arch_state_and_jump
restore_arch_state_and_jump:
    mov %rsi, %rsp

    mov    (%rdi), %r15
    mov   8(%rdi), %r14
    mov  16(%rdi), %r13
    mov  24(%rdi), %r12
    mov  32(%rdi), %r11
    mov  40(%rdi), %r10
    mov  48(%rdi), %r9
    mov  56(%rdi), %r8
    mov  72(%rdi), %rsi
    mov  80(%rdi), %rbp
    mov  88(%rdi), %rdx
    mov  96(%rdi), %rcx
    mov 104(%rdi), %rbx
    mov 112(%rdi), %rax

    // Restore RDI last because it points to the saved registers
    mov  64(%rdi), %rdi
    jmp disable_vmx

.section .note.GNU-stack, "", %progbits
