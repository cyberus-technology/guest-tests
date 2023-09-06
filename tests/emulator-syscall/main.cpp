/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include "helpers.hpp"

#include <baretest/baretest.hpp>
#include <compiler.hpp>
#include <debugport_interface.h>
#include <idt.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <cbl/trace.hpp>
#include <usermode.hpp>

#include <tuple>

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
} // namespace x86

static usermode_helper um;

EXTERN_C uint64_t syscall_handler(uint64_t param0, uint64_t param1)
{
    return param0 + param1;
}

uint64_t emulated_syscall(uint64_t param0, uint64_t param1)
{
    uint64_t res;
    asm volatile("outb %[dbgport]; syscall"
                 : "=a"(res)
                 : [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), "D"(param0), "S"(param1)
                 : "memory");
    return res;
}

EXTERN_C void syscall_entry();

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
    const x86::segment_selector tss_selector {x86::str()};
    assert(tss_selector.raw != 0);
    x86::gdt_entry* gdte {x86::get_gdt_entry(get_current_gdtr(), tss_selector)};
    x86::tss* current_tss {reinterpret_cast<x86::tss*>(gdte->base())};
    current_tss->ist4 = 0xC01000;

    // By using the IST mechanism, we can make sure that the first time this
    // stack pointer is ever used is when we actually trigger this exception.
    auto& db_entry {global_idt.entries[static_cast<uint8_t>(x86::exception::DB)]};
    db_entry.set_ist(4);

    const auto lstar_original {rdmsr(x86::LSTAR)};
    const auto fmask_original {rdmsr(x86::FMASK)};

    constexpr uintptr_t FAKE_LSTAR {0x1337F000};
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
    const auto faulting_rip {irq_info.rip};
    BARETEST_VERIFY(baretest::expectation<uintptr_t>(FAKE_LSTAR) == faulting_rip);
}

TEST_CASE(syscall)
{
    static constexpr const uint64_t param0 {0xcafe};
    static constexpr const uint64_t param1 {0xcafe};

    baretest::expectation<uint64_t> expected {param0 + param1};

    uint64_t res;

    um.enter_sysret();

    // User-mode code here
    res = emulated_syscall(param0, param1);

    um.leave_syscall();

    BARETEST_VERIFY(expected == res);
}

TEST_CASE(sysret)
{
    um.enter_sysret(true);
    um.leave_syscall();
}

TEST_CASE(iret)
{
    um.enter_iret(true);
    um.leave_syscall();
}

TEST_CASE(iret_flags_rf_ac_id)
{
    constexpr uint64_t flags_to_clear {x86::FLAGS_RF | x86::FLAGS_AC | x86::FLAGS_ID};
    set_rflags(flags_to_clear);

    // Enter via emulated IRET, clearing the flags
    um.enter_iret(true, 0, flags_to_clear);
    uint64_t user_flags {get_rflags()};
    um.leave_syscall();

    BARETEST_VERIFY(not(user_flags & flags_to_clear));
}

TEST_CASE(sgdt)
{
    baretest::expectation<x86::descriptor_ptr> native {sgdt(false)};
    BARETEST_VERIFY(native == sgdt(true));
}

TEST_CASE(sidt)
{
    baretest::expectation<x86::descriptor_ptr> native {sidt(false)};
    BARETEST_VERIFY(native == sidt(true));
}

TEST_CASE(sldt)
{
    baretest::expectation<uint16_t> native {sldt(false)};
    BARETEST_VERIFY(native == sldt(true));
}

TEST_CASE(str)
{
    baretest::expectation<uint16_t> native {str(false)};
    BARETEST_VERIFY(native == str(true));
}

TEST_CASE(lgdt)
{
    auto current {sgdt(false)};
    baretest::expectation<x86::descriptor_ptr> expected {current};

    lgdt(current, true);
    auto after {sgdt(false)};

    BARETEST_VERIFY(expected == after);
}

TEST_CASE(lidt)
{
    auto current {sidt(false)};
    baretest::expectation<x86::descriptor_ptr> expected {current};

    lidt(current, true);
    auto after {sidt(false)};

    BARETEST_VERIFY(expected == after);
}

TEST_CASE(lldt)
{
    auto current {sldt(false)};
    baretest::expectation<uint16_t> expected {current};

    lldt(current, true);
    auto after {sldt(false)};

    BARETEST_VERIFY(expected == after);
}

template <typename T>
void test_ljmp()
{
    struct __PACKED__
    {
        T ip;
        uint16_t cs;
    } ptr {0, 0};
    uintptr_t ptr_addr {reinterpret_cast<uintptr_t>(&ptr)};

    asm volatile(
        // Prepare memory operand for far jump (CS:{E/R}IP)
        "mov %%cs, %%rbx;"
        "lea 2f, %%rcx;"
        "mov %%rdi, %%r14;"
        "mov %%ecx, (%%rdi);"
        "add %[opsize], %%rdi;"
        "mov %%bx, (%%rdi);"

        // Check if we need a REX prefix
        "mov %[opsize], %%rdi;"
        "cmp $8, %%rdi;"
        "je 1f;"

        // 32-bit operand size
        "outb %[dbgport];"
        "ljmp *(%%r14);"

        // 64-bit operand size
        "1: outb %[dbgport];"

#ifdef __clang__
        "ljmpq *(%%r14);"
#else
        "rex64 ljmp *(%%r14);"
#endif

        // This instruction needs to be skipped by the far jump
        "ud2a;"
        "2:"
        : "+D"(ptr_addr)
        : [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [opsize] "i"(sizeof(T))
        : "rbx", "rcx", "r14", "memory");
}

TEST_CASE(ljmp_32bit)
{
    test_ljmp<uint32_t>();
}

TEST_CASE(ljmp_64bit)
{
    test_ljmp<uint64_t>();
}

EXTERN_C void far_jmp_ptr();

TEST_CASE(ljmp_compat)
{
    static struct __PACKED__
    {
        uint32_t ip {static_cast<uint32_t>(reinterpret_cast<uintptr_t>(far_jmp_ptr))};
        uint16_t cs {0x1b};
    } jmp_target;

    um.enter_sysret();

    asm volatile("lcall *(%[fn]);" ::[fn] "r"(&jmp_target), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE)
                 : "memory");

    um.leave_syscall();
}

BARETEST_RUN;
