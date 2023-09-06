/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/x86/virtual_msr_bus.hpp>

virtual_msr_access_result virtual_msr_bus::read(uint32_t idx)
{
    if (auto msr {msrs.find(idx)}; msr != msrs.end()) {
        return msr->second.read();
    } else {
        return virtual_msr_access_result::access_was_not_handled();
    }
}

virtual_msr_access_result virtual_msr_bus::write(uint32_t idx, uint64_t val)
{
    if (auto msr {msrs.find(idx)}; msr != msrs.end()) {
        return msr->second.write(val);
    } else {
        return virtual_msr_access_result::access_was_not_handled();
    }
}

bool virtual_msr_bus::add(const virtual_msr& msr)
{
    const auto [_, success] = msrs.insert({msr.index(), msr});
    return success;
}
