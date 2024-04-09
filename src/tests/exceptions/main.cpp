/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_enabler.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>
#include <toyos/util/cpuid.hpp>
#include <toyos/util/trace.hpp>
#include <toyos/x86/x86asm.hpp>

using x86::exception;

static irqinfo irq_info;

// We need this for interrupt draining.
// If we don't set a sane base vector, we might be getting interrupts with the
// vector 8, which looks like a double fault. Those are expected to have an
// error code pushed to the stack, which then causes a messed up stack frame
// when we return from handling it.
//
// The pic object also masks all PIC pins afterwards.
[[maybe_unused]] static pic global_pic{ 0x30 };

static void irq_handler_fn(intr_regs* regs)
{
    lapic_test_tools::send_eoi();

    irq_info.record(regs->vector, regs->error_code);
    irq_info.fixup(regs);
}

TEST_CASE(test_ud)
{
    irq_info.reset();
    irqinfo::func_t fixup = [](intr_regs* regs) { regs->rip = regs->rcx; };
    irq_info.fixup_fn = fixup;

    asm volatile("lea 1f, %%rcx; ud2a; 1:" ::
                     : "rcx");

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == static_cast<unsigned>(exception::UD));
}

TEST_CASE(test_int3)
{
    irq_info.reset();

    // Execute emulated instruction with different length to test for correct
    // instruction length injection
    cpuid(0);

    uint32_t result{ 0 };
    asm volatile("int3; mov $1, %[result];"
                 : [result] "=r"(result)::"memory");

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(result == 1);
    BARETEST_ASSERT(irq_info.vec == static_cast<unsigned>(exception::BP));
}

static const constexpr uint8_t SELF_IPI_VECTOR{ 0x42 };
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

    lapic_enabler lenabler{};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    uint64_t dummy{ 0 };
    asm volatile("sti;"
                 "cli;"
                 : "=a"(dummy)
                 :
                 : "rbx", "rcx", "rdx");

    BARETEST_ASSERT(not irq_info.valid);
}

static uint64_t interrupted_rip{ 0 };

TEST_CASE(test_sti_blocking_with_cpuid)
{
    irq_info.reset();

    irq_info.fixup_fn = [](intr_regs* regs) { interrupted_rip = regs->rip; };

    lapic_enabler lenabler{};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    uint64_t protected_rip{ 0 };
    asm volatile("lea 1f, %[protected_rip];"
                 "sti;"
                 "1: cpuid; cpuid;"
                 "cli;"
                 : [protected_rip] "=&r"(protected_rip)
                 :
                 : "rax", "rbx", "rcx", "rdx");

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == SELF_IPI_VECTOR);
    BARETEST_ASSERT(interrupted_rip != protected_rip);
}

// AMD CPUs don't inhibit interrupts throughout a sequence of
//     STI - MOV SS - CLI
// Even though no official documentation states this, the behavior
// is not guaranteed and our AMD machines fail here.
// So far, we only verified this behavior on Intel CPUs.
// See also: https://c9x.me/x86/html/file_module_x86_id_304.html
TEST_CASE_CONDITIONAL(test_mov_ss_blocking, util::cpuid::is_intel_cpu())
{
    irq_info.reset();

    lapic_enabler lenabler{};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    asm volatile("mov %%ss, %%bx;"
                 "sti;"
                 "mov %%bx, %%ss;"
                 "cli;"
                 :
                 :
                 : "rbx");

    BARETEST_ASSERT(not irq_info.valid);
}

TEST_CASE(test_mov_ss_blocking_with_cpuid)
{
    irq_info.reset();

    irq_info.fixup_fn = [](intr_regs* regs) { interrupted_rip = regs->rip; };

    lapic_enabler lenabler{};
    send_self_ipi_and_poll(SELF_IPI_VECTOR);

    // The forced emulation mechanism is not ready for injecting interrupts,
    // because we didn't implement emulated injection. So we run this test
    // a little differently instead:
    // We let the instruction directly after "mov ss" be one that exits to
    // the VMM to check if we handle that condition correctly.

    uint64_t protected_rip{ 0 };
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

void prologue()
{
    irq_handler::set(irq_handler_fn);

    // Drain IRQs
    enable_interrupts_for_single_instruction();
}

BARETEST_RUN;
