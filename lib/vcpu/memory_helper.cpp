/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <segmentation_unit.hpp>
#include <vcpu/memory_helper.hpp>

namespace vcpu
{

namespace memory
{

/*
 * Sanity check for the size of memory accesses
 *
 * While the memory subsystem would theoretically support accessing up to 4 KiB
 * at once, standard operation will only ever require a pre-defined subset of
 * sizes corresponding to memory operands of instructions. To catch bugs early
 * (e.g., the emulator trying to read 7 bytes instead of 8), we restrict the
 * access sizes more than strictly necessary. Should this limitation become too
 * strict, this can be revisited.
 */
bool size_supported(size_t size)
{
    switch (size) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16: return true;
    default: return false;
    };

    __UNREACHED__;
}

mmu_entry translate_linear_internal(virtual_mmu_interface& vmmu, vmmu_vcpu_interface& vcpu, uintptr_t lin_addr,
                                    bool is_implicit_supervisor, unsigned cpl, x86::memory_access_type_t type,
                                    std::optional<phy_addr_t> cr3)
{
    if (cr3) {
        // If a CR3 access override is requested, flush the TLB
        // to ensure that we don't hit any TLB entry that corresponds
        // to the current address space (the TLB does not use CR3 tags).
        vmmu.flush();
    }

    auto entry {vmmu.translate_linear(vcpu, lin_addr, is_implicit_supervisor, cpl, type, cr3)};

    if (cr3) {
        // Same as above, but the other way round. Make sure no subsequent request
        // for the current address space will hit any TLB entries of the overridden
        // address space.
        vmmu.flush();
    }

    return entry;
}

std::optional<lin_addr_t> translate(hedron::utcb& utcb, x86::cpu_mode mode, log_addr_t log_addr,
                                    x86::memory_access_type_t type, size_t bytes)
{
    auto& seg {utcb.get_segment(log_addr.seg)};
    segmentation_unit segment({log_addr.seg, seg.base, seg.limit, seg.ar}, mode);
    const auto addr_after_segmentation {segment.translate_logical_address(log_addr.off, type, bytes)};

    if (not addr_after_segmentation) {
        return {};
    }

    auto lin_addr {lin_addr_t(*addr_after_segmentation)};
    return lin_addr;
}

} // namespace memory

} // namespace vcpu
