/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <optional>

#include <arch.hpp>
#include "decoder.hpp"
#include <math/math.hpp>
#include <x86defs.hpp>

namespace decoder
{

class common_operand
{
public:
    virtual ~common_operand() {}

    enum class type
    {
        REG,
        IMM,
        MEM,
        PTR,
    };

    virtual bool read() = 0;  ///< Retrieves the operand's value and stores it in the object
    virtual bool write() = 0; ///< Writes back the cached value to the operand destination

    template <typename T>
    const T get() const
    {
        return static_cast<T>(value_);
    } ///< Returns the cached value

    void set(uint64_t v) { value_ = v; }

    explicit common_operand(decoder_vcpu& vcpu, size_t width, type t) : vcpu_(vcpu), width_(width), type_(t) {}
    explicit common_operand(decoder_vcpu& vcpu, size_t width, type t, uint64_t val)
        : vcpu_(vcpu), value_(val), width_(width), type_(t)
    {}

    type get_type() const { return type_; }
    size_t get_width() const { return width_; }

protected:
    decoder_vcpu& vcpu_;
    uint64_t value_ {0};
    size_t width_ {0};
    type type_;
};

class register_operand : public common_operand
{
public:
    virtual bool read() override;
    virtual bool write() override;

    explicit register_operand(decoder_vcpu& vcpu, size_t width, x86::gpr reg)
        : common_operand(vcpu, width, type::REG), reg_(reg)
    {}

private:
    x86::gpr reg_;
};

class segment_register_operand : public common_operand
{
public:
    virtual bool read() override;
    virtual bool write() override;

    explicit segment_register_operand(decoder_vcpu& vcpu, size_t width, x86::segment_register reg)
        : common_operand(vcpu, width, type::REG), reg_(reg)
    {}

private:
    x86::segment_register reg_;
};

class immediate_operand : public common_operand
{
public:
    virtual bool read() override { return true; }
    virtual bool write() override;

    explicit immediate_operand(decoder_vcpu& vcpu, size_t width, uint64_t val)
        : common_operand(vcpu, width, type::IMM, val)
    {}
};

class memory_operand final : public common_operand
{
public:
    virtual bool read() override;
    virtual bool write() override;

    uintptr_t effective_address() const;
    std::optional<phy_addr_t> physical_address(x86::memory_access_type_t type);
    bool is_guest_accessible() const;

    explicit memory_operand(decoder_vcpu& vcpu, size_t width, x86::segment_register seg, std::optional<x86::gpr> base,
                            std::optional<x86::gpr> index, uint8_t scale, std::optional<int64_t> disp)
        : common_operand(vcpu, width, type::MEM)
        , seg_(seg)
        , base_(base)
        , index_(index)
        , scale_(std::max<uint8_t>(1, scale))
        , disp_(disp)
    {}

private:
    x86::segment_register seg_;
    std::optional<x86::gpr> base_;
    std::optional<x86::gpr> index_;
    uint8_t scale_;
    std::optional<int64_t> disp_;
    std::optional<phy_addr_t> physical_address_;
};

class pointer_operand : public common_operand
{
public:
    virtual bool read() override;
    virtual bool write() override;

    explicit pointer_operand(decoder_vcpu& vcpu, uint16_t selector, uint64_t offset)
        : common_operand(vcpu, 0, type::PTR), selector_(selector), offset_(offset)
    {}

    uint16_t get_selector() const { return selector_; }
    uint64_t get_offset() const { return offset_; }

private:
    uint16_t selector_;
    uint64_t offset_;
};

/// Maximum size of an operand object
static constexpr size_t MAX_OPERAND_SIZE {
    math::align_up(sizeof(memory_operand), math::order_envelope(DECODER_HEAP_ALIGNMENT))};

/// Maximum number of operand objects involved in emulating a single instruction
static constexpr size_t MAX_OPERANDS {8};

/// Heap size needed for storing operands during emulation
static constexpr size_t OPERAND_HEAP_SIZE {MAX_OPERANDS * MAX_OPERAND_SIZE};

static_assert(MAX_OPERAND_SIZE >= sizeof(register_operand), "Maximum operand size is incorrect!");
static_assert(MAX_OPERAND_SIZE >= sizeof(segment_register_operand), "Maximum operand size is incorrect!");
static_assert(MAX_OPERAND_SIZE >= sizeof(immediate_operand), "Maximum operand size is incorrect!");
static_assert(MAX_OPERAND_SIZE >= sizeof(memory_operand), "Maximum operand size is incorrect!");
static_assert(MAX_OPERAND_SIZE >= sizeof(pointer_operand), "Maximum operand size is incorrect!");

} // namespace decoder
