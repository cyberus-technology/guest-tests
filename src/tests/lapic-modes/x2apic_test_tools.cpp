// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "x2apic_test_tools.hpp"

#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/x86/x86defs.hpp>

using namespace lapic_test_tools;
using namespace x86;

uint64_t x2apic_test_tools::read_x2_msr(uint32_t msr)
{
    // The RDMSR instruction is not serializing and this behavior is unchanged when reading
    // APIC registers in x2APIC mode.
    asm("mfence; lfence");
    return rdmsr(msr);
}

void x2apic_test_tools::write_x2_msr(uint32_t msr, uint64_t value)
{
    // A WRMSR to an APIC register may complete before all preceding stores are globally visible;
    // software can prevent this by inserting a serializing instruction or the sequence
    // MFENCE;LFENCE before the WRMSR.
    asm("mfence; lfence");
    wrmsr(msr, value);
}

void x2apic_test_tools::x2apic_mode_enable()
{
    wrmsr(msr::IA32_APIC_BASE, rdmsr(msr::IA32_APIC_BASE) | (X2APIC_ENABLED_MASK << X2APIC_ENABLED_SHIFT));
}

void x2apic_test_tools::x2apic_mode_disable()
{
    // software has to guarantee that no interrupt is sent to the apic
    // while the xapic is disabled (see vol 3a, 10-8, p. 368)
    int_guard _;

    // disable x2apic mode and xapic
    auto msr_val{ rdmsr(msr::IA32_APIC_BASE) };
    msr_val &= ~(APIC_GLOBAL_ENABLED_MASK << APIC_GLOBAL_ENABLED_SHIFT);
    msr_val &= ~(X2APIC_ENABLED_MASK << X2APIC_ENABLED_SHIFT);
    write_x2_msr(msr::IA32_APIC_BASE, msr_val);

    // enable xapic
    global_apic_enable();
    software_apic_enable();
}

bool x2apic_test_tools::x2apic_mode_enabled()
{
    return rdmsr(msr::IA32_APIC_BASE) & (X2APIC_ENABLED_MASK << X2APIC_ENABLED_SHIFT);
}

bool x2apic_test_tools::x2apic_mode_supported()
{
    static constexpr uint32_t CPUID_BASIC_01{ 1 };
    static constexpr uint32_t ECX_X2APIC_SHIFT{ 21 };
    static constexpr uint32_t ECX_X2APIC_MASK{ 1u << ECX_X2APIC_SHIFT };
    return cpuid(CPUID_BASIC_01).ecx & ECX_X2APIC_MASK;
}
