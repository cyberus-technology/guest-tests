/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
#include <toyos/testhelper/debugport_interface.h>
#include <toyos/x86/segmentation.hpp>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86defs.hpp>

EXTERN_C void syscall_entry();

class usermode_helper
{
    static constexpr size_t KERNEL_STACK_PAGES{ 1 };
    static constexpr size_t KERNEL_STACK_SIZE{ KERNEL_STACK_PAGES * PAGE_SIZE };

 public:
    usermode_helper()
    {
        enable_sce();

        // Set LSTAR to syscall entry IP
        wrmsr(x86::LSTAR, uintptr_t(syscall_entry));

        // Disable interrupts in syscall path
        wrmsr(x86::FMASK, x86::FLAGS_IF);

        // Configure STAR to hold the GDT indexes
        wrmsr(x86::STAR, (0x18ull << 48) | (0x8ull << 32));

        // TSS is expected to be set up and have correct size
        x86::segment_selector tss_selector{ x86::str() };
        assert(tss_selector.raw != 0);
        x86::gdt_entry* gdte{ x86::get_gdt_entry(get_current_gdtr(), tss_selector) };
        assert(sizeof(x86::tss) == gdte->limit());
        x86::tss* current_tss{ reinterpret_cast<x86::tss*>(gdte->base()) };

        current_tss->rsp0 = uintptr_t(&kernel_stack_[KERNEL_STACK_SIZE]);
    }

    void enter_sysret(bool emulated = false)
    {
        asm volatile("lea 1f, %%rcx;"
                     "pushf;"
                     "pop %%r11;"
                     "cmp $0, %[emul];"
                     "je 2f;"
                     "outb %[dbgport];"
                     "2: sysretq;"
                     "1:"
                     :
                     : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE)
                     : "rcx", "r11", "memory");
    }

    void enter_iret(bool emulated = false, uint64_t set_flags = 0, uint64_t clear_flags = 0)
    {
        asm volatile("mov %%rsp, %%rbx;"
                     "pushq $0x23;"  // Push usermode SS
                     "pushq %%rbx;"  // Push future SP

                     "pushf;"  // Push and modify FLAGS
                     "pop %%rcx;"
                     "or %[set_flags], %%rcx;"
                     "and %[clear_mask], %%rcx;"
                     "pushq %%rcx;"

                     "pushq $0x2b;"  // Push 64-bit usermode CS
                     "pushq $1f;"    // Push continuation IP
                     "cmp $0, %[emul];"
                     "je 2f;"
                     "outb %[dbgport];"
                     "2: iretq;"
                     "1:"
                     :
                     : [emul] "r"(emulated), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE), [set_flags] "rm"(set_flags), [clear_mask] "rm"(~clear_flags)
                     : "rbx", "rcx", "memory");
    }

    void leave_syscall()
    {
        // Providing syscall parameter 0 will cause the handler to stay in kernel mode
        asm volatile("pushf; lea 1f, %%rsi; syscall; 1: popf" ::"D"(0)
                     : "rsi", "rcx", "r11", "memory");
    }

 private:
    alignas(PAGE_SIZE) uint8_t kernel_stack_[KERNEL_STACK_SIZE];
    void enable_sce()
    {
        wrmsr(x86::EFER, rdmsr(x86::EFER) | x86::EFER_SCE);
    }
};
