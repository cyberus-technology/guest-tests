/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <decoder/instructions.hpp>
#include <udis86.h>
#include <x86defs.hpp>

namespace decoder
{

inline x86::gpr to_gpr(ud_type t)
{
    switch (t) {
    case UD_R_AL: return x86::gpr::AL;
    case UD_R_CL: return x86::gpr::CL;
    case UD_R_DL: return x86::gpr::DL;
    case UD_R_BL: return x86::gpr::BL;
    case UD_R_AH: return x86::gpr::AH;
    case UD_R_CH: return x86::gpr::CH;
    case UD_R_DH: return x86::gpr::DH;
    case UD_R_BH: return x86::gpr::BH;
    case UD_R_SPL: return x86::gpr::SPL;
    case UD_R_BPL: return x86::gpr::BPL;
    case UD_R_SIL: return x86::gpr::SIL;
    case UD_R_DIL: return x86::gpr::DIL;
    case UD_R_R8B: return x86::gpr::R8L;
    case UD_R_R9B: return x86::gpr::R9L;
    case UD_R_R10B: return x86::gpr::R10L;
    case UD_R_R11B: return x86::gpr::R11L;
    case UD_R_R12B: return x86::gpr::R12L;
    case UD_R_R13B: return x86::gpr::R13L;
    case UD_R_R14B: return x86::gpr::R14L;
    case UD_R_R15B: return x86::gpr::R15L;

    case UD_R_AX: return x86::gpr::AX;
    case UD_R_CX: return x86::gpr::CX;
    case UD_R_DX: return x86::gpr::DX;
    case UD_R_BX: return x86::gpr::BX;
    case UD_R_SP: return x86::gpr::SP;
    case UD_R_BP: return x86::gpr::BP;
    case UD_R_SI: return x86::gpr::SI;
    case UD_R_DI: return x86::gpr::DI;
    case UD_R_R8W: return x86::gpr::R8W;
    case UD_R_R9W: return x86::gpr::R9W;
    case UD_R_R10W: return x86::gpr::R10W;
    case UD_R_R11W: return x86::gpr::R11W;
    case UD_R_R12W: return x86::gpr::R12W;
    case UD_R_R13W: return x86::gpr::R13W;
    case UD_R_R14W: return x86::gpr::R14W;
    case UD_R_R15W: return x86::gpr::R15W;

    case UD_R_EAX: return x86::gpr::EAX;
    case UD_R_ECX: return x86::gpr::ECX;
    case UD_R_EDX: return x86::gpr::EDX;
    case UD_R_EBX: return x86::gpr::EBX;
    case UD_R_ESP: return x86::gpr::ESP;
    case UD_R_EBP: return x86::gpr::EBP;
    case UD_R_ESI: return x86::gpr::ESI;
    case UD_R_EDI: return x86::gpr::EDI;
    case UD_R_R8D: return x86::gpr::R8D;
    case UD_R_R9D: return x86::gpr::R9D;
    case UD_R_R10D: return x86::gpr::R10D;
    case UD_R_R11D: return x86::gpr::R11D;
    case UD_R_R12D: return x86::gpr::R12D;
    case UD_R_R13D: return x86::gpr::R13D;
    case UD_R_R14D: return x86::gpr::R14D;
    case UD_R_R15D: return x86::gpr::R15D;

    case UD_R_RAX: return x86::gpr::RAX;
    case UD_R_RCX: return x86::gpr::RCX;
    case UD_R_RDX: return x86::gpr::RDX;
    case UD_R_RBX: return x86::gpr::RBX;
    case UD_R_RSP: return x86::gpr::RSP;
    case UD_R_RBP: return x86::gpr::RBP;
    case UD_R_RSI: return x86::gpr::RSI;
    case UD_R_RDI: return x86::gpr::RDI;
    case UD_R_R8: return x86::gpr::R8;
    case UD_R_R9: return x86::gpr::R9;
    case UD_R_R10: return x86::gpr::R10;
    case UD_R_R11: return x86::gpr::R11;
    case UD_R_R12: return x86::gpr::R12;
    case UD_R_R13: return x86::gpr::R13;
    case UD_R_R14: return x86::gpr::R14;
    case UD_R_R15: return x86::gpr::R15;

    case UD_R_RIP: return x86::gpr::IP;
    default: PANIC("Not a register type: {}", unsigned(t));
    }
}

inline x86::segment_register to_segment(ud_type t)
{
    switch (t) {
    case UD_R_CS: return x86::segment_register::CS;
    case UD_R_DS: return x86::segment_register::DS;
    case UD_R_ES: return x86::segment_register::ES;
    case UD_R_FS: return x86::segment_register::FS;
    case UD_R_GS: return x86::segment_register::GS;
    case UD_R_SS: return x86::segment_register::SS;
    default: PANIC("Not a segment register type: {}", unsigned(t));
    };
}

inline instruction to_instruction(ud_mnemonic_code code)
{
    switch (code) {
    case UD_Imov: return instruction::MOV;
    case UD_Imovbe: return instruction::MOVBE;
    case UD_Imovzx: return instruction::MOVZX;
    case UD_Imovsb: return instruction::MOVSB;
    case UD_Imovsw: return instruction::MOVSW;
    case UD_Imovsd: return instruction::MOVSD;
    case UD_Imovsq: return instruction::MOVSQ;
    case UD_Istosb: return instruction::STOSB;
    case UD_Istosw: return instruction::STOSW;
    case UD_Istosd: return instruction::STOSD;
    case UD_Istosq: return instruction::STOSQ;
    case UD_Ibt: return instruction::BT;
    case UD_Isyscall: return instruction::SYSCALL;
    case UD_Isysret: return instruction::SYSRET;
    case UD_Iiretq: return instruction::IRET;
    case UD_Ilgdt: return instruction::LGDT;
    case UD_Isgdt: return instruction::SGDT;
    case UD_Ilidt: return instruction::LIDT;
    case UD_Isidt: return instruction::SIDT;
    case UD_Illdt: return instruction::LLDT;
    case UD_Isldt: return instruction::SLDT;
    case UD_Iltr: return instruction::LTR;
    case UD_Istr: return instruction::STR;
    case UD_Isti: return instruction::STI;
    case UD_Icli: return instruction::CLI;
    case UD_Iout: return instruction::OUT;
    case UD_Ijmp: return instruction::JMP;
    case UD_Icall: return instruction::CALL;
    case UD_Iret: return instruction::RET;
    case UD_Ipush: return instruction::PUSH;
    case UD_Ipop: return instruction::POP;
    case UD_Ilea: return instruction::LEA;
    case UD_Icpuid: return instruction::CPUID;
    case UD_Inop: return instruction::NOP;
    case UD_Ipause: return instruction::PAUSE;
    case UD_Iinc: return instruction::INC;
    case UD_Idec: return instruction::DEC;
    case UD_Ineg: return instruction::NEG;
    case UD_Inot: return instruction::NOT;
    case UD_Iadd: return instruction::ADD;
    case UD_Ior: return instruction::OR;
    case UD_Iand: return instruction::AND;
    case UD_Isub: return instruction::SUB;
    case UD_Ixor: return instruction::XOR;
    case UD_Ixchg: return instruction::XCHG;
    case UD_Isbb: return instruction::SBB;
    case UD_Iadc: return instruction::ADC;
    case UD_Icmp: return instruction::CMP;
    case UD_Itest: return instruction::TEST;
    case UD_Ishl: return instruction::SHL;
    case UD_Ishr: return instruction::SHR;
    case UD_Isar: return instruction::SAR;
    case UD_Ija: return instruction::JA;
    case UD_Ijae: return instruction::JAE;
    case UD_Ijb: return instruction::JB;
    case UD_Ijbe: return instruction::JBE;
    case UD_Ijg: return instruction::JG;
    case UD_Ijge: return instruction::JGE;
    case UD_Ijl: return instruction::JL;
    case UD_Ijle: return instruction::JLE;
    case UD_Ijno: return instruction::JNO;
    case UD_Ijnp: return instruction::JNP;
    case UD_Ijns: return instruction::JNS;
    case UD_Ijnz: return instruction::JNZ;
    case UD_Ijo: return instruction::JO;
    case UD_Ijp: return instruction::JP;
    case UD_Ijs: return instruction::JS;
    case UD_Ijz: return instruction::JZ;
    case UD_Iinvalid: return instruction::INVALID;
    default: return instruction::UNKNOWN;
    };
}

} // namespace decoder
