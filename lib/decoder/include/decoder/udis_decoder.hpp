/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <memory>

#include <decoder.hpp>
#include <decoder/operand.hpp>
#include <udis86.h>
#include <x86defs.hpp>

enum class instruction;

namespace decoder
{

class common_operand;

class udis_decoder final : public decoder
{
public:
    udis_decoder(decoder_vcpu& vcpu, x86::cpu_vendor vendor, heap_t& vcpu_local_heap)
        : vendor_(vendor), vcpu_local_heap_(vcpu_local_heap), decoder_data_(vcpu)
    {}

    virtual void decode(uintptr_t ip, std::optional<phy_addr_t> cr3 = {},
                        std::optional<uint8_t> replace_byte = {}) override;
    virtual void print_instruction() const override;
    virtual instruction get_instruction() const override;
    virtual unsigned get_instruction_length() const override;
    virtual operand_ptr_t get_operand(unsigned idx) const override;
    virtual bool is_far_branch() const override;
    virtual bool has_lock_prefix() const override;
    virtual optional_rep_prefix get_rep_prefix() const override;
    virtual optional_segment_register get_segment_override() const override;
    virtual size_t get_address_size() const override;
    virtual size_t get_operand_size() const override;
    virtual size_t get_stack_address_size() const override;

    virtual operand_ptr_t make_register_operand(size_t operand_size, x86::gpr reg) const override;

    virtual operand_ptr_t make_memory_operand(size_t operand_size, x86::segment_register seg,
                                              std::optional<x86::gpr> base, std::optional<x86::gpr> index,
                                              uint8_t scale, std::optional<int64_t> disp) const override;

private:
    template <typename T, class... ARGS>
    decoder::operand_ptr_t make_operand(ARGS&&... args) const
    {
        auto ptr = vcpu_local_heap_.alloc(sizeof(T));
        PANIC_UNLESS(ptr, "Out of local heap memory!");
        return {new (ptr) T(decoder_data_.vcpu, std::forward<ARGS>(args)...), {vcpu_local_heap_}};
    }

    struct custom_data
    {
        explicit custom_data(decoder_vcpu& v) : vcpu(v) {}
        decoder_vcpu& vcpu;
        uintptr_t emulate_ip = 0;
        std::optional<phy_addr_t> emulate_cr3 = {};
        std::optional<uint8_t> emulate_replacement_byte = {};
    };

    static int read_instruction_byte(ud_t* ud_obj);
    decoder_vcpu& get_vcpu() const { return decoder_data_.vcpu; }

    x86::cpu_vendor vendor_;
    heap_t& vcpu_local_heap_;
    custom_data decoder_data_;
    ud_t decoder_;
};

} // namespace decoder
