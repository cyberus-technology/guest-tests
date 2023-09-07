/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/decoder/gdt.hpp>
#include <toyos/decoder/operand.hpp>

using namespace decoder;
using namespace x86;

bool register_operand::read()
{
    value_ = vcpu_.read(reg_);
    return true;
}

bool register_operand::write()
{
    vcpu_.write(reg_, value_);
    return true;
}

bool immediate_operand::write()
{
    PANIC("Cannot write immediate operand.");
}

bool segment_register_operand::read()
{
    value_ = vcpu_.read(reg_).sel;
    return true;
}

bool segment_register_operand::write()
{
    // TODO: Check for exceptions
    auto entry = parse_gdt(vcpu_, value_ & ~SELECTOR_PL_MASK);
    if (reg_ == segment_register::SS) {
        vcpu_.set_mov_ss_blocking();
    }
    return vcpu_.write(reg_, {uint16_t(value_), entry.ar(), entry.limit(), entry.base()});
}

uintptr_t memory_operand::effective_address() const
{
    uintptr_t effective_address {0};

    if (base_) {
        effective_address += vcpu_.read(*base_);
    }

    if (index_) {
        effective_address += vcpu_.read(*index_) * scale_;
    }

    if (disp_) {
        effective_address += *disp_;
    }

    return effective_address;
}

std::optional<phy_addr_t> memory_operand::physical_address(x86::memory_access_type_t atype)
{
    auto translation {vcpu_.translate(log_addr_t(seg_, effective_address()), atype, width_)};
    physical_address_ = translation.phy_addr;
    return physical_address_;
}

bool memory_operand::is_guest_accessible() const
{
    PANIC_UNLESS(physical_address_, "Cannot check guest accessibility if translation is invalid!");
    return vcpu_.is_guest_accessible(*physical_address_);
}

bool memory_operand::read()
{
    return vcpu_.read(log_addr_t(seg_, effective_address()), width_, &value_, x86::memory_access_type_t::READ);
}

bool memory_operand::write()
{
    return vcpu_.write(log_addr_t(seg_, effective_address()), width_, &value_);
}

bool pointer_operand::read()
{
    PANIC("Not implemented");
}
bool pointer_operand::write()
{
    PANIC("Not implemented");
}
