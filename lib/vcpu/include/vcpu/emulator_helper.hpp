/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <decoder/udis_decoder.hpp>
#include <emulator.hpp>
#include <hedron/utcb.hpp>

namespace vcpu
{

namespace emulator
{

template <typename VCPU>
struct temp_emulator
{
    template <typename T>
    temp_emulator(VCPU& vcpu_, ::emulator::emulator_vcpu& emu_vcpu_, T& storage)
        : vcpu(vcpu_)
        , emu_vcpu(emu_vcpu_)
        , mem(ptr_to_num(storage.data()), storage.size())
        , heap(mem)
        , dec(emu_vcpu, x86::cpu_vendor::INTEL, heap)
    {}

    VCPU& vcpu;
    ::emulator::emulator_vcpu& emu_vcpu;
    fixed_memory mem;
    decoder::heap_t heap;
    decoder::udis_decoder dec;
};

template <typename VCPU>
inline bool emulate(temp_emulator<VCPU>& temp_emulator, std::optional<uint8_t> replace_byte)
{
    using x86::gpr;

    auto& dec {temp_emulator.dec};
    auto& vcpu {temp_emulator.vcpu};
    auto& emu_vcpu {temp_emulator.emu_vcpu};
    auto& state {temp_emulator.vcpu.get_state()};

    ::emulator::emulator instance(emu_vcpu, dec, &state.ins_len);

    switch (instance.emulate(vcpu.read(gpr::CUR_IP), replace_byte)) {
    case ::emulator::emulator::status_t::OK:
        // TODO: Check if rep prefix is actually supported for the current instruction
        if (dec.get_rep_prefix()) {
            // FIXME: This can be optimized by emulating batches of instructions until
            //        either an interrupt is pending or the conditions are no longer met.
            auto adr_size {dec.get_address_size()};
            ASSERT(vcpu.read(gpr::RCX) & math::mask(adr_size), "REP prefix, but CX=0?");

            // Decrement counter register and set MTD bit
            vcpu.write(gpr::RCX, vcpu.read(gpr::RCX) - 1);

            // We don't update the IP here, because that will be handled by the hardware.
            // Once CX=0 after this, the instruction will become a nop and just be skipped.
        } else {
            vcpu.update_ip();
        }
        return true;
    case ::emulator::emulator::status_t::FAULT:
        // If there was a fault, the only way for the guest to make progress is
        // by handling an exception. If nothing is injected, this would just
        // cause an endless loop because the cause for the exception can never
        // be removed. But currently not all fault conditions lead to an
        // injection request.
        // In order to prevent silent infinite loops, we return false when
        // we arrive here and are still able to inject an exception so the
        // caller can detect this condition.
        if (vcpu.can_inject_exception()) {
            WARN_ONCE("Emulator indicates fault, but no exception is registered. This might cause an infinite loop!");
            return false;
        } else {
            return true;
        }
    case ::emulator::emulator::status_t::UNIMPLEMENTED: return false;
    };
    __UNREACHED__;
}

template <typename VCPU>
inline decoder::instruction get_instruction(temp_emulator<VCPU>& temp_emulator, uintptr_t ip,
                                            std::optional<phy_addr_t> cr3)
{
    auto& dec {temp_emulator.dec};
    dec.decode(ip, cr3);
    return dec.get_instruction();
}

template <typename VCPU>
inline bool is_fully_implemented(temp_emulator<VCPU>& temp_emulator, uintptr_t ip, std::optional<phy_addr_t> cr3)
{
    return ::emulator::emulator::is_fully_implemented(get_instruction(temp_emulator, ip, cr3));
}

} // namespace emulator

} // namespace vcpu
