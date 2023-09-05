/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <baretest/baretest.hpp>
#include <debugport_interface.h>
#include <idt.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <lapic_enabler.hpp>
#include <lapic_test_tools.hpp>
#include <pic.hpp>
#include <sotest.hpp>
#include <trace.hpp>
#include <usermode.hpp>
#include <x86asm.hpp>

using x86::exception;

static usermode_helper um;

static irqinfo irq_info;

// We need this for interrupt draining.
// If we don't set a sane base vector, we might be getting interrupts with the
// vector 8, which looks like a double fault. Those are expected to have an
// error code pushed to the stack, which then causes a messed up stack frame
// when we return from handling it.
//
// The pic object also masks all PIC pins afterwards.
static pic global_pic {0x30};

static void irq_handler_fn(intr_regs* regs)
{
    lapic_test_tools::send_eoi();

    irq_info.record(regs->vector, regs->error_code);
    irq_info.fixup(regs);
}

static void test_ud(bool with_exit)
{
    irq_info.reset();
    irqinfo::func_t fixup = [](intr_regs* regs) { regs->rip = regs->rcx; };
    irq_info.fixup_fn = fixup;

    if (with_exit) {
        enable_exc_exit(exception::UD);
    }

    asm volatile("lea 1f, %%rcx; ud2a; 1:" ::: "rcx");

    if (with_exit) {
        disable_exc_exit(exception::UD);
    }

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == static_cast<unsigned>(exception::UD));
}

TEST_CASE(test_ud_without_exit)
{
    test_ud(false);
}

TEST_CASE(test_ud_with_exit)
{
    test_ud(true);
}

static void test_int3(bool with_exit)
{
    irq_info.reset();

    if (with_exit) {
        enable_exc_exit(exception::BP);
    }

    // Execute emulated instruction with different length to test for correct
    // instruction length injection
    cpuid(0);

    uint32_t result {0};
    asm volatile("int3; mov $1, %[result];" : [result] "=r"(result)::"memory");

    if (with_exit) {
        disable_exc_exit(exception::BP);
    }

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(result == 1);
    BARETEST_ASSERT(irq_info.vec == static_cast<unsigned>(exception::BP));
}

TEST_CASE(test_int3_without_exit)
{
    test_int3(false);
}

TEST_CASE(test_int3_with_exit)
{
    test_int3(true);
}

static const constexpr uint8_t SELF_IPI_VECTOR {0x42};
static void send_self_ipi_and_poll(uint8_t ipi_vec)
{
    lapic_test_tools::send_self_ipi(ipi_vec);

    while (not lapic_test_tools::check_irr(ipi_vec)) {
        cpu_pause();
    }
}

TEST_CASE(test_sti_blocking)
{
    irq_info.reset();

    lapic_enabler lenabler {};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    uint64_t dummy {0};
    asm volatile("outb %[dbgport];"
                 "sti;"
                 "cli;"
                 : "=a"(dummy)
                 : "a"(DEBUGPORT_EMUL_ONCE), [dbgport] "i"(DEBUG_PORTS.a)
                 : "rbx", "rcx", "rdx");

    BARETEST_ASSERT(not irq_info.valid);
}

static uint64_t interrupted_rip {0};

TEST_CASE(test_sti_blocking_with_cpuid)
{
    irq_info.reset();

    irq_info.fixup_fn = [](intr_regs* regs) { interrupted_rip = regs->rip; };

    lapic_enabler lenabler {};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    uint64_t protected_rip {0};
    asm volatile("lea 1f, %[protected_rip];"
                 "mov %[emul_once], %%ax;"
                 "outb %[dbgport];"
                 "sti;"
                 "1: cpuid; cpuid;"
                 "cli;"
                 : [protected_rip] "=&r"(protected_rip)
                 : [emul_once] "i"(DEBUGPORT_EMUL_ONCE), [dbgport] "i"(DEBUG_PORTS.a)
                 : "rax", "rbx", "rcx", "rdx");

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == SELF_IPI_VECTOR);
    BARETEST_ASSERT(interrupted_rip != protected_rip);
}

TEST_CASE(test_mov_ss_blocking)
{
    irq_info.reset();

    lapic_enabler lenabler {};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    asm volatile("outb %[dbgport];"
                 "mov %%ss, %%bx;"
                 "sti;"
                 "mov %%bx, %%ss;"
                 "cli;"
                 "mov %[end], %%ax;"
                 "outb %[dbgport];" ::"a"(DEBUGPORT_EMUL_START),
                 [end] "i"(DEBUGPORT_EMUL_END), [dbgport] "i"(DEBUG_PORTS.a)
                 : "rbx");

    BARETEST_ASSERT(not irq_info.valid);
}

TEST_CASE(test_mov_ss_blocking_with_cpuid)
{
    irq_info.reset();

    irq_info.fixup_fn = [](intr_regs* regs) { interrupted_rip = regs->rip; };

    lapic_enabler lenabler {};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    // The forced emulation mechanism is not ready for injecting interrupts,
    // because we didn't implement emulated injection. So we run this test
    // a little differently instead:
    // We let the instruction directly after "mov ss" be one that exits to
    // the VMM to check if we handle that condition correctly.

    uint64_t protected_rip {0};
    asm volatile("lea 1f, %[protected_rip];"
                 "mov %%ss, %%bx;"
                 "sti;"
                 "mov %%bx, %%ss;"
                 "1: cpuid;"
                 "cpuid;"
                 "cli;"
                 : [protected_rip] "=&r"(protected_rip)::"rax", "rbx", "rcx", "rdx");

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == SELF_IPI_VECTOR);
    BARETEST_ASSERT(interrupted_rip != protected_rip);
}

TEST_CASE(test_ac_exception)
{
    set_cr0(get_cr0() | math::mask_from(x86::cr0::AM));

    static const uint32_t test_variable {0xcafe};
    auto unaligned_pointer {reinterpret_cast<uintptr_t>(&test_variable) + 1};

    irq_info.reset();
    irqinfo::func_t fixup = [](intr_regs* regs) { regs->rip = regs->rcx; };
    irq_info.fixup_fn = fixup;

    um.enter_sysret();

    set_rflags(x86::FLAGS_AC);

    asm volatile("lea 1f, %%rcx;"
                 "cmpw $0, (%[unaligned_pointer]);"
                 "1:" ::[unaligned_pointer] "r"(unaligned_pointer)
                 : "rcx");

    clear_rflags(x86::FLAGS_AC);

    um.leave_syscall();

    set_cr0(get_cr0() & ~math::mask_from(x86::cr0::AM));

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == static_cast<uint8_t>(exception::AC));
}

void prologue()
{
    irq_handler::set(irq_handler_fn);

    // Drain IRQs
    enable_interrupts_for_single_instruction();
}

BARETEST_RUN;
