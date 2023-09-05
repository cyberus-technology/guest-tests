/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <x86defs.hpp>

#include "utcb.hpp"

namespace hedron
{

struct utcb_field
{
    uint64_t* ptr;
    size_t width;
    size_t offset;
};

struct utcb_field_const
{
    const uint64_t* ptr;
    size_t width;
    size_t offset;
};

template <typename RET, typename UTCB>
inline RET utcb_field_from_reg_helper(UTCB u, x86::gpr reg)
{
    using x86::gpr;

    switch (reg) {
    // 8-bit
    case gpr::AL: return {&u.ax, 8, 0};
    case gpr::BL: return {&u.bx, 8, 0};
    case gpr::CL: return {&u.cx, 8, 0};
    case gpr::DL: return {&u.dx, 8, 0};

    case gpr::AH: return {&u.ax, 8, 8};
    case gpr::BH: return {&u.bx, 8, 8};
    case gpr::CH: return {&u.cx, 8, 8};
    case gpr::DH: return {&u.dx, 8, 8};

    case gpr::SIL: return {&u.si, 8, 0};
    case gpr::DIL: return {&u.di, 8, 0};
    case gpr::SPL: return {&u.sp, 8, 0};
    case gpr::BPL: return {&u.bp, 8, 0};

    case gpr::R8L: return {&u.r8, 8, 0};
    case gpr::R9L: return {&u.r9, 8, 0};
    case gpr::R10L: return {&u.r10, 8, 0};
    case gpr::R11L: return {&u.r11, 8, 0};
    case gpr::R12L: return {&u.r12, 8, 0};
    case gpr::R13L: return {&u.r13, 8, 0};
    case gpr::R14L: return {&u.r14, 8, 0};
    case gpr::R15L: return {&u.r15, 8, 0};

    // 16-bit
    case gpr::AX: return {&u.ax, 16, 0};
    case gpr::BX: return {&u.bx, 16, 0};
    case gpr::CX: return {&u.cx, 16, 0};
    case gpr::DX: return {&u.dx, 16, 0};

    case gpr::SI: return {&u.si, 16, 0};
    case gpr::DI: return {&u.di, 16, 0};
    case gpr::SP: return {&u.sp, 16, 0};
    case gpr::BP: return {&u.bp, 16, 0};

    case gpr::R8W: return {&u.r8, 16, 0};
    case gpr::R9W: return {&u.r9, 16, 0};
    case gpr::R10W: return {&u.r10, 16, 0};
    case gpr::R11W: return {&u.r11, 16, 0};
    case gpr::R12W: return {&u.r12, 16, 0};
    case gpr::R13W: return {&u.r13, 16, 0};
    case gpr::R14W: return {&u.r14, 16, 0};
    case gpr::R15W: return {&u.r15, 16, 0};

    // 32-bit
    case gpr::EAX: return {&u.ax, 32, 0};
    case gpr::EBX: return {&u.bx, 32, 0};
    case gpr::ECX: return {&u.cx, 32, 0};
    case gpr::EDX: return {&u.dx, 32, 0};

    case gpr::ESI: return {&u.si, 32, 0};
    case gpr::EDI: return {&u.di, 32, 0};
    case gpr::ESP: return {&u.sp, 32, 0};
    case gpr::EBP: return {&u.bp, 32, 0};

    case gpr::R8D: return {&u.r8, 32, 0};
    case gpr::R9D: return {&u.r9, 32, 0};
    case gpr::R10D: return {&u.r10, 32, 0};
    case gpr::R11D: return {&u.r11, 32, 0};
    case gpr::R12D: return {&u.r12, 32, 0};
    case gpr::R13D: return {&u.r13, 32, 0};
    case gpr::R14D: return {&u.r14, 32, 0};
    case gpr::R15D: return {&u.r15, 32, 0};

    // 64-bit
    case gpr::RAX: return {&u.ax, 64, 0};
    case gpr::RBX: return {&u.bx, 64, 0};
    case gpr::RCX: return {&u.cx, 64, 0};
    case gpr::RDX: return {&u.dx, 64, 0};

    case gpr::RSI: return {&u.si, 64, 0};
    case gpr::RDI: return {&u.di, 64, 0};
    case gpr::RSP: return {&u.sp, 64, 0};
    case gpr::RBP: return {&u.bp, 64, 0};

    case gpr::R8: return {&u.r8, 64, 0};
    case gpr::R9: return {&u.r9, 64, 0};
    case gpr::R10: return {&u.r10, 64, 0};
    case gpr::R11: return {&u.r11, 64, 0};
    case gpr::R12: return {&u.r12, 64, 0};
    case gpr::R13: return {&u.r13, 64, 0};
    case gpr::R14: return {&u.r14, 64, 0};
    case gpr::R15: return {&u.r15, 64, 0};

    case gpr::FLAGS: return {&u.rflags, 64, 0};

    default: PANIC("Unknown register.");
    }
}

template <typename RET = utcb_field>
inline RET utcb_field_from_reg(hedron::utcb& u, x86::gpr reg)
{
    return utcb_field_from_reg_helper<RET, decltype(u)>(u, reg);
}

template <typename RET = utcb_field_const>
inline RET utcb_field_from_reg(const hedron::utcb& u, x86::gpr reg)
{
    return utcb_field_from_reg_helper<RET, decltype(u)>(u, reg);
}

template <typename RET, typename UTCB>
inline RET* utcb_field_from_control_reg_helper(UTCB u, x86::control_register creg)
{
    using x86::control_register;

    switch (creg) {
    case control_register::CR0: return &u.cr0;
    case control_register::CR2: return &u.cr2;
    case control_register::CR3: return &u.cr3;
    case control_register::CR4: return &u.cr4;
    case control_register::XCR0: return &u.xcr0;
    default: PANIC("invalid control register");
    }

    __UNREACHED__;
}

template <typename RET = uint64_t>
inline RET* utcb_field_from_control_reg(hedron::utcb& u, x86::control_register creg)
{
    return utcb_field_from_control_reg_helper<RET, decltype(u)>(u, creg);
}

template <typename RET = const uint64_t>
inline RET* utcb_field_from_control_reg(const hedron::utcb& u, x86::control_register creg)
{
    return utcb_field_from_control_reg_helper<RET, decltype(u)>(u, creg);
}

struct reg_info_segment
{
    hedron::segment_descriptor* desc;
    hedron::mtd_bits::field field;
};

inline reg_info_segment to_utcb(x86::segment_register reg, hedron::utcb& utcb)
{
    using x86::segment_register;

    switch (reg) {
    case segment_register::CS: return {&utcb.cs, hedron::mtd_bits::field::CSSS};
    case segment_register::DS: return {&utcb.ds, hedron::mtd_bits::field::DSES};
    case segment_register::ES: return {&utcb.es, hedron::mtd_bits::field::DSES};
    case segment_register::FS: return {&utcb.fs, hedron::mtd_bits::field::FSGS};
    case segment_register::GS: return {&utcb.gs, hedron::mtd_bits::field::FSGS};
    case segment_register::SS: return {&utcb.ss, hedron::mtd_bits::field::CSSS};
    }
    __UNREACHED__;
}

inline reg_info_segment to_utcb(x86::mmreg reg, hedron::utcb& utcb)
{
    using x86::mmreg;

    switch (reg) {
    case mmreg::GDTR: return {&utcb.gdtr, hedron::mtd_bits::field::GDTR};
    case mmreg::IDTR: return {&utcb.idtr, hedron::mtd_bits::field::IDTR};
    case mmreg::LDTR: return {&utcb.ldtr, hedron::mtd_bits::field::LDTR};
    case mmreg::TR: return {&utcb.tr, hedron::mtd_bits::field::TR};
    }
    __UNREACHED__;
}

inline hedron::mtd_bits::field gpr_to_mtd_bit(x86::gpr reg)
{
    using x86::gpr;

    switch (reg) {
    case gpr::AL:
    case gpr::BL:
    case gpr::CL:
    case gpr::DL:
    case gpr::AH:
    case gpr::BH:
    case gpr::CH:
    case gpr::DH:
    case gpr::AX:
    case gpr::BX:
    case gpr::CX:
    case gpr::DX:
    case gpr::EAX:
    case gpr::EBX:
    case gpr::ECX:
    case gpr::EDX:
    case gpr::RAX:
    case gpr::RBX:
    case gpr::RCX:
    case gpr::RDX: return hedron::mtd_bits::ACDB;
    case gpr::SIL:
    case gpr::DIL:
    case gpr::BPL:
    case gpr::SI:
    case gpr::DI:
    case gpr::BP:
    case gpr::ESI:
    case gpr::EDI:
    case gpr::EBP:
    case gpr::RSI:
    case gpr::RDI:
    case gpr::RBP: return hedron::mtd_bits::BSD;
    case gpr::SPL:
    case gpr::SP:
    case gpr::ESP:
    case gpr::RSP: return hedron::mtd_bits::ESP;
    case gpr::R8L:
    case gpr::R9L:
    case gpr::R10L:
    case gpr::R11L:
    case gpr::R8D:
    case gpr::R9D:
    case gpr::R10D:
    case gpr::R11D:
    case gpr::R8W:
    case gpr::R9W:
    case gpr::R10W:
    case gpr::R11W:
    case gpr::R8:
    case gpr::R9:
    case gpr::R10:
    case gpr::R11:
    case gpr::R12L:
    case gpr::R13L:
    case gpr::R14L:
    case gpr::R15L:
    case gpr::R12D:
    case gpr::R13D:
    case gpr::R14D:
    case gpr::R15D:
    case gpr::R12W:
    case gpr::R13W:
    case gpr::R14W:
    case gpr::R15W:
    case gpr::R12:
    case gpr::R13:
    case gpr::R14:
    case gpr::R15: return hedron::mtd_bits::GPR_R8_R15;
    case gpr::IP:
    case gpr::CUR_IP: return hedron::mtd_bits::EIP;
    case gpr::FLAGS: return hedron::mtd_bits::EFL;
    }

    __UNREACHED__;
}

} // namespace hedron
