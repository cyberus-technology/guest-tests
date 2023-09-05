/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <virtual_mmu_interface.hpp>
#include <vmmu.hpp>
#include <x86defs.hpp>

namespace vmmu
{

// Memory implementation encapsulates access to phys memory for the vmmu
class memory final : public vmmu::abstract_memory
{
    vmmu_vcpu_interface& vcpu;

    template <typename INT>
    INT read_int(uint64_t phys_addr)
    {
        INT value;

        PANIC_UNLESS(vcpu.read(phy_addr_t {phys_addr}, sizeof(value), &value), "MMU: unable to read {#x}", phys_addr);

        return value;
    }

    template <typename INT>
    void write_int(uint64_t phys_addr, INT value)
    {
        PANIC_UNLESS(vcpu.write(phy_addr_t {phys_addr}, sizeof(value), &value), "MMU: unable to write {#x}", phys_addr);
    }

    template <typename INT>
    bool cmpxchg_int(uint64_t phys_addr, INT expected, INT new_value)
    {
        // TODO This is (obviously) not atomic and the memory subsystem needs to be fixed to support atomic accesses.
        if (read_int<INT>(phys_addr) == expected) {
            write_int<INT>(phys_addr, new_value);
            return true;
        }

        return false;
    }

public:
    virtual uint64_t read(uint64_t phys_addr, uint64_t) override { return read_int<uint64_t>(phys_addr); }

    virtual uint32_t read(uint64_t phys_addr, uint32_t) override { return read_int<uint32_t>(phys_addr); }

    virtual bool cmpxchg(uint64_t phys_addr, uint32_t expected, uint32_t new_value) override
    {
        return cmpxchg_int<uint32_t>(phys_addr, expected, new_value);
    }

    virtual bool cmpxchg(uint64_t phys_addr, uint64_t expected, uint64_t new_value) override
    {
        return cmpxchg_int<uint64_t>(phys_addr, expected, new_value);
    }

    explicit memory(vmmu_vcpu_interface& vcpu_) : vcpu(vcpu_) {}
};

// A virtual MMU implementation that funnels requests to the vmmu contrib code.
class virtual_mmu final : public ::virtual_mmu_interface
{
    // The TLB is usually used for emulating a single instruction. There is no point in making it large.
    static const size_t TLB_SIZE {4};
    vmmu::tlb<TLB_SIZE> tlb;

    using vmmu_access_type = vmmu::linear_memory_op::access_type;
    using vmmu_supervisor_type = vmmu::linear_memory_op::supervisor_type;

    vmmu_access_type translate_type(x86::memory_access_type_t type)
    {
        switch (type) {
        case x86::memory_access_type_t::READ: return vmmu_access_type::READ;
        case x86::memory_access_type_t::WRITE: return vmmu_access_type::WRITE;
        case x86::memory_access_type_t::EXECUTE: return vmmu_access_type::EXECUTE;
        }

        __UNREACHED__;
    }

    static vmmu::paging_state state_from_vcpu(vmmu_vcpu_interface& vcpu, unsigned cpl, std::optional<phy_addr_t> cr3)
    {
        using x86::control_register;
        using x86::gpr;

        return vmmu::paging_state(vcpu.read(gpr::FLAGS), vcpu.read(control_register::CR0),
                                  static_cast<uintptr_t>(cr3.value_or(phy_addr_t(vcpu.read(control_register::CR3)))),
                                  vcpu.read(control_register::CR4), vcpu.read_efer(), cpl,
                                  {
                                      vcpu.read_pdpte(0),
                                      vcpu.read_pdpte(1),
                                      vcpu.read_pdpte(2),
                                      vcpu.read_pdpte(3),
                                  });
    }

public:
    virtual_mmu() = default;
    virtual ~virtual_mmu() override {}

    virtual mmu_entry translate_linear(vmmu_vcpu_interface& vcpu, uintptr_t lin_addr, bool is_implicit_supervisor,
                                       unsigned cpl, x86::memory_access_type_t type,
                                       std::optional<phy_addr_t> cr3 = {}) override
    {
        memory mem {vcpu};

        vmmu::linear_memory_op const op {lin_addr, translate_type(type),
                                         is_implicit_supervisor ? vmmu_supervisor_type::IMPLICIT
                                                                : vmmu_supervisor_type::EXPLICIT};

        auto const result {tlb.translate(op, state_from_vcpu(vcpu, cpl, cr3), &mem)};

        if (std::holds_alternative<vmmu::tlb_entry>(result)) {
            auto const tlb_e {std::get<vmmu::tlb_entry>(result)};
            auto opt_phys {tlb_e.translate(lin_addr)};

            ASSERT(opt_phys, "Return TLB entry doesn't translate address that generated it?");

            return phy_addr_t {*opt_phys};
        }

        if (std::holds_alternative<vmmu::page_fault_info>(result)) {
            auto const pf_info {std::get<vmmu::page_fault_info>(result)};

            return mmu_exception_info {uint8_t(x86::exception::PF), pf_info.error_code};
        }

        __UNREACHED__;
    }

    virtual void flush() override { tlb.clear(); }
};

} // namespace vmmu
