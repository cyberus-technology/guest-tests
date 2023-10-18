/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

// clang-format off
#define ELFNOTE_START(section_name, name, type)                                                                        \
    .pushsection .note.section_name, "", @note;                                                                        \
    .align 4;                                                                                                          \
    .long 2f - 1f /* namesz */;                                                                                        \
    .long 4f - 3f /* descsz */;                                                                                        \
    .long type;                                                                                                        \
    1 :.asciz #name;                                                                                                   \
    2 :.align 4;                                                                                                       \
    3:

#define ELFNOTE_END                                                                                                    \
    4 :.align 4;                                                                                                       \
    .popsection;

// clang-format on

/*
 * Creates an ELF note section that is prefixed with `.note`.
 */
#define ELFNOTE(section_name, name, type, desc) \
    ELFNOTE_START(section_name, name, type)     \
    desc;                                       \
    ELFNOTE_END
