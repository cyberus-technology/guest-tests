/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */
#pragma once

#include <hedron/cap_sel.hpp>
#include <hedron/mtd.hpp>
#include <hedron/qpd.hpp>

#include <optional>

namespace hedron
{
class utcb;
class vcpu;
}; // namespace hedron

class execution_context
{
    template <class T>
    using optional = std::optional<T>;

public:
    /**
     * Non default constructible object holding a stack pointer. This forces
     * the user to explicitly pass a stack descriptor to the ec_desc.
     */
    struct stack_descriptor
    {
        stack_descriptor() = delete;

        explicit stack_descriptor(uintptr_t sp_) : sp(sp_) {}

        const uintptr_t sp;
    };

    struct ec_desc
    {
        unsigned cpu {0};
        optional<hedron::cap_sel> pd {};
        optional<hedron::cap_sel> evt_base {};
        optional<hedron::cap_sel> selector {};
        optional<hedron::utcb*> utcb {};
        stack_descriptor stack_desc;
        bool user_page_in_calling_pd {false};
    };

    enum
    {
        EXC_RANGE_ORDER = 5,
        EVT_RANGE_ORDER = 8,

        EXC_RANGE_SIZE = 1U << EXC_RANGE_ORDER,
        EVT_RANGE_SIZE = 1U << EVT_RANGE_ORDER,
    };

    enum class type
    {
        LOCAL,
        GLOBAL,
    };

    execution_context(const ec_desc& descriptor, type ec_type);

    hedron::cap_sel get_evt() const;

    uintptr_t get_sp() const;

protected:
    ec_desc desc;
};

class local_ec : public execution_context
{
public:
    explicit local_ec(const ec_desc& descriptor) : execution_context(descriptor, type::LOCAL) {}

    void create_portal(hedron::mtd_bits::field mtd, uintptr_t entry_ip, hedron::cap_sel pt_sel = ~0ul,
                       uint64_t id = 0) const;
};

class global_ec : public execution_context
{
public:
    explicit global_ec(const ec_desc& descriptor) : execution_context(descriptor, type::GLOBAL) {}

    void create_sc(hedron::qpd qpd, std::optional<hedron::cap_sel> sel = {}) const;
};
