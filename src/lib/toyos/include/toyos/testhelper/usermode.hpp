/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
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

 private:
    alignas(PAGE_SIZE) uint8_t kernel_stack_[KERNEL_STACK_SIZE]{};
    void enable_sce()
    {
        wrmsr(x86::EFER, rdmsr(x86::EFER) | x86::EFER_SCE);
    }
};
