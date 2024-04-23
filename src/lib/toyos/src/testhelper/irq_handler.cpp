// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/x86/x86asm.hpp>

idt global_idt;
static irq_handler_t irq_handler_fn = nullptr;

void irq_handler::set(const irq_handler_t& new_handler)
{
    irq_handler_fn = new_handler;
}

void irq_entry(intr_regs* regs)
{
    if (irq_handler_fn) {
        irq_handler_fn(regs);
    }
    else {
        info("NO INTERRUPT HANDLER DEFINED");
        info("{s}: vector {#x} error code {#x} ip {#x}, ", __func__, regs->vector, regs->error_code, regs->rip);
        disable_interrupts_and_halt();
    }
}

irq_handler::guard::guard(const irq_handler_t& new_handler)
{
    old_handler = irq_handler_fn;
    irq_handler_fn = new_handler;
}
irq_handler::guard::~guard()
{
    irq_handler_fn = old_handler;
}
