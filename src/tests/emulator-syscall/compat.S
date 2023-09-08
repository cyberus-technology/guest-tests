/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/testhelper/debugport_interface.h>

.code32

.global far_jmp_ptr;

far_jmp_ptr:
    mov $DEBUGPORT_EMUL_ONCE, %eax
    outb $DEBUGPORT_NUMBER
    ljmp $0x1b, $__jump_target
    ud2a
__jump_target:
    lret

.section .note.GNU-stack, "", %progbits
