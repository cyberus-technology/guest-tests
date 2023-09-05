/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <assert.h>

#include <memory>
#include <optional>

#include <arch.hpp>
#include <heap.hpp>

#include <decoder_vcpu.hpp>
#include <simple_heap.hpp>

namespace decoder
{

using decoder_vcpu = vcpu;

using heap_t = simple_heap<>;
static constexpr size_t DECODER_HEAP_ALIGNMENT {HEAP_MIN_ALIGNMENT};

enum class instruction;
enum class reg_prefix;
class common_operand;

class decoder
{
public:
    virtual ~decoder() {}

    struct custom_deleter
    {
        // Needed for mocking :( This should never be called.
        custom_deleter() : heap_(*reinterpret_cast<heap_t*>(0x10)) { assert(false); }

        custom_deleter(heap_t& heap) : heap_(heap) {}
        void operator()(void* ptr) { heap_.free(ptr); }
        heap_t& heap_;
    };
    using operand_ptr_t = std::unique_ptr<common_operand, custom_deleter>;
    using optional_rep_prefix = std::optional<x86::rep_prefix>;
    using optional_segment_register = std::optional<x86::segment_register>;

    virtual void decode(uintptr_t ip, std::optional<phy_addr_t> cr3 = {}, std::optional<uint8_t> replace_byte = {}) = 0;

    virtual void print_instruction() const = 0;
    virtual instruction get_instruction() const = 0;
    virtual unsigned get_instruction_length() const = 0;
    virtual operand_ptr_t get_operand(unsigned idx) const = 0;
    virtual bool is_far_branch() const = 0;
    virtual bool has_lock_prefix() const = 0;
    virtual optional_rep_prefix get_rep_prefix() const = 0;
    virtual optional_segment_register get_segment_override() const = 0;
    virtual size_t get_address_size() const = 0;
    virtual size_t get_operand_size() const = 0;
    virtual size_t get_stack_address_size() const = 0;

    virtual operand_ptr_t make_register_operand(size_t operand_size, x86::gpr reg) const = 0;

    virtual operand_ptr_t make_memory_operand(size_t operand_size, x86::segment_register seg,
                                              std::optional<x86::gpr> base, std::optional<x86::gpr> index,
                                              uint8_t scale, std::optional<int64_t> disp) const = 0;
};

} // namespace decoder
