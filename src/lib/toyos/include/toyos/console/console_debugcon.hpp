// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Module for the QEMU debugcon device. Useful to get very early output without
 * having to configure anything or parsing the cmdline.
 *
 * More info:
 * - https://phip1611.de/blog/how-to-use-qemus-debugcon-feature-and-write-to-a-file/
 * - https://github.com/qemu/qemu/blob/master/hw/char/debugcon.c
 */

#pragma once

#include <toyos/printf/backend.hpp>
#include <toyos/x86/x86asm.hpp>

namespace debugcon
{
    constexpr uint16_t IO_PORT = 0xe9;

    void putc(unsigned char c)
    {
        outb(IO_PORT, c);
    }
}  // namespace debugcon

namespace console_debugcon
{
    static void init()
    {
        add_printf_backend(debugcon::putc);
        info("Initialized QEMU debugcon console");
    }

}  // namespace console_debugcon
