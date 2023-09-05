/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// This test checks whether we can gracefully recover from running out of space for page tables.

#include <arch.hpp>
#include <baretest/baretest.hpp>
#include <slvm/api.h>
#include <trace.hpp>

TEST_CASE(memory_map_loop)
{
    const slvm_status_t vm_create_status {slvm_vmcall_create_vm()};
    PANIC_ON(vm_create_status < 0, "Failed to create VM");

    const auto vm_id {static_cast<slvm_vm_id_t>(vm_create_status)};

    size_t pages_mapped {0};

    // Try to map until this guest physical address. If we get here, something is wrong, because we must have consumed
    // terabytes worth of page tables and that should not be possible.
    const uint64_t gpa_max {1UL << 52};

    // This must be less than 512 to avoid creating superpages. Superpages ruin the purpose of this test to consume lots
    // of page table memory in the hypervisor.
    const size_t page_count {256};

    alignas(page_count * PAGE_SIZE) static char source_pages[page_count * PAGE_SIZE];
    const uint64_t src_fn {addr2pn(reinterpret_cast<uintptr_t>(source_pages))};

    slvm_status_t status {SLVMAPI_VMCALL_SUCCESS};

    for (uint64_t gpa {0}; gpa < gpa_max; gpa += page_count * PAGE_SIZE) {
        // Occasionally print out status so that the log shows how far we get.
        if ((pages_mapped * PAGE_SIZE) % (1 << 29) == 0) {
            info("Mapping {} MB...", (pages_mapped * PAGE_SIZE) >> 20);
        }

        uint64_t dst_gfn {addr2pn(gpa)};

        status = {slvm_vmcall_map_memory(vm_id, src_fn, dst_gfn, page_count, SLVMAPI_PROT_READ)};

        if (status != SLVMAPI_VMCALL_SUCCESS) {
            break;
        }

        pages_mapped += page_count;
    }

    BARETEST_ASSERT(status == SLVMAPI_VMCALL_ENOMEM);

    status = slvm_vmcall_unmap_memory(vm_id, 0, pages_mapped);
    BARETEST_ASSERT(status == SLVMAPI_VMCALL_SUCCESS);

    status = slvm_vmcall_destroy_vm(vm_id);
    BARETEST_ASSERT(status == SLVMAPI_VMCALL_SUCCESS);
}
