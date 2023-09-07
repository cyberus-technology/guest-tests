/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/x86/arch.hpp>
#include <toyos/decoder/gdt.hpp>

using x86::mmreg;
using x86::segment_register;

namespace decoder
{

x86::gdt_entry parse_gdt(decoder_vcpu& vcpu, uint16_t selector, bool is_64b_system)
{
    auto gdtr = vcpu.read(mmreg::GDTR);
    uint64_t entry {0}, entry_high {0};

    access_override ovrd {true};

    PANIC_UNLESS(vcpu.read(log_addr_t(segment_register::DS, gdtr.base + selector), 8, &entry,
                           x86::memory_access_type_t::READ, ovrd),
                 "Could not read GDT");

    if (is_64b_system) {
        PANIC_UNLESS(vcpu.read(log_addr_t(segment_register::DS, gdtr.base + selector + 8), 8, &entry_high,
                               x86::memory_access_type_t::READ, ovrd),
                     "Could not read upper 64bits of system descriptor");
    }

    return {entry, entry_high};
}

} // namespace decoder
