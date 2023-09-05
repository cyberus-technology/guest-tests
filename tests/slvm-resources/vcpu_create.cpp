/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// This test checks whether we can gracefully recover from creating too many vCPUs.

#include <cstdint>

#include <arch.hpp>
#include <baretest/baretest.hpp>
#include <slvm/api.h>
#include <trace.hpp>

#include <array>

namespace
{
constexpr size_t MAX_VCPUS {256};

using page = std::array<uint8_t, PAGE_SIZE>;

alignas(PAGE_SIZE) std::array<page, MAX_VCPUS> state_pages {};
alignas(PAGE_SIZE) std::array<page, MAX_VCPUS> xsave_pages {};

} // namespace

TEST_CASE(vcpu_create_loop)
{
    const slvm_status_t vm_create_status {slvm_vmcall_create_vm()};
    PANIC_ON(vm_create_status < 0, "Failed to create VM");

    const auto vm_id {static_cast<slvm_vm_id_t>(vm_create_status)};

    size_t vcpus_created {0};

    for (size_t i = 0; i < state_pages.size(); i++) {
        const uint64_t state_gfn {addr2pn(reinterpret_cast<uintptr_t>(state_pages[i].data()))};
        const uint64_t xsave_gfn {addr2pn(reinterpret_cast<uintptr_t>(xsave_pages[i].data()))};

        const slvm_status_t status {slvm_vmcall_run_vcpu_v4(vm_id, state_gfn, xsave_gfn, false).status};

        if (status < 0) {
            break;
        }

        vcpus_created++;
    }

    info("Created {} vCPU{s}.", vcpus_created, vcpus_created == 1 ? "" : "s");

    // It's hard to know how many vCPUs we can create exactly, but we should at least be able to create more than one.
    BARETEST_ASSERT(vcpus_created > 1);

    // If we fail this assertion, we are not testing whether we can recover from creating too many vCPUs anymore. At
    // that point, we either need to increase MAX_VCPUS or rethink this test.
    BARETEST_ASSERT(vcpus_created < MAX_VCPUS);

    BARETEST_ASSERT(slvm_vmcall_destroy_vm(vm_id) == SLVMAPI_VMCALL_SUCCESS);
}
