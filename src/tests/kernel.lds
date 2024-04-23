/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>
ENTRY(_startup)
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")

PHDRS
{
    rx    PT_LOAD FLAGS(1);
    ro    PT_LOAD FLAGS(4);
    rw    PT_LOAD FLAGS(6);
    note  PT_NOTE;
}

/** From our testing and experience, this address works on a variety of
    real and virtual platforms with different firmware. Libtoyos is currently
    not relocatable in physical address space. */
PROVIDE(LOAD_ADDR = 12M);

SECTIONS
{
    .text (LOAD_ADDR) : ALIGN(PAGE_SIZE) {
        KEEP(*(.multiboot));
        PROVIDE(code = .);
        KEEP(*(.text))
        KEEP(*(.text.*))
        *(.gnu.linkonce.t*)
    } : rx

    .rodata ALIGN(PAGE_SIZE) : ALIGN(PAGE_SIZE) {
        *(.got)
        *(.got.plt)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(8);
        PROVIDE(__CTOR_LIST_START__ = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array*)))
        KEEP(*(SORT_BY_INIT_PRIORITY(.ctors*)))
        PROVIDE(__CTOR_LIST_END__ = .);
        PROVIDE(__DTOR_LIST_START__ = .);
        *(SORT_BY_INIT_PRIORITY(.fini_array*))
        *(SORT_BY_INIT_PRIORITY(.dtors*))
        PROVIDE(__DTOR_LIST_END__ = .);
        *(.gnu.linkonce.r*)
        *(.note.gnu.build-id)
    } : ro

    .data ALIGN(PAGE_SIZE) : ALIGN(PAGE_SIZE) {
        PROVIDE(data = .);
        *(.gnu.linkonce.d*)
        *(.data)
        *(.data.*)
        *(COMMON)
    } : rw

    .bss : {
        *(.gnu.linkonce.b*)
        *(.bss)
        *(.bss.*)
    } : rw

    .note.xen_pvh : {
        *(.note.xen_pvh)
    } : note

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note.gnu.property)
    }
}
