/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <decoder.hpp>
#include <emulator_vcpu.hpp>
#include <trace.hpp>
#include <x86defs.hpp>

#include <optional>

namespace emulator
{

using emulator_vcpu = vcpu;
using access_override = decoder::access_override;

class emulator
{
public:
    using translation_info = decoder::decoder_vcpu::translation_info;

    enum class status_t
    {
        OK,
        FAULT,
        UNIMPLEMENTED,
    };

    emulator(emulator_vcpu& vcpu, decoder::decoder& dec, uint64_t* insn_len)
        : vcpu_(vcpu), decoder_(dec), insn_len_(insn_len)
    {
        ASSERT(insn_len != nullptr, "No valid instruction length pointer supplied!");
    }

    status_t emulate(uintptr_t ip, std::optional<uint8_t> replace_byte = {});

    static bool is_fully_implemented(decoder::instruction i);

private:
    emulator_vcpu& vcpu_;
    decoder::decoder& decoder_;
    uint64_t* insn_len_;
};

} // namespace emulator
