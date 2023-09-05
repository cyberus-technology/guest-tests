/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <x86defs.hpp>

namespace vcpu
{

namespace registers
{

template <typename SET_MTD_OUT_FN>
inline void write(hedron::utcb& utcb, x86::gpr reg, uint64_t value, SET_MTD_OUT_FN set_mtd_out)
{
    ASSERT(utcb.mtd.contains(hedron::gpr_to_mtd_bit(reg)), "register not transferred");

    set_mtd_out(hedron::gpr_to_mtd_bit(reg));

    if (reg == x86::gpr::IP or reg == x86::gpr::CUR_IP) {
        utcb.ip = value;
        return;
    }

    auto field = utcb_field_from_reg(utcb, reg);

    // Writing 32-bit GPRs always clears the upper half of the register in 64-bit mode
    // When switching between CPU modes, the upper half is undefined, we can just always clear it.
    if (field.width == 32) {
        *field.ptr &= math::mask(32);
    }

    *field.ptr &= ~math::mask(field.width, field.offset);
    *field.ptr |= (value & math::mask(field.width)) << field.offset;
}

inline uint64_t read(const hedron::utcb& utcb, x86::gpr reg)
{
    using x86::gpr;

    if (reg == gpr::IP) {
        return utcb.ip + utcb.ins_len;
    } else if (reg == gpr::CUR_IP) {
        return utcb.ip;
    }

    auto field = utcb_field_from_reg(utcb, reg);
    return (*field.ptr & math::mask(field.width, field.offset)) >> field.offset;
}

inline uint64_t read(const hedron::utcb& utcb, x86::control_register reg)
{
    return *utcb_field_from_control_reg(utcb, reg);
}

inline x86::cpu_mode get_cpu_mode(const hedron::utcb& utcb)
{
    // See http://sandpile.org/x86/mode.htm for details

    ASSERT(utcb.mtd.contains(hedron::mtd_bits::EFER_PAT), "inbound EFER missing");
    ASSERT(utcb.mtd.contains(hedron::mtd_bits::CSSS), "inbound CSSS missing");
    ASSERT(utcb.mtd.contains(hedron::mtd_bits::EFL), "inbound EFL  missing");

    x86::gdt_segment cs {.sel = utcb.cs.sel, .ar = utcb.cs.ar, .limit = utcb.cs.limit, .base = utcb.cs.base};

    return x86::determine_cpu_mode(cs, utcb.cr0, utcb.efer, utcb.rflags);
}

inline unsigned current_cpl(const hedron::utcb& utcb)
{
    ASSERT(utcb.mtd.contains(hedron::mtd_bits::CSSS), "CS/SS not transferred inbound");

    return x86::determine_cpl(utcb.ss.sel);
}

template <typename SET_MTD_OUT_FN>
inline void update_ip(hedron::utcb& utcb, uint64_t intr_blocking, bool ip_forced, SET_MTD_OUT_FN set_mtd_out)
{
    ASSERT(utcb.mtd.contains(hedron::mtd_bits::EIP), "instruction pointer not transferred inbound");

    // We always clear sti and mov ss blocking. The vcpu informs us about the
    // interrupt blocking to enable via the bits set in the intr_blocking
    // parameter.
    utcb.int_state.clear_sti_blocking();
    utcb.int_state.clear_mov_ss_blocking();

    if (intr_blocking & hedron::intr_state::STI_BLOCKING) {
        utcb.int_state.set_sti_blocking();
    }

    if (intr_blocking & hedron::intr_state::MOV_SS_BLOCKING) {
        utcb.int_state.set_mov_ss_blocking();
    }

    set_mtd_out(hedron::mtd_bits::field::STA);

    // If somebody forced the IP to a certain value, don't mess with it!
    if (not ip_forced) {
        utcb.ip += utcb.ins_len;
        set_mtd_out(hedron::mtd_bits::field::EIP);
    }
}

} // namespace registers

} // namespace vcpu
