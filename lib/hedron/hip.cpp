/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <hedron/hip.hpp>

namespace hedron
{

unsigned hip::cpus() const
{
    const auto& d(cpu_descriptors());
    return std::count_if(std::begin(d), std::end(d), [](const auto& i) { return i.enabled(); });
}

const hip::cpu_descriptor* hip::first_cpu() const
{
    return reinterpret_cast<const cpu_descriptor*>(reinterpret_cast<const char*>(&hip_) + hip_.cpu_.offset_);
}

const hip::mem_descriptor* hip::first_mem() const
{
    return reinterpret_cast<const mem_descriptor*>(reinterpret_cast<const char*>(&hip_) + hip_.mem_.offset_);
}

bool hip::valid() const
{
    if (hip_.signature_ != SIGNATURE_VALID) {
        return false;
    }

    return math::checksum<uint16_t>(&hip_, hip_.length_) == 0;
}

bool hip::version_ok() const
{
    return api_version() / 1000 == API_VERSION / 1000 and api_version() % 1000 >= API_VERSION % 1000;
}

} // namespace hedron
