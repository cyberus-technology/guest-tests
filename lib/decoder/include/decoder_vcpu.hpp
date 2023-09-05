/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.hpp>
#include <x86defs.hpp>

#include <optional>

namespace decoder
{

struct access_override
{
    access_override(bool implicit_supervisor_ = false, std::optional<phy_addr_t> cr3_ = {})
        : implicit_supervisor(implicit_supervisor_), cr3(cr3_)
    {}

    bool implicit_supervisor;
    std::optional<phy_addr_t> cr3;

    bool operator==(const access_override& o) const
    {
        return implicit_supervisor == o.implicit_supervisor and cr3 == o.cr3;
    }
};

class vcpu
{
public:
    virtual ~vcpu() {}

    /**
     * Information structure that is returned for logical address translations.
     * If there is no logical to linear translation, both members will be empty.
     */
    struct translation_info
    {
        std::optional<lin_addr_t> lin_addr;
        std::optional<phy_addr_t> phy_addr;
    };

    virtual std::optional<uint32_t> read(uint16_t port, size_t bytes) = 0;
    virtual bool write(uint16_t port, size_t bytes, uint32_t val) = 0;
    virtual bool read(lin_addr_t lin_addr, size_t bytes, void* buf,
                      x86::memory_access_type_t type = x86::memory_access_type_t::READ, access_override ovrd = {}) = 0;
    virtual bool write(lin_addr_t lin_addr, size_t bytes, void* buf, access_override ovrd = {}) = 0;
    virtual bool read(log_addr_t log_addr, size_t bytes, void* buf,
                      x86::memory_access_type_t type = x86::memory_access_type_t::READ, access_override ovrd = {}) = 0;
    virtual bool write(log_addr_t log_addr, size_t bytes, void* buf, access_override ovrd = {}) = 0;
    virtual bool read(phy_addr_t p_addr, size_t bytes, void* buf,
                      x86::memory_access_type_t type = x86::memory_access_type_t::READ) = 0;
    virtual bool write(phy_addr_t p_addr, size_t bytes, void* buf) = 0;
    virtual bool is_guest_accessible(phy_addr_t phy_addr) = 0;
    virtual translation_info translate(log_addr_t log_addr, x86::memory_access_type_t type, size_t bytes,
                                       access_override ovrd = {}) = 0;
    virtual uint64_t read(x86::gpr reg) const = 0;
    virtual void write(x86::gpr reg, uint64_t value) = 0;
    virtual uint64_t read(x86::control_register reg) const = 0;
    virtual void write(x86::control_register reg, uint64_t value) = 0;
    virtual x86::gdt_segment read(x86::segment_register reg) const = 0;
    virtual bool write(x86::segment_register reg, x86::gdt_segment segment) = 0;
    virtual x86::gdt_segment read(x86::mmreg reg) const = 0;
    virtual bool write(x86::mmreg reg, x86::gdt_segment segment) = 0;
    virtual void set_mov_ss_blocking() = 0;
    virtual x86::cpu_mode get_cpu_mode() = 0;
    virtual bool can_inject_exception() const = 0;
};

} // namespace decoder
