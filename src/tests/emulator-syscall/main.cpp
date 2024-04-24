// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <compiler.hpp>
#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/usermode.hpp>
#include <toyos/util/trace.hpp>

static irqinfo irq_info;
static void irq_handler_fn(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code, regs->rip);
    irq_info.fixup(regs);
}

void prologue()
{
    irq_handler::set(irq_handler_fn);
}

namespace x86
{
    static bool operator==(const descriptor_ptr& lhs, const descriptor_ptr& rhs)
    {
        return lhs.base == rhs.base and lhs.limit == rhs.limit;
    }
}  // namespace x86

// Constructor has relevant side effects:
// - Enable EFER.SCE
// - Configure STAR, LSTAR, FMASK MSRs
// - Configure TSS with kernel stack pointer
[[maybe_unused]] static usermode_helper um;

TEST_CASE(syscall_sysret_works)
{
    um.enter_sysret();
    um.leave_syscall();

    um.enter_iret();
    um.leave_syscall();
}

// Inspired by the following blog post:
// https://revers.engineering/patchguard-detection-of-hypervisor-based-instrospection-p1/
//
// And enriched with the use of a debug exception handler stack that was never
// touched before (which makes IDT vectoring more likely).
TEST_CASE(patchguard_KiErrata704Present)
{
    irq_info.reset();

    // In order to cause an EPT violation while delivering the exception, we
    // need a bit of stack space that is likely to not have been touched before.
    // Anything contained in the binary (even BSS) doesn't work because the
    // bootloader probably touches it when the test is loaded.
    // Similar to the pagefault test, we choose an address that is close enough,
    // but not overlapping with our own binary. So far, this region has always
    // been available.
    const x86::segment_selector tss_selector{ x86::str() };
    assert(tss_selector.raw != 0);
    x86::gdt_entry* gdte{ x86::get_gdt_entry(get_current_gdtr(), tss_selector) };
    x86::tss* current_tss{ reinterpret_cast<x86::tss*>(gdte->base()) };
    current_tss->ist4 = 0xC01000;

    // By using the IST mechanism, we can make sure that the first time this
    // stack pointer is ever used is when we actually trigger this exception.
    auto& db_entry{ global_idt.entries[static_cast<uint8_t>(x86::exception::DB)] };
    db_entry.set_ist(4);

    const auto lstar_original{ rdmsr(x86::LSTAR) };
    const auto fmask_original{ rdmsr(x86::FMASK) };

    constexpr uintptr_t FAKE_LSTAR{ 0x1337F000 };
    wrmsr(x86::LSTAR, FAKE_LSTAR);

    wrmsr(x86::FMASK, fmask_original & ~x86::FLAGS_TF);

    // Letting the syscall instruction cause a debug exception allows us to
    // inspect the triggering instruction pointer without reading the LSTAR MSR
    // or ever even executing the syscall handler.
    // We take the return instruction pointer already placed in RCX to skip it
    // altogether.
    irq_info.fixup_fn = [](intr_regs* regs) {
        if (regs->vector == static_cast<unsigned>(x86::exception::DB)) {
            regs->flags &= ~x86::FLAGS_TF;
            regs->rip = regs->rcx;
        }
    };

    asm volatile("pushfq;"
                 "orq %[tf], (%%rsp);"
                 "popfq;"
                 "syscall;" ::[tf] "i"(x86::FLAGS_TF)
                 : "rax", "rcx", "r11", "memory");

    wrmsr(x86::LSTAR, lstar_original);
    wrmsr(x86::FMASK, fmask_original);

    BARETEST_VERIFY(baretest::expectation<unsigned>(static_cast<unsigned>(x86::exception::DB)) == irq_info.vec);
    const auto faulting_rip{ irq_info.rip };
    BARETEST_VERIFY(baretest::expectation<uintptr_t>(FAKE_LSTAR) == faulting_rip);
}

BARETEST_RUN;
