/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/utcb.hpp>
#include <vmmu_virtual_mmu.hpp>

namespace vcpu
{

namespace memory
{

[[maybe_unused]] bool size_supported(size_t size);

mmu_entry translate_linear_internal(virtual_mmu_interface& vmmu, vmmu_vcpu_interface& vcpu, uintptr_t lin_addr,
                                    bool is_implicit_supervisor, unsigned cpl, x86::memory_access_type_t type,
                                    std::optional<phy_addr_t> cr3);

std::optional<lin_addr_t> translate(hedron::utcb& utcb, x86::cpu_mode mode, log_addr_t log_addr,
                                    x86::memory_access_type_t type, size_t bytes);

template <class ACCESS_FN>
inline bool generic_access_helper(lin_addr_t lin_addr, size_t bytes, void* buf, unsigned cpl, bool implicit_supervisor,
                                  std::optional<phy_addr_t> cr3, ACCESS_FN access_fn, x86::memory_access_type_t type,
                                  virtual_mmu_interface& vmmu, vmmu_vcpu_interface& vcpu)
{
    ASSERT(size_supported(bytes), "invalid access, size = {}", bytes);
    ASSERT(static_cast<uintptr_t>(lin_addr) <= (~0ull) - (bytes - 1), "address overflow on addition");

    static constexpr size_t max_pages {2};
    uintptr_t paddr[max_pages];

    uintptr_t pn_start {lin_addr.pn()};
    uintptr_t pn_end {(lin_addr + (bytes - 1)).pn()};
    size_t npages {pn_end - pn_start + 1};
    uintptr_t page_boundary {(lin_addr.pn() + page_num_t {1}).to_address()};

    ASSERT(npages <= max_pages, "invalid number of pages: {}", npages);

    // We need to do all translations first. Otherwise, we might end up with a partial memory access.
    // This is exactly what the hardware does for single instruction accesses.
    for (size_t page {0}; page < npages; page++) {
        auto entry {translate_linear_internal(vmmu, vcpu, page ? page_boundary : static_cast<uintptr_t>(lin_addr),
                                              implicit_supervisor, cpl, type, cr3)};

        if (not std::holds_alternative<phy_addr_t>(entry)) {
            // TODO Injection exceptions is not supported yet. See #452.
            return false;
        }

        paddr[page] = uintptr_t(static_cast<uintptr_t>(std::get<phy_addr_t>(entry)));
    }

    // we can finally access the actual data knowing that all translations are valid
    for (size_t page {0}; page < npages; page++) {
        size_t copy_size {page ? static_cast<uintptr_t>(lin_addr) + bytes - page_boundary
                               : std::min(bytes, page_boundary - static_cast<uintptr_t>(lin_addr))};
        void* target_buffer {page ? static_cast<char*>(buf) + page_boundary - static_cast<uintptr_t>(lin_addr) : buf};

        if (not access_fn(phy_addr_t(paddr[page]), copy_size, target_buffer, type)) {
            return false;
        }
    }

    return true;
}

template <typename ACCESS_OVERRIDE>
inline std::optional<phy_addr_t> translate(virtual_mmu_interface& vmmu, vmmu_vcpu_interface& vcpu, lin_addr_t lin_addr,
                                           x86::memory_access_type_t type, size_t bytes, ACCESS_OVERRIDE ovrd,
                                           unsigned cpl)
{
    uintptr_t pn_start {lin_addr.pn()};
    uintptr_t pn_end {(lin_addr + bytes).pn()};
    size_t npages {pn_end - pn_start + 1};
    PANIC_UNLESS(npages == 1, "Crossing page boundary not supported yet!");

    auto entry {translate_linear_internal(vmmu, vcpu, static_cast<uintptr_t>(lin_addr), ovrd.implicit_supervisor, cpl,
                                          type, ovrd.cr3)};

    if (std::holds_alternative<phy_addr_t>(entry)) {
        return std::get<phy_addr_t>(entry);
    }

    // TODO Injection exceptions is not supported yet. See #452.
    return {};
}

template <typename ACCESS_OVERRIDE, typename ACCESS_FN>
inline bool access_lin_addr(virtual_mmu_interface& vmmu, vmmu_vcpu_interface& vcpu, ACCESS_FN access_fn,
                            lin_addr_t lin_addr, size_t bytes, void* buf, unsigned cpl, x86::memory_access_type_t type,
                            ACCESS_OVERRIDE ovrd)
{
    return generic_access_helper(lin_addr, bytes, buf, cpl, ovrd.implicit_supervisor, ovrd.cr3, access_fn, type, vmmu,
                                 vcpu);
}

template <typename ACCESS_OVERRIDE, typename ACCESS_FN>
inline bool access_log_addr(hedron::utcb& utcb, x86::cpu_mode mode, virtual_mmu_interface& vmmu,
                            vmmu_vcpu_interface& vcpu, ACCESS_FN access_fn, log_addr_t log_addr, size_t bytes,
                            void* buf, unsigned cpl, x86::memory_access_type_t type, ACCESS_OVERRIDE ovrd)
{
    auto lin_addr {translate(utcb, mode, log_addr, type, bytes)};

    if (not lin_addr) {
        return false;
    }

    return access_lin_addr(vmmu, vcpu, access_fn, *lin_addr, bytes, buf, cpl, type, ovrd);
}

} // namespace memory

} // namespace vcpu
