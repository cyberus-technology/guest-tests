/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <segmentation.hpp>
#include <x86defs.hpp>

class vcpu_register_interface
{
public:
    virtual ~vcpu_register_interface() {}

    virtual void init_state() = 0;

    virtual void update_ip() = 0;

    virtual uint64_t read(x86::gpr reg) const = 0;
    virtual void write(x86::gpr reg, uint64_t value) = 0;

    virtual uint64_t read(x86::control_register reg) const = 0;
    virtual void write(x86::control_register reg, uint64_t value) = 0;

    virtual x86::gdt_segment read(x86::segment_register reg) const = 0;
    virtual bool write(x86::segment_register reg, x86::gdt_segment segment) = 0;

    virtual x86::gdt_segment read(x86::mmreg reg) const = 0;
    virtual bool write(x86::mmreg reg, x86::gdt_segment segment) = 0;

    virtual uint64_t read_pdpte(unsigned idx) const = 0;
    virtual void write_pdpte(unsigned idx, uint64_t val) = 0;

    virtual x86::cpu_mode get_cpu_mode() = 0;

    virtual unsigned current_cpl() const = 0;
    virtual unsigned current_iopl() const = 0;
};
