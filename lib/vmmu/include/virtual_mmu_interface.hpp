/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "vmmu.hpp"

#include <optional>
#include <variant>

#include <arch.hpp>
#include <x86defs.hpp>

// Minimal vcpu functionality required by the vmmu.
class vmmu_vcpu_interface
{
public:
    virtual ~vmmu_vcpu_interface() {}

    virtual bool read(phy_addr_t p_addr, size_t bytes, void* buf) = 0;
    virtual bool write(phy_addr_t p_addr, size_t bytes, void* buf) = 0;

    virtual uint64_t read(x86::gpr reg) const = 0;
    virtual uint64_t read(x86::control_register reg) const = 0;

    virtual uint64_t read_pdpte(uint64_t num) const = 0;
    virtual uint64_t read_efer() const = 0;
};

struct mmu_exception_info
{
    uint8_t vector;
    uint32_t error;

    bool operator!=(const mmu_exception_info& other) const
    {
        return (vector != other.vector) or (error != other.error);
    }
};

using mmu_entry = std::variant<phy_addr_t, mmu_exception_info>;

/// A wrapper around extern memory management unit implementations.
class virtual_mmu_interface
{
public:
    virtual ~virtual_mmu_interface() {}

    virtual mmu_entry translate_linear(vmmu_vcpu_interface& vcpu, uintptr_t lin_addr, bool is_implicit_supervisor,
                                       unsigned cpl, x86::memory_access_type_t type,
                                       std::optional<phy_addr_t> cr3 = {}) = 0;

    virtual void flush() = 0;
};
