/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

/* These constants have to be macros because they are included from assembly files and linker scripts. */
#include <arch.h>

/** The next four macros describe the memory layout of the stack.
 * The stack is always natuarally aligned to its virtual size and
 * backed with anonymous memory defined by STACK_BITS_PHYS.
 *
 * TOS
 * |
 * |
 * ********************************************
 * *                 *                        *
 * * STACK_SIZE_PHYS *       GUARD AREA       *
 * *                 *                        *
 * ********************************************
 * |    --      STACK_SIZE_VIRT      --       *
 *
 */
#define STACK_BITS_PHYS (PAGE_BITS + 1)
#define STACK_SIZE_PHYS (1 << STACK_BITS_PHYS)
#define STACK_BITS_VIRT (PAGE_BITS + 2)
#define STACK_SIZE_VIRT (1 << STACK_BITS_VIRT)

#define HEAP_ALIGNMENT (0x10)

#define LOAD_ADDR (0x100000000000)
#define LOAD_AREA_SIZE (0x40000000)

#define STATIC_TLS_SIZE (0x48)
#define UTCB_FROM_HIP_OFFSET (-0x1000)
