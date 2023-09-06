/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <decoder_vcpu.hpp>
#include "virtual_msr_access_result.hpp"

namespace emulator
{

class vcpu : virtual public decoder::vcpu
{
public:
    virtual ~vcpu() {}

    virtual virtual_msr_access_result handle_rdmsr(uint32_t idx) = 0;
    virtual void handle_cpuid() = 0;
    virtual unsigned current_cpl() const = 0;
    virtual unsigned current_iopl() const = 0;
    virtual void set_sti_blocking() = 0;
    virtual bool can_inject_exception() const = 0;
};

} // namespace emulator
