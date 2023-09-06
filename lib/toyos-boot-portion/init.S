/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

/*
 * This file specifies the following entries:
 * - Multiboot 1
 * - Multiboot 2
 * - Xen PVH / direct boot
 *
 * All of them perform the handoff in 32-bit protected mode without paging.
 * Also for Xen PVH, this essentially corresponds to the I386 machine state of
 * Multiboot 1/2.
 */

#include <config.h>
#include <toyos/elfnote.hpp>

.global _startup, __cxa_pure_virtual, _boot_heap_start, __dso_handle, __cxa_atexit, gdt, gdt_tss

ELFNOTE(xen_pvh, Xen, 18 /* XEN_ELFNOTE_PHYS32_ENTRY  */, .long _startup_xen)

.code32
.section .multiboot, "aw"

.align 4
.long 0x1badb002 // multiboot magic
.long 0x0        // multiboot flags
.long (0x100000000 - 0x1badb002) // multiboot checksum

.align 8

mb2_header_start:

.long 0xe85250d6                                                       // magic
.long 0                                                                // arch
.long (mb2_header_end - mb2_header_start)                              // hdr length
.long (0x100000000 - (mb2_header_end - mb2_header_start) - 0xe85250d6) // checksum

mb2_header_end:

.section .text

/*
 * Xen PVH entry
 * Machine state is similar to I386 from Multiboot 1/2, except that the boot
 * info looks different.
 */
_startup_xen:
    // Save Xen boot info in %esi and put PVH magic in %edi.
    mov (%ebx), %edi
    mov %ebx, %esi
    jmp _startup_common

/* Multiboot1 and Multiboot2 entry */
_startup:
    // When we enter here, there is no stack pointer and EBX contains
    // a pointer to the Multiboot Information structure.

    // print 'Hi' in the top left corner
    movl $0x0f690f48,0xb8000

    // Save multiboot magic and MBI pointer
    mov %eax, %edi
    mov %ebx, %esi

_startup_common:

    // prepare page tables and the gdt
    mov $pml4, %eax
    mov %eax, %cr3

    // enable PAE
    mov $(1 << 5), %eax
    mov %eax, %cr4

    // enable long mode
    mov $0xc0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    // enable paging
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    // we're in compatibility mode now...

    lgdtl (gdt_ptr)

    // initialize our stack segment
    mov $0x10, %ax
    mov %ax, %ss

    ljmp $0x8, $__entry_64
    ud2a

.code64
.section .text
.extern init_heap, init_tss, init_interrupt_controllers, entry64

__entry_64:
    movabs $stack, %rsp
    and $~0xF, %rsp

    // Set IOPL=3
    pushf
    pop %rax
    or $(3 << 12), %rax
    push %rax
    popf

    // Save Multiboot arguments
    sub $8, %rsp
    push %rdi
    push %rsi

    call init_heap
    call init_tss
    call init_interrupt_controllers
    call __call_global_objects

    // Pass Multiboot magic and MBI pointer to entry function
    pop %rsi
    pop %rdi
    add $8, %rsp

    call entry64

    cli
    hlt

__cxa_pure_virtual:
    ud2a

__dso_handle:
__cxa_atexit:
    ret

__call_global_objects:
    // invoke global constructors
    movabs $__CTOR_LIST_START__, %rbx
    movabs $__CTOR_LIST_END__, %r15
    jmp .check_finish
.again:
    mov (%rbx), %rcx
    call *%rcx
    add $8, %rbx
.check_finish:
    cmp %r15, %rbx
    jb .again
    ret

.section .data
.space 4096
stack:

.align 4096
pd:
.set SPAGE, 0
.rept 512
    .quad SPAGE + 0x87      // PRESENT, R/W, USER, 2MB
.set SPAGE, SPAGE + 0x200000
.endr

pd2:
.rept 512
    .quad SPAGE + 0x87
.set SPAGE, SPAGE + 0x200000
.endr

pd3:
.rept 512
    .quad SPAGE + 0x87
.set SPAGE, SPAGE + 0x200000
.endr

pd4:
.rept 512
    .quad SPAGE + 0x87
.set SPAGE, SPAGE + 0x200000
.endr

pdpt:
    .quad pd  + 0x7       // PRESENT, R/W, USER
    .quad pd2 + 0x7       // PRESENT, R/W, USER
    .quad pd3 + 0x7       // PRESENT, R/W, USER
    .quad pd4 + 0x7       // PRESENT, R/W, USER
    .fill 508 * 8

pml4:
.rept 512
    .quad pdpt + 0x7       // PRESENT, R/W, USER
.endr

gdt:
    .long 0
    .long 0

    .long 0
    .long 0xa09b00 // 64bit CS CPL0

    .long 0xffff
    .long 0xa09300 // DS CPL0

    .long 0xffff   // 32bit CS CPL3
    .long 0xcffb00

    .long 0xffff   // DS CPL3
    .long 0xcff300

    .long 0
    .long 0xa0fb00 // 64bit CS CPL3

gdt_tss:
    .quad 0 // 64bit TSS
    .quad 0

gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .quad gdt

.section .note.GNU-stack, "", %progbits
