/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// This test tests proper resource reclamation in SLVM. Functional aspects are tested in the slvm test.
//
// We keep this as a separate test to immediately see whether we have a functional problem (slvm test fails) or a
// resource cleanup problem (slvm-resources test fails, but slvm test succeeds).

#include <cstdint>

#include <arch.hpp>
#include <baretest/baretest.hpp>
#include <slvm/api.h>
#include <trace.hpp>

#include <array>

namespace
{
// Eventually this number should approach infinity, but for now we settle for "good enough". This number always needs to
// be high enough that manual VM creation and destruction in SVP will never reach it.
constexpr size_t TEST_REPETITIONS {512};

// If we fail, it's nice to know how far we get, but we also don't want to spam the console.
constexpr size_t TEST_REPORT_EVERY_REPETITION {4};

// This set of operations is intended to represent "normal" usage of the SLVM API and should hit all functions that
// allocate resources in the VMM.
void start_stop_vm()
{
    // We are going to map this page over and over again ...
    alignas(PAGE_SIZE) static std::array<uint8_t, PAGE_SIZE> zero_page {};
    const uint64_t zero_page_pn {addr2pn(reinterpret_cast<uintptr_t>(zero_page.data()))};

    // ... this many times into the guest.
    const size_t four_kb_mappings {4096};

    const slvm_vm_id_t vm_id {slvm_vmcall_create_vm()};

    BARETEST_ASSERT(vm_id >= 0);

    for (size_t dst_page = 0; dst_page < four_kb_mappings; dst_page++) {
        BARETEST_ASSERT(slvm_vmcall_map_memory(vm_id, zero_page_pn, dst_page, 1, SLVMAPI_PROT_READ)
                        == SLVMAPI_VMCALL_SUCCESS);
    }

    // We don't bother running vCPUs, because that doesn't consume resources in the SLVM implementation. They are
    // eagerly created.

    BARETEST_ASSERT(slvm_vmcall_destroy_vm(vm_id) == SLVMAPI_VMCALL_SUCCESS);
}

} // namespace

TEST_CASE(vm_create_destroy_loop)
{
    for (size_t i = 0; i < TEST_REPETITIONS; i++) {
        start_stop_vm();

        if ((i % TEST_REPORT_EVERY_REPETITION) == 0) {
            info("Finished round {}", i);
        }
    }
}
