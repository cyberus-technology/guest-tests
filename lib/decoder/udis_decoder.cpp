/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <memory>

#include <decoder/gdt.hpp>
#include <decoder/instructions.hpp>
#include <decoder/operand.hpp>
#include <decoder/translations.hpp>
#include <decoder/udis_decoder.hpp>
#include <x86defs.hpp>

using namespace x86;

namespace decoder
{

void udis_decoder::decode(uintptr_t ip, std::optional<phy_addr_t> cr3, std::optional<uint8_t> replace_byte)
{
    ud_init(&decoder_);

    switch (vendor_) {
    case cpu_vendor::INTEL: ud_set_vendor(&decoder_, UD_VENDOR_INTEL); break;
    case cpu_vendor::AMD: ud_set_vendor(&decoder_, UD_VENDOR_AMD); break;
    default: ud_set_vendor(&decoder_, UD_VENDOR_ANY); break;
    }
    ud_set_syntax(&decoder_, UD_SYN_INTEL);

    switch (decoder_data_.vcpu.get_cpu_mode()) {
    case x86::cpu_mode::RM16:
    case x86::cpu_mode::CM16:
    case x86::cpu_mode::PM16: ud_set_mode(&decoder_, 16); break;
    case x86::cpu_mode::RM32:
    case x86::cpu_mode::CM32:
    case x86::cpu_mode::PM32: ud_set_mode(&decoder_, 32); break;
    case x86::cpu_mode::PM64: ud_set_mode(&decoder_, 64); break;
    };

    ud_set_pc(&decoder_, ip);
    decoder_data_.emulate_ip = ip;
    decoder_data_.emulate_cr3 = cr3;
    decoder_data_.emulate_replacement_byte = replace_byte;

    ud_set_user_opaque_data(&decoder_, &decoder_data_);
    ud_set_input_hook(&decoder_, read_instruction_byte);
    ud_disassemble(&decoder_);
}

void udis_decoder::print_instruction() const
{
    info("Instruction: {s}", ud_insn_asm(&decoder_));
}

instruction udis_decoder::get_instruction() const
{
    return to_instruction(ud_insn_mnemonic(&decoder_));
}

unsigned udis_decoder::get_instruction_length() const
{
    return ud_insn_len(&decoder_);
}

static segment_register determine_segment(std::optional<gpr> base)
{
    if (not base) {
        return segment_register::DS;
    }

    switch (*base) {
    case gpr::SPL:
    case gpr::BPL:
    case gpr::SP:
    case gpr::BP:
    case gpr::ESP:
    case gpr::EBP:
    case gpr::RSP:
    case gpr::RBP: return segment_register::SS;
    default: return segment_register::DS;
    }
    __UNREACHED__;
}

namespace
{
bool is_segment_register(ud_type t)
{
    return t >= UD_R_ES and t <= UD_R_GS;
}
bool is_gpr(ud_type t)
{
    return t >= UD_R_AL and t <= UD_R_R15;
}
} // namespace

decoder::operand_ptr_t udis_decoder::get_operand(unsigned idx) const
{
    const ud_operand* op {ud_insn_opr(&decoder_, idx)};

    if (op == nullptr) {
        return {nullptr, {vcpu_local_heap_}};
    }

    switch (op->type) {
    case UD_OP_REG:
        if (is_segment_register(op->base)) {
            return make_operand<segment_register_operand>(op->size / 8, to_segment(op->base));
        } else if (is_gpr(op->base)) {
            return make_operand<register_operand>(op->size / 8, to_gpr(op->base));
        } else {
            PANIC("Unknown register type {}", unsigned(op->base));
        }
    case UD_OP_MEM: {
        std::optional<int64_t> disp;
        switch (op->offset) {
        case 8: disp = op->lval.sbyte; break;
        case 16: disp = op->lval.sword; break;
        case 32: disp = op->lval.sdword; break;
        case 64: disp = op->lval.sqword; break;
        default: break;
        }

        if (disp) {
            *disp &= math::mask(get_address_size());
        }

        std::optional<gpr> base;
        if (op->base != UD_NONE) {
            base = to_gpr(op->base);
        }

        std::optional<gpr> index;
        if (op->index != UD_NONE) {
            index = to_gpr(op->index);
        }

        segment_register seg =
            decoder_.pfx_seg != UD_NONE ? to_segment(ud_type(decoder_.pfx_seg)) : determine_segment(base);

        return make_operand<memory_operand>(op->size / 8, seg, base, index, op->scale, disp);
    }
    case UD_OP_IMM:
    case UD_OP_JIMM: {
        int64_t value {0};
        switch (op->size) {
        case 8: value = op->lval.sbyte; break;
        case 16: value = op->lval.sword; break;
        case 32: value = op->lval.sdword; break;
        case 64: value = op->lval.sqword; break;
        default: PANIC("Unknown immediate width!");
        }

        return make_operand<immediate_operand>(op->size / 8, value);
    }
    case UD_OP_PTR: return make_operand<pointer_operand>(op->lval.ptr.seg, op->lval.ptr.off);
    default: PANIC("Operand type {} not implemented", unsigned(op->type));
    };
}

bool udis_decoder::is_far_branch() const
{
    auto mnemonic {ud_insn_mnemonic(&decoder_)};
    PANIC_UNLESS(mnemonic == UD_Ijmp or mnemonic == UD_Icall, "Invalid instruction to check for far branch");
    return decoder_.br_far or ud_insn_opr(&decoder_, 0)->type == UD_OP_PTR;
}

bool udis_decoder::has_lock_prefix() const
{
    return decoder_.pfx_lock != UD_NONE;
}

std::optional<rep_prefix> udis_decoder::get_rep_prefix() const
{
    if (decoder_.pfx_rep)
        return {rep_prefix::REP};
    if (decoder_.pfx_repe)
        return {rep_prefix::REPE};
    if (decoder_.pfx_repne)
        return {rep_prefix::REPNE};
    return {};
}

size_t udis_decoder::get_address_size() const
{
    return decoder_.adr_mode;
}

size_t udis_decoder::get_operand_size() const
{
    return decoder_.opr_mode;
}

size_t udis_decoder::get_stack_address_size() const
{
    auto& vcpu {get_vcpu()};

    if (vcpu.get_cpu_mode() == x86::cpu_mode::PM64) {
        return 64;
    }

    auto ss = vcpu.read(segment_register::SS);
    return (ss.ar & gdt_entry::AR_DB) ? 32 : 16;
}

std::optional<segment_register> udis_decoder::get_segment_override() const
{
    if (decoder_.pfx_seg != UD_NONE) {
        return {to_segment(ud_type(decoder_.pfx_seg))};
    }
    return {};
}

decoder::operand_ptr_t udis_decoder::make_register_operand(size_t operand_size, gpr reg) const
{
    return make_operand<register_operand>(operand_size, reg);
}

decoder::operand_ptr_t udis_decoder::make_memory_operand(size_t operand_size, segment_register seg,
                                                         std::optional<gpr> base, std::optional<gpr> index,
                                                         uint8_t scale, std::optional<int64_t> disp) const
{
    return make_operand<memory_operand>(operand_size, seg, base, index, scale, disp);
}

int udis_decoder::read_instruction_byte(ud_t* ud_obj)
{
    auto data {reinterpret_cast<custom_data*>(ud_get_user_opaque_data(ud_obj))};

    uint8_t value {0};
    if (data->emulate_replacement_byte) {
        value = *data->emulate_replacement_byte;
        data->emulate_replacement_byte = {};
    } else {
        log_addr_t addr {segment_register::CS, data->emulate_ip};
        bool success {data->vcpu.read(addr, 1, &value, x86::memory_access_type_t::EXECUTE, {false, data->emulate_cr3})};

        PANIC_UNLESS(success, "Read byte failed!");
    }

    data->emulate_ip++;

    return value;
}

} // namespace decoder
